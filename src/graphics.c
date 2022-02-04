#include "graphics.h"

#include <graphx.h>
#include <time.h>
#include <tice.h>

#include "gfx/gfx.h"

#define TILE_SIZE 24
#define TILE_SPACING 3
#define TILE_BASE_X (LCD_WIDTH / 2 - 2 * (TILE_SIZE + TILE_SPACING))
#define TILE_BASE_Y 66

enum color {
    COLOR_BG,
    COLOR_ICONS,
    COLOR_TEXT,
    COLOR_UNUSED,
    COLOR_ABSENT,
    COLOR_PRESENT,
    COLOR_CORRECT,
    COLOR_WHITE,
};

const uint16_t palette_dark[] = {
        [COLOR_BG]      = gfx_RGBTo1555(0x12, 0x12, 0x13),
        [COLOR_ICONS]   = gfx_RGBTo1555(0x56, 0x57, 0x58),
        [COLOR_TEXT]    = gfx_RGBTo1555(0xd7, 0xda, 0xdc),
        [COLOR_UNUSED]  = gfx_RGBTo1555(0x81, 0x83, 0x84),
        [COLOR_ABSENT]  = gfx_RGBTo1555(0x3a, 0x3a, 0x3c),
        [COLOR_PRESENT] = gfx_RGBTo1555(0xb5, 0x9f, 0x3b),
        [COLOR_CORRECT] = gfx_RGBTo1555(0x53, 0x8d, 0x4e),
        [COLOR_WHITE]   = gfx_RGBTo1555(0xd7, 0xda, 0xdc),
};

const uint16_t palette_light[] = {
        [COLOR_BG]      = gfx_RGBTo1555(0xff, 0xff, 0xff),
        [COLOR_ICONS]   = gfx_RGBTo1555(0x87, 0x8a, 0x8c),
        [COLOR_TEXT]    = gfx_RGBTo1555(0x00, 0x00, 0x00),
        [COLOR_UNUSED]  = gfx_RGBTo1555(0xd3, 0xd6, 0xda),
        [COLOR_ABSENT]  = gfx_RGBTo1555(0x78, 0x7c, 0x7e),
        [COLOR_PRESENT] = gfx_RGBTo1555(0xc9, 0xb4, 0x58),
        [COLOR_CORRECT] = gfx_RGBTo1555(0x6a, 0xaa, 0x64),
        [COLOR_WHITE]   = gfx_RGBTo1555(0xff, 0xff, 0xff),
};

const uint24_t anim_lengths[] = {
        [ANIM_TYPE_LETTER]  = 5,
        [ANIM_FLIP_ALL]     = 35,
        [ANIM_FLIP_LINE]    = 55,
        [ANIM_INVALID_WORD] = 20,
        [ANIM_SUCCESS]      = 100,
};

static void draw_boxed_char(char c, enum color bg, enum color border, uint24_t center_x, uint24_t center_y, uint8_t width, uint8_t height);
static void get_tile_colors(enum color *bg, enum color *border, enum tile_type type);
static void disp_title(void);
static void disp_toast(const char *toast);

void graphics_init(void) {
    gfx_Begin();
    graphics_set_palette(SETTING_DARK);
    gfx_FillScreen(COLOR_BG);
    gfx_SetDrawBuffer();
    timer_Enable(1, TIMER_32K, TIMER_NOINT, TIMER_UP);
    timer_Set(1, 0);
}

void graphics_cleanup(void) {
    gfx_End();
}

void graphics_frame(uint8_t cur_line, const char *word, const char guesses[][WORD_LENGTH], const char *toast, struct anim_state *anim_state) {
    gfx_SwapDraw();
    gfx_FillScreen(COLOR_BG);

    disp_title();

    if(toast && (anim_state->frame == 0 || anim_state->animation == ANIM_INVALID_WORD)) {
        disp_toast(toast);
    }

    for(uint8_t y = 0; y < MAX_GUESSES; y++) {
        for(uint8_t x = 0; x < WORD_LENGTH; x++) {
            enum color bg, border;

            enum tile_type type = get_tile_type(guesses, y, x, word, cur_line);
            get_tile_colors(&bg, &border, type);

            uint24_t center_x = TILE_BASE_X + (TILE_SIZE + TILE_SPACING) * x;
            uint24_t center_y = TILE_BASE_Y + (TILE_SIZE + TILE_SPACING) * y;

            uint8_t width = TILE_SIZE;
            uint8_t height = TILE_SIZE;

            if(anim_state->frame) {
                switch(anim_state->animation) {
                    case ANIM_TYPE_LETTER: {
                        if(y == cur_line && (x == WORD_LENGTH - 1 || !guesses[y][x+1]) && guesses[y][x]) {
                            if(anim_state->frame == 5) {
                                // skip drawing this tile this frame
                                continue;
                            }
                            const int8_t sizes[] = {1, 2, 1, -1, 0};
                            width = TILE_SIZE + sizes[anim_state->frame - 1];
                            height = TILE_SIZE + sizes[anim_state->frame - 1];
                        }
                        break;
                    }
                    case ANIM_FLIP_ALL: {
                        if(y < cur_line && guesses[y][x]) {
                            int tile_frame = 35 - anim_state->frame - 5 * x;
                            if(tile_frame < 7) {
                                bg = COLOR_BG;
                                border = COLOR_ICONS;
                            }
                            if(tile_frame > 0 && tile_frame <= 15) {
                                height = abs(TILE_SIZE - tile_frame * 2 * TILE_SIZE / 15);
                            }
                        }
                        break;
                    }
                    case ANIM_FLIP_LINE: {
                        if(y == cur_line - 1) {
                            int tile_frame = 55 - anim_state->frame - 10 * x;
                            if(tile_frame < 7) {
                                bg = COLOR_BG;
                                border = COLOR_ICONS;
                            }
                            if(tile_frame > 0 && tile_frame <= 15) {
                                height = abs(TILE_SIZE - tile_frame * 2 * TILE_SIZE / 15);
                            }
                        }
                        break;
                    }
                    case ANIM_INVALID_WORD: {
                        if(y == cur_line) {
                            if(anim_state->frame % 4 == 0) center_x--;
                            if(anim_state->frame % 4 == 2) center_x++;
                        }
                        break;
                    }
                    case ANIM_SUCCESS: {
                        if(y == cur_line - 1) {
                            int tile_flip_frame = 100 - anim_state->frame - 10 * x;
                            if(tile_flip_frame < 7) {
                                bg = COLOR_BG;
                                border = COLOR_ICONS;
                            }
                            if(tile_flip_frame > 0 && tile_flip_frame <= 15) {
                                height = abs(TILE_SIZE - tile_flip_frame * 2 * TILE_SIZE / 15);
                            }

                            int tile_hop_frame = 40 - anim_state->frame - 5 * x;
                            int8_t heights[20] = {1, 3, 6, 8, 10, 11, 12, 8, 4, -1, 0, 4, 5, 5, 4, 2, 1, 0, -1, -1};
                            if(tile_hop_frame > 0 && tile_hop_frame <= 20) {
                                center_y -= heights[tile_hop_frame - 1];
                            }
                        }
                        break;
                    }
                }
            }

            draw_boxed_char(guesses[y][x], bg, border, center_x, center_y, width, height);
        }
    }

    const uint8_t icon_y = LCD_HEIGHT - 22;
    gfx_RLETSprite_NoClip(icon_help, 1 * LCD_WIDTH / 10 - icon_help_width / 2, icon_y);
    gfx_RLETSprite_NoClip(icon_stats, 7 * LCD_WIDTH / 10 - icon_stats_width / 2, icon_y);
    gfx_RLETSprite_NoClip(icon_settings, 9 * LCD_WIDTH / 10 - icon_settings_width / 2, icon_y);

#ifndef NDEBUG
    // debug stuff
    gfx_SetTextFGColor(COLOR_TEXT);
    gfx_SetTextScale(1, 1);
    gfx_SetTextXY(2, 30);
    gfx_PrintUInt(32768 / timer_Get(1), 1);
    timer_Set(1, 0);
    gfx_SetTextXY(2, 40);
    gfx_PrintUInt(anim_state->animation, 1);
    gfx_SetTextXY(2, 50);
    gfx_PrintUInt(anim_state->frame, 1);
#endif

    if(anim_state->frame) {
        anim_state->frame--;
    }
}

void graphics_start_anim(struct anim_state *state, enum animation anim) {
    state->animation = anim;
    state->frame = anim_lengths[anim];
}

static void draw_boxed_char(char c, enum color bg, enum color border, uint24_t center_x, uint24_t center_y, uint8_t width, uint8_t height) {
    uint24_t base_x = center_x - width / 2;
    uint24_t base_y = center_y - height / 2;
    if(bg != COLOR_BG) {
        gfx_SetColor(bg);
        gfx_FillRectangle_NoClip(base_x, base_y, width, height);
    }
    gfx_SetColor(border);
    gfx_Rectangle_NoClip(base_x, base_y, width, height);
    uint8_t height_scale = (height - 4) / 8;
    if(c && height_scale) {
        gfx_SetTextScale(2, height_scale);
        gfx_SetTextFGColor(COLOR_WHITE);
        gfx_SetTextXY(center_x - gfx_GetCharWidth(c) / 2 + 1, center_y - height_scale * 4 + 1);
        gfx_PrintChar(c);
    }
}

static void get_tile_colors(enum color *bg, enum color *border, enum tile_type type) {
    switch (type) {
        case TILE_EMPTY: {
            *border = COLOR_ABSENT;
            *bg = COLOR_BG;
            return;
        }
        case TILE_UNSUBMITTED: {
            *border = COLOR_ICONS;
            *bg = COLOR_BG;
            return;
        }
        case TILE_CORRECT: *border = *bg = COLOR_CORRECT; return;
        case TILE_PRESENT: *border = *bg = COLOR_PRESENT; return;
        case TILE_ABSENT: *border = *bg = COLOR_ABSENT; return;
    }
}

static void disp_title(void) {
    gfx_SetTextScale(2, 2);
    gfx_SetTextFGColor(COLOR_TEXT);
    const char string[] = "WORDLE";
    gfx_PrintStringXY(string, (LCD_WIDTH - gfx_GetStringWidth(string)) / 2, 4);
    gfx_SetColor(COLOR_ICONS);
    gfx_HorizLine_NoClip(0, 24, LCD_WIDTH);
}

static void disp_toast(const char *toast) {
    gfx_SetTextScale(1, 1);
    const uint8_t base_y = 32;
    uint24_t width = gfx_GetStringWidth(toast) + 8;
    uint24_t base_x = (LCD_WIDTH - width) / 2;
    gfx_SetColor(COLOR_TEXT);
    gfx_FillRectangle_NoClip(base_x, base_y + 1, width, 14);
    gfx_HorizLine_NoClip(base_x + 1, base_y, width - 2);
    gfx_HorizLine_NoClip(base_x + 1, base_y + 15, width - 2);
    gfx_SetTextFGColor(COLOR_BG);
    gfx_PrintStringXY(toast, base_x + 4, base_y + 4);
}

void graphics_screen_error(const char *msg[], uint8_t lines) {
    gfx_FillScreen(COLOR_BG);
    disp_title();
    gfx_SetTextFGColor(COLOR_TEXT);
    gfx_SetTextScale(1, 1);
    for(uint8_t i = 0; i < lines; i++) {
        gfx_PrintStringXY(msg[i], (LCD_WIDTH - gfx_GetStringWidth(msg[i])) / 2, LCD_HEIGHT / 3 + 10 * i);
    }
    gfx_SwapDraw();
}

void graphics_screen_help(void) {
    uint24_t base_x = 4;

    gfx_FillScreen(COLOR_BG);
    gfx_SetTextScale(1, 1);
    gfx_SetTextFGColor(COLOR_TEXT);

    const char *how_to_play = "HOW TO PLAY";
    gfx_PrintStringXY(how_to_play, (LCD_WIDTH - gfx_GetStringWidth(how_to_play)) / 2, 4);

    const struct {
        uint8_t y;
        const char *str;
    } text[] = {
            {16, "Guess the WORDLE in 6 tries."},
            {29, "Each guess must be a valid 5 letter word."},
            {39, "Hit the enter key to submit."},
            {52, "After each guess, the color of the tiles"},
            {62, "will change to show how close your guess"},
            {72, "was to the word."},
            {85, "Examples"},
            {124, "W is in the word and in the correct spot."},
            {168, "I is in the word but in the wrong spot."},
            {212, "G is not in the word in any spot."},
            {227, "A new WORDLE will be available each day!"},
    };

    for(uint8_t i = 0; i < sizeof text / sizeof text[0]; i++) {
        gfx_PrintStringXY(text[i].str, base_x, text[i].y);
    }

    const struct {
        uint8_t y;
        struct {
            char c;
            enum color bg;
            enum color border;
        } tiles[WORD_LENGTH];
    } examples[] = {
            {95, {
                    {'W', COLOR_CORRECT, COLOR_CORRECT},
                    {'E', COLOR_BG, COLOR_ABSENT},
                    {'A', COLOR_BG, COLOR_ABSENT},
                    {'R', COLOR_BG, COLOR_ABSENT},
                    {'Y', COLOR_BG, COLOR_ABSENT},
            }},
            {139, {
                    {'P', COLOR_BG, COLOR_ABSENT},
                    {'I', COLOR_PRESENT, COLOR_PRESENT},
                    {'L', COLOR_BG, COLOR_ABSENT},
                    {'L', COLOR_BG, COLOR_ABSENT},
                    {'S', COLOR_BG, COLOR_ABSENT},
            }},
            {183, {
                    {'V', COLOR_BG, COLOR_ABSENT},
                    {'A', COLOR_BG, COLOR_ABSENT},
                    {'G', COLOR_ABSENT, COLOR_ABSENT},
                    {'U', COLOR_BG, COLOR_ABSENT},
                    {'E', COLOR_BG, COLOR_ABSENT},
            }},
    };

    for(uint8_t i = 0; i < sizeof examples / sizeof examples[0]; i++) {
        for(uint8_t j = 0; j < WORD_LENGTH; j++) {
            draw_boxed_char(examples[i].tiles[j].c, examples[i].tiles[j].bg, examples[i].tiles[j].border,
                            base_x + TILE_SIZE / 2 + (TILE_SIZE + TILE_SPACING) * j, examples[i].y + TILE_SIZE / 2,
                            TILE_SIZE, TILE_SIZE);
        }
    }

    gfx_SwapDraw();
}

void graphics_screen_stats(uint24_t games_played, uint24_t current_streak, uint24_t max_streak, uint16_t *guess_counts,
                           uint8_t current_guesses) {
    gfx_FillScreen(COLOR_BG);
    gfx_SetTextFGColor(COLOR_TEXT);
    gfx_SetTextScale(1, 1);
    const uint8_t numbers_y = 40;
    const struct {
        uint24_t center_x;
        uint8_t y;
        const char *str;
    } text[] = {
            {LCD_WIDTH / 2, 16, "STATISTICS"},
            {1 * LCD_WIDTH / 5, numbers_y + 24, "Played"},
            {2 * LCD_WIDTH / 5, numbers_y + 24, "Win %"},
            {3 * LCD_WIDTH / 5, numbers_y + 24, "Current"},
            {3 * LCD_WIDTH / 5, numbers_y + 34, "Streak"},
            {4 * LCD_WIDTH / 5, numbers_y + 24, "Max"},
            {4 * LCD_WIDTH / 5, numbers_y + 34, "Streak"},
            {LCD_WIDTH / 2, 90, "GUESS DISTRIBUTION"},
            {LCD_WIDTH / 2, 190, "NEXT WORDLE"}
    };
    for(uint8_t i = 0; i < sizeof text / sizeof text[0]; i++) {
        gfx_PrintStringXY(text[i].str, text[i].center_x - gfx_GetStringWidth(text[i].str) / 2, text[i].y);
    }

    uint24_t total_wins = 0;
    for(uint8_t i = 0; i < MAX_GUESSES; i++) {
        total_wins += guess_counts[i];
    }

    gfx_SetTextScale(2, 2);
    gfx_SetTextXY(1 * LCD_WIDTH / 5 - 20, numbers_y);
    gfx_PrintUInt(games_played, 1);
    gfx_SetTextXY(2 * LCD_WIDTH / 5 - 20, numbers_y);
    if(games_played) {
        gfx_PrintUInt(100 * total_wins / games_played, 1);
    } else {
        gfx_PrintChar('-');
    }
    gfx_SetTextXY(3 * LCD_WIDTH / 5 - 20, numbers_y);
    gfx_PrintUInt(current_streak, 1);
    gfx_SetTextXY(4 * LCD_WIDTH / 5 - 20, numbers_y);
    gfx_PrintUInt(max_streak, 1);

    gfx_SetTextScale(1, 1);

    uint24_t max_count = 0;
    for(uint8_t i = 0; i < MAX_GUESSES; i++) {
        if(guess_counts[i] > max_count) {
            max_count = guess_counts[i];
        }
    }

    for(uint8_t i = 0; i < MAX_GUESSES; i++) {
        uint8_t y = 105 + 13 * i;
        const uint24_t base_x = 32;
        const uint24_t max_width = LCD_WIDTH - base_x * 2 - 22;
        const uint8_t base_width = 24;
        uint24_t width = max_count ? base_width + guess_counts[i] * max_width / max_count : base_width;
        gfx_SetTextFGColor(COLOR_TEXT);
        gfx_SetTextXY(base_x, y + 2);
        gfx_PrintUInt(i + 1, 1);

        if(current_guesses == i + 1) {
            gfx_SetColor(COLOR_CORRECT);
        } else {
            gfx_SetColor(COLOR_ABSENT);
        }
        gfx_FillRectangle_NoClip(base_x + 11, y, width, 11);

        gfx_SetTextXY(base_x + width - 6, y + 2);
        gfx_SetTextFGColor(COLOR_WHITE);
        gfx_PrintUInt(guess_counts[i], 1);
    }

    int hours = 23 - time(NULL) / (60 * 60) % 24;
    int minutes = 59 - time(NULL) / 60 % 60;
    int seconds = 59 - time(NULL) % 60;
    gfx_SetTextFGColor(COLOR_TEXT);
    gfx_SetTextScale(2, 2);
    gfx_SetTextXY(108, 210);
    gfx_PrintUInt(hours, 2);
    gfx_PrintChar(':');
    gfx_PrintUInt(minutes, 2);
    gfx_PrintChar(':');
    gfx_PrintUInt(seconds, 2);

    gfx_SwapDraw();
}

void disp_toggle(bool state, uint8_t y, bool highlighted) {
    const uint8_t radius = 6;
    const uint8_t length = 12;
    const uint24_t x = LCD_WIDTH - radius * 2 - length - 12;
    gfx_SetColor(state ? COLOR_CORRECT : COLOR_ABSENT);
    gfx_FillRectangle_NoClip(x + radius + 1, y, length + 2, radius * 2 + 1);
    gfx_FillCircle_NoClip(x + radius, y + radius, radius);
    gfx_FillCircle_NoClip(x + radius + length, y + radius, radius);
    gfx_SetColor(COLOR_TEXT);
    gfx_FillCircle_NoClip(x + radius + (state ? length : 0), y + radius, radius - 1);
    if(highlighted) {
        gfx_SetColor(COLOR_WHITE);
        gfx_PrintStringXY(">", x - gfx_GetCharWidth('>') - 1, y + radius - 4);
        gfx_PrintStringXY("<", x + radius * 2 + length + 3, y + radius - 4);
    }
}

void graphics_screen_settings(uint8_t settings, uint8_t selection, uint24_t day) {
    gfx_FillScreen(COLOR_BG);

    gfx_SetTextScale(1, 1);
    gfx_SetTextFGColor(COLOR_TEXT);

    const char *text_settings = "SETTINGS";
    gfx_PrintStringXY(text_settings, (LCD_WIDTH - gfx_GetStringWidth(text_settings)) / 2, 16);

    const struct {
        const char *text;
        const char *description;
    } items[] = {
            {"Hard Mode", "Must use revealed hints in future guesses"},
            {"Dark Theme", ""},
            {"Color Blind Mode", "High contrast colors"},
    };

    const uint24_t x = 4;


    for(uint8_t i = 0; i < sizeof items / sizeof items[0]; i++) {
        uint8_t y = 50 + 45 * i;
        gfx_SetTextFGColor(COLOR_TEXT);
        gfx_SetTextScale(2, 2);
        gfx_PrintStringXY(items[i].text, x, y);
        gfx_SetTextFGColor(COLOR_ICONS);
        gfx_SetTextScale(1, 1);
        gfx_PrintStringXY(items[i].description, x, y + 20);
        gfx_SetColor(COLOR_ICONS);
        gfx_HorizLine_NoClip(x, y + 35, LCD_WIDTH - 2 * x);
        disp_toggle((settings >> i) & 1, y, i == selection);
    }

    gfx_SetTextFGColor(COLOR_ICONS);
    gfx_SetTextXY(LCD_WIDTH - 40, LCD_HEIGHT - 20);
    gfx_PrintChar('#');
    gfx_PrintUInt(day, 1);

    gfx_PrintStringXY(COMMIT, LCD_WIDTH - gfx_GetStringWidth(COMMIT) - 8, LCD_HEIGHT - 10);

    gfx_SwapDraw();
}

void graphics_set_palette(uint8_t settings) {
    if(settings & SETTING_DARK) {
        gfx_SetPalette(palette_dark, sizeof palette_dark, 0);
    } else {
        gfx_SetPalette(palette_light, sizeof palette_light, 0);
    }
    if(settings & SETTING_CONTRAST) {
        gfx_palette[COLOR_CORRECT] = gfx_RGBTo1555(0xf5, 0x79, 0x3a);
        gfx_palette[COLOR_PRESENT] = gfx_RGBTo1555(0x85, 0xc0, 0xf9);
    }
}
