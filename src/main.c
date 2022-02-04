#include <string.h>
#include <tice.h>
#include <time.h>
#include <debug.h>
#include "graphics.h"
#include "words.h"

/*
 *--------------------------------------
 * Program Name: Wordle
 * Author: commandblockguy
 * License:
 * Description:
 *--------------------------------------
*/

const char (*words)[WORD_LENGTH];
uint16_t num_words;

enum tile_type get_tile_type(const char guesses[][WORD_LENGTH], uint8_t guess, uint8_t pos, const char *word, uint8_t num_guesses) {
    char c = guesses[guess][pos];
    if(!c) {
        return TILE_EMPTY;
    } else if(guess == num_guesses) {
        return TILE_UNSUBMITTED;
    } else if(c == word[pos]) {
        return TILE_CORRECT;
    } else {
        // Count occurrences in answer
        uint8_t count_answer = 0;
        for(uint8_t i = 0; i < WORD_LENGTH; i++) {
            if(c == word[i]) {
                count_answer++;
            }
        }
        // Count how many tiles with this letter have already been shown as present or correct
        uint8_t count_prev = 0;
        for(uint8_t i = 0; i < pos; i++) {
            if(c == guesses[guess][i]) {
                count_prev++;
            }
        }
        // Only mark present if there are fewer of this character already marked than there are in the answer
        if(count_answer > count_prev) return TILE_PRESENT;
        else return TILE_ABSENT;
    }
}

bool is_word_in_list(const char *word) {
    int min = 0;
    int max = num_words;
    while(min <= max) {
        int mean = (min + max) / 2;
        int diff = memcmp(words[mean], word, WORD_LENGTH);
        if(diff < 0) min = mean + 1;
        else if(diff > 0) max = mean - 1;
        else return true;
    }
    return false;
}

const char *validate_word(const char guesses[][WORD_LENGTH], const char *word, uint8_t cur_guess, uint8_t settings) {
    const char *guess = guesses[cur_guess];
    if(strnlen(guess, 5) != 5) return "Not enough letters";
    if(!is_word_in_list(guess)) return "Not in word list";

    if(settings & SETTING_HARD && cur_guess > 0) {
        const char *prev_guess = guesses[cur_guess - 1];
        static char text_position[] = "___ letter must be _";
        dbg_printf("hard mode\n");
        for(uint8_t i = 0; i < WORD_LENGTH; i++) {
            const char ordinals[WORD_LENGTH][3] = {"1st", "2nd", "3rd", "4th", "5th"};
            if(guess[i] != prev_guess[i] && get_tile_type(guesses, cur_guess - 1, i, word, cur_guess) == TILE_CORRECT) {
                memcpy(text_position, ordinals[i], 3);
                text_position[19] = prev_guess[i];
                return text_position;
            }
        }
        static char text_contains[] = "Guess must contain _";
        for(uint8_t i = 0; i < WORD_LENGTH; i++) {
            if(get_tile_type(guesses, cur_guess - 1, i, word, cur_guess) != TILE_PRESENT) {
                continue;
            }
            bool found = false;
            for(uint8_t j = 0; j < WORD_LENGTH; j++) {
                if(guess[j] == prev_guess[i]) {
                    found = true;
                    break;
                }
            }
            if(!found) {
                text_contains[19] = prev_guess[i];
                return text_contains;
            }
        }
        return NULL;
    } else {
        return NULL;
    }
}

int get_day_number(void) {
    time_t current = time(NULL);
    time_t start = 1624060800;
    return (long)(current - start) / (60 * 60 * 24);
}

void screen_help(void) {
    do {
        graphics_screen_help();
    } while(!os_GetCSC());
}

void screen_settings(uint8_t *settings, uint24_t day) {
    int8_t selection = 0;
    sk_key_t key;
    do {
        key = os_GetCSC();
        switch (key) {
            case sk_Up: {
                selection--;
                if(selection < 0) {
                    selection = 2;
                }
                break;
            }
            case sk_Down: {
                selection++;
                if(selection >= 3) {
                    selection = 0;
                }
                break;
            }
            case sk_Enter:
            case sk_Left:
            case sk_Right: {
                *settings ^= 1 << selection;
                graphics_set_palette(*settings);
                break;
            }
        }
        graphics_screen_settings(*settings, selection, day);
    } while(key != sk_Clear);
}

void play_game(int day) {
    struct save save = {};

    FILE *f = fopen("WORDLE", "r");
    if(f) {
        fread(&save, sizeof save, 1, f);
        if(save.day != day) {
            if(save.day != day - 1) {
                save.current_streak = 0;
            }
            memset(save.guesses, 0, sizeof save.guesses);
            save.num_guesses = 0;
        }
        fclose(f);
        f = NULL;
    } else {
        screen_help();
        save.settings = SETTING_DARK;
    }

    graphics_set_palette(save.settings);

    dbg_printf("day: %u\n", save.day);
    dbg_printf("num guesses: %u\n", save.num_guesses);
    dbg_printf("streak: %u\n", save.current_streak);
    dbg_printf("max streak: %u\n", save.max_streak);
    dbg_printf("played: %u\n", save.games_played);
    dbg_printf("guesses: %.30s\n", save.guesses);

    save.day = day;

    const char *word = answers[day];
    uint8_t input_index = strnlen(save.guesses[save.num_guesses], WORD_LENGTH);
    struct anim_state anim_state;
    bool completed = save.num_guesses == MAX_GUESSES || (save.num_guesses && memcmp(save.guesses[save.num_guesses - 1], word, WORD_LENGTH) == 0);
    const char *toast = NULL;

    graphics_start_anim(&anim_state, ANIM_FLIP_ALL);

    sk_key_t key;
    while ((key = os_GetCSC()) != sk_Clear) {
        switch (key) {
            case sk_Enter: {
                if(!completed) {
                    if(memcmp(save.guesses[save.num_guesses], word, WORD_LENGTH) == 0) {
                        // Guess matches word
                        save.games_played++;
                        save.guess_counts[save.num_guesses]++;
                        save.current_streak++;
                        if(save.current_streak > save.max_streak) {
                            save.max_streak = save.current_streak;
                        }
                        save.num_guesses++;
                        input_index = 0;
                        graphics_start_anim(&anim_state, ANIM_SUCCESS);
                        completed = true;
                        static const char *results[] = {
                                "Genius",
                                "Magnificent",
                                "Impressive",
                                "Splendid",
                                "Great",
                                "Phew",
                        };
                        toast = results[save.num_guesses - 1];
                    } else if(!(toast = validate_word(save.guesses, word, save.num_guesses,  save.settings))) {
                        // Guess is valid word that's not today's
                        save.num_guesses++;
                        input_index = 0;
                        graphics_start_anim(&anim_state, ANIM_FLIP_LINE);
                        if(save.num_guesses == MAX_GUESSES) {
                            // Game is lost
                            completed = true;
                            save.games_played++;
                            save.current_streak = 0;
                            toast = word;
                        }
                    } else {
                        // Word is invalid
                        graphics_start_anim(&anim_state, ANIM_INVALID_WORD);
                    }
                }
                break;
            }

            case sk_Del: {
                if(input_index > 0 && !completed) {
                    input_index--;
                    save.guesses[save.num_guesses][input_index] = 0;
                    toast = NULL;
                }
                break;
            }

            case sk_Yequ: {
                screen_help();
                break;
            }

            case sk_Trace: {
                do {
                    graphics_screen_stats(save.games_played, save.current_streak, save.max_streak, save.guess_counts,
                                          completed ? save.num_guesses : 0);
                } while(!os_GetCSC());
                break;
            }

            case sk_Graph: {
                screen_settings(&save.settings, day);
                break;
            }

            default: {
                const char *chars = "\0\0\0\0\0\0\0\0\0\0\0WRMH\0\0\0\0VQLG\0\0\0ZUPKFC\0\0YTOJEB\0\0XSNIDA\0\0\0\0\0\0\0\0";
                if(chars[key] && input_index < WORD_LENGTH && !completed) {
                    save.guesses[save.num_guesses][input_index] = chars[key];
                    input_index++;
                    graphics_start_anim(&anim_state, ANIM_TYPE_LETTER);
                }
            }
        }

        graphics_frame(save.num_guesses, word, save.guesses, toast, &anim_state);
    }

    f = fopen("WORDLE", "w");
    if(f) {
        fwrite(&save, sizeof save, 1, f);
        fclose(f);
        f = NULL;
    } else {
        dbg_printf("write fopen failed\n");
    }
}

void error_set_time(void) {
    do {
        const char *msg[] = {
                "Before playing, please set",
                "the system clock to today's",
                "date from the mode menu.",
        };
        graphics_screen_error(msg, 3);
    } while(os_GetCSC() != sk_Clear);
}

void error_no_puzzle(void) {
    do {
        const char *msg[] = {
                "We've somehow run out of puzzles.",
                "",
                "Sorry about that, inhabitant",
                "of the far future."
        };
        graphics_screen_error(msg, 4);
    } while(os_GetCSC() != sk_Clear);
}

void error_no_appvar(void) {
    do {
        const char *msg[] = {
                "Word list not found.",
                "Please resend WORDS.8xv.",
        };
        graphics_screen_error(msg, 2);
    } while(os_GetCSC() != sk_Clear);
}

int main(void) {
    graphics_init();

    int day = get_day_number();
    dbg_printf("WORDLE: day %i\n", day);

    void *appvar_data = os_GetAppVarData("WORDS", NULL);

    if(!appvar_data) {
        error_no_appvar();
    } else if(day < 0) {
        error_set_time();
    } else if(day >= num_answers) {
        error_no_puzzle();
    } else {
        words = appvar_data + 2;
        num_words = *((uint16_t*)appvar_data) / WORD_LENGTH;
        dbg_printf("found %u words, starting with %.5s\n", num_words, words[0]);
        play_game(day);
    }

    graphics_cleanup();
    return 0;
}
