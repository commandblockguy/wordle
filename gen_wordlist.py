from bitstring import BitArray
from dahuffman import HuffmanCodec
from collections import defaultdict
from itertools import pairwise


def encode_bitarray(codec, text):
    result = BitArray()
    for s in text:
        b, v = codec.get_code_table()[s]
        result += BitArray(uint=v, length=b)
    return result


def decode_n(codec, bitarray, length):
    lookup = {(b, v): s for s, (b, v) in codec.get_code_table().items()}

    taken = 0
    size = 0
    result = []
    while len(result) < length:
        size += 1
        buffer = bitarray[taken:taken+size].uint
        if (size, buffer) in lookup:
            result.append(lookup[size, buffer])
            taken += size
            size = 0
    return result, taken


def pad_to(bits, length):
    return bits if len(bits) == length else bits + BitArray(uint=0, length=length-len(bits))


def encode(words):
    frequencies = defaultdict(int)
    for word in words:
        for l in word:
            frequencies[l] += 1
    codec = HuffmanCodec.from_frequencies(frequencies, eof='Q')
    codec._eof = None
    codec.print_code_table()
    bitarrays = [encode_bitarray(codec, w) for w in words]
    max_len = max(len(x) for x in bitarrays)
    print('max huffman length:', max_len)
    sorted_words = sorted(bitarrays, key=lambda x: (pad_to(x, max_len)).uint)
    # print(decode_n(codec, sorted_words[0], 0, 5)[0], sorted_words[0].bin)
    match_lengths = []
    novel_bits_length = 0
    for prev, w in pairwise(sorted_words):
        pw_min = min(len(prev), len(w))
        matching, = (prev[:pw_min] ^ w[:pw_min]).find(BitArray(uint=1,length=1))
        novel_bits_length += len(w) - matching - 1
        match_lengths.append(matching)
        # print(decode_bitarray(codec, w), w.bin)
    match_diffs = [c - p for p, c in pairwise(match_lengths)]
    print("novel bits:", novel_bits_length / 8)
    diff_freqs = defaultdict(int)
    for d in match_diffs:
        diff_freqs[d] += 1
    print(diff_freqs)
    diff_len_codec = HuffmanCodec.from_frequencies(diff_freqs, eof=1)
    diff_len_codec._eof = None
    diff_len_codec.print_code_table()
    # todo: divide by 2, round down, should save on average 1/2 a bit? - maybe not due to 0 being absent
    result = sorted_words[0] + BitArray(uint=match_lengths[0], length=6) + sorted_words[1][match_lengths[0]:]
    for w, l, d in zip(sorted_words[2:], match_lengths[1:], match_diffs):
        # todo: the position we break on will always be a zero bit - take that into account when encoding diffs?
        result += encode_bitarray(diff_len_codec, [d]) + w[l+1:]
    return result, codec, diff_len_codec


def decode(data, word_codec, diff_len_codec):
    result = set()
    word, pos = decode_n(word_codec, data, 5)
    result.add(''.join(word))
    match_len = data[pos:pos+6].uint
    pos += 6
    word = data[:match_len] + data[pos:]
    chars, taken = decode_n(word_codec, word, 5)
    pos += taken - match_len
    result.add(''.join(chars))

    while pos < len(data):
        (offset,), taken = decode_n(diff_len_codec, data[pos:], 1)
        match_len += offset
        pos += taken
        word = word[:match_len] + BitArray([not word[match_len]]) + data[pos:]
        chars, taken = decode_n(word_codec, word, 5)
        pos += taken - match_len - 1
        result.add(''.join(chars))

    return result


with open('words.txt', 'r') as f:
    words = set()
    for line in f.readlines():
        w = line[:5]
        words.add(w)
    encoded, word_codec, len_codec = encode(words)
    # print(encoded)
    print(len(encoded) / 8)
    decoded = sorted(decode(encoded, word_codec, len_codec))
    # print(result)
    print(len(words), len(decoded), sorted(words) == decoded)

