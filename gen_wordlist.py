from bitstring import BitArray
from dahuffman import HuffmanCodec
from collections import defaultdict
from itertools import pairwise, chain


def encode_single(codec, item):
    b, v = codec.get_code_table()[item]
    return BitArray(uint=v, length=b)


def encode_word(codecs, text):
    result = BitArray()
    for (p, c) in pairwise(chain([None], text)):
        result += encode_single(codecs[p], c)
    return result


def decode_single(codec, bitarray):
    lookup = {(b, v): s for s, (b, v) in codec.get_code_table().items()}

    for size in range(1,len(bitarray)+1):
        buffer = bitarray[:size].uint
        if (size, buffer) in lookup:
            return lookup[size, buffer], size


def decode_word(codecs, bitarray, length):
    taken = 0
    result = ''
    char = None
    while len(result) < length:
        char, size = decode_single(codecs[char], bitarray[taken:])
        result += char
        taken += size
    return result, taken


def pad_to(bits, length):
    return bits if len(bits) == length else bits + BitArray(uint=0, length=length-len(bits))


def nth_0_pos(bitarray, n):
    seen = 0
    for i, v in enumerate(bitarray):
        if not v:
            seen += 1
        if seen == n:
            return i
    return len(bitarray)


def encode(words):
    frequencies = defaultdict(lambda: defaultdict(int))
    for word in words:
        for (p, c) in pairwise(chain([None], word)):
            frequencies[p][c] += 1

    codecs = {k: HuffmanCodec.from_frequencies(v, eof=next(iter(v))) for k, v in frequencies.items()}
    for prev, codec in codecs.items():
        print(prev)
        codec.print_code_table()
    bitarrays = [encode_word(codecs, w) for w in words]
    max_len = max(len(x) for x in bitarrays)
    print('max huffman length:', max_len)
    sorted_words = sorted(bitarrays, key=lambda x: (pad_to(x, max_len)).uint)
    # print(decode_n(codec, sorted_words[0], 0, 5)[0], sorted_words[0].bin)
    match_lengths = []
    novel_bits_length = 0
    for prev, w in pairwise([BitArray(length=max_len)] + sorted_words):
        pw_min = min(len(prev), len(w))
        matching, = (prev[:pw_min] ^ w[:pw_min]).find(BitArray(uint=1,length=1))
        novel_bits_length += len(w) - matching - 1
        match_lengths.append(matching)
        # print(decode_bitarray(codec, w), w.bin)
    print(match_lengths)
    matching_zeros = [sum(not x for x in w[:l]) for w, l in zip([BitArray(length=max_len)] + sorted_words, match_lengths)]
    print(matching_zeros)
    print("novel bits:", novel_bits_length / 8)
    matching_zeros_freqs = defaultdict(int)
    for d in matching_zeros:
        matching_zeros_freqs[d] += 1
    matching_zeros_codec = HuffmanCodec.from_frequencies(matching_zeros_freqs, eof=1)
    matching_zeros_codec.print_code_table()
    print("total zero lens len:", sum(len(encode_single(matching_zeros_codec, x)) for x in matching_zeros) / 8)
    result = BitArray()
    for (p, w), l in zip(pairwise([BitArray(length=max_len)] + sorted_words), matching_zeros):
        result += encode_single(matching_zeros_codec, l) + w[nth_0_pos(p, l + 1) + 1:]
    return result, codecs, matching_zeros_codec


def decode(data, codecs, matching_zeros_codec):
    result = set()
    word = BitArray(length=64)
    pos = 0

    while pos < len(data):
        match_zeros, taken = decode_single(matching_zeros_codec, data[pos:])
        match_len = nth_0_pos(word, match_zeros + 1)
        pos += taken
        word = word[:match_len] + BitArray([not word[match_len]]) + data[pos:]
        chars, taken = decode_word(codecs, word, 5)
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
    decoded = decode(encoded, word_codec, len_codec)
    # print(decoded)
    print(len(words), len(decoded), words == decoded)

