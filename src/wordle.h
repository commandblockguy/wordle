#ifndef WORDLE_WORDLE_H
#define WORDLE_WORDLE_H

#include <stdint.h>
#include <stdbool.h>

#define WORD_LENGTH 5
#define MAX_GUESSES 6

#define SETTING_HARD     (1 << 0)
#define SETTING_DARK     (1 << 1)
#define SETTING_CONTRAST (1 << 2)

struct save {
    uint16_t day;
    uint8_t num_guesses;
    char guesses[MAX_GUESSES][WORD_LENGTH];
    uint16_t games_played;
    uint16_t current_streak;
    uint16_t max_streak;
    uint16_t guess_counts[MAX_GUESSES];
    uint8_t settings;
};

enum tile_type {
    TILE_EMPTY,
    TILE_UNSUBMITTED,
    TILE_ABSENT,
    TILE_PRESENT,
    TILE_CORRECT,
};

enum tile_type get_tile_type(const char guesses[][WORD_LENGTH], uint8_t guess, uint8_t pos, const char *word, uint8_t num_guesses);

#endif //WORDLE_WORDLE_H
