#ifndef WORDLE_GRAPHICS_H
#define WORDLE_GRAPHICS_H

#include "wordle.h"

enum animation {
    ANIM_TYPE_LETTER,
    ANIM_FLIP_ALL,
    ANIM_FLIP_LINE,
    ANIM_INVALID_WORD,
    ANIM_SUCCESS,
};

struct anim_state {
    enum animation animation;
    uint8_t frame;
};

void graphics_init(void);
void graphics_cleanup(void);

void graphics_set_palette(uint8_t settings);

void graphics_frame(uint8_t cur_line, const char *word, const char guesses[][WORD_LENGTH], const char *toast, struct anim_state *anim_state);

void graphics_start_anim(struct anim_state *state, enum animation anim);

void graphics_screen_error(const char *msg[], uint8_t lines);
void graphics_screen_help(void);
void graphics_screen_stats(uint24_t games_played, uint24_t current_streak, uint24_t max_streak, uint16_t *guess_counts,
                           uint8_t current_guesses);
void graphics_screen_settings(uint8_t settings, uint8_t selection, uint24_t day);

#endif //WORDLE_GRAPHICS_H
