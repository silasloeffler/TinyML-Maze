/**
 * @file display.c
 * @brief OLED rendering helpers for the welcome screen, game screen, and score view.
 * @author Silas Loeffler
 */

#include "display.h"
#include "config.h"
#include "maze_levels.h"
#include "ssd1306.h"
#include "ui_assets.h"

static void DrawFilledRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, SSD1306_COLOR color) {
    for (uint8_t yy = y; yy < (uint8_t)(y + h); yy++) {
        for (uint8_t xx = x; xx < (uint8_t)(x + w); xx++) {
            ssd1306_DrawPixel(xx, yy, color);
        }
    }
}

static void DrawHollowRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, SSD1306_COLOR color) {
    if (w == 0U || h == 0U) {
        return;
    }

    for (uint8_t xx = x; xx < (uint8_t)(x + w); xx++) {
        ssd1306_DrawPixel(xx, y, color);
        ssd1306_DrawPixel(xx, (uint8_t)(y + h - 1U), color);
    }

    for (uint8_t yy = y; yy < (uint8_t)(y + h); yy++) {
        ssd1306_DrawPixel(x, yy, color);
        ssd1306_DrawPixel((uint8_t)(x + w - 1U), yy, color);
    }
}

static void DrawText5x7(uint8_t x, uint8_t y, const char* text) {
    if (text == NULL) {
        return;
    }

    uint8_t cursor_x = x;
    while (*text != '\0') {
        DrawChar5x7(cursor_x, y, *text);
        cursor_x = (uint8_t)(cursor_x + 6U);
        text++;
    }
}

static void DrawGoalTile(uint8_t tile_x, uint8_t tile_y) {
    uint8_t px = (uint8_t)(tile_x * TILE_SIZE);
    uint8_t py = (uint8_t)(tile_y * TILE_SIZE);

    for (uint8_t yy = 1U; yy < (uint8_t)(TILE_SIZE - 1U); yy++) {
        for (uint8_t xx = 1U; xx < (uint8_t)(TILE_SIZE - 1U); xx++) {
            if (((xx + yy) & 0x01U) == 0U) {
                ssd1306_DrawPixel((uint8_t)(px + xx), (uint8_t)(py + yy), White);
            }
        }
    }

    DrawHollowRect(px, py, TILE_SIZE, TILE_SIZE, White);
}

static uint8_t ScoreToPercent(int8_t score) {
    int32_t shifted = (int32_t)score + 128;
    if (shifted < 0) {
        shifted = 0;
    }
    if (shifted > 255) {
        shifted = 255;
    }
    return (uint8_t)((shifted * 100) / 255);
}

static void OLED_DrawDigit4x8(uint8_t x, uint8_t y, uint8_t digit) {
    if (digit > 9U) {
        return;
    }

    for (uint8_t col = 0; col < 4U; col++) {
        uint8_t bits = kDigitFont4x8[digit][col];
        for (uint8_t row = 0; row < 8U; row++) {
            SSD1306_COLOR color = ((bits >> row) & 0x01U) ? White : Black;
            ssd1306_DrawPixel((uint8_t)(x + col), (uint8_t)(y + row), color);
        }
    }
}

static void OLED_DrawBarRow(uint8_t y, uint8_t label_digit, uint8_t percent, uint8_t winner_row) {
    uint8_t frame_x1 = OLED_BAR_X;
    uint8_t frame_y1 = y;
    uint8_t frame_x2 = (uint8_t)(OLED_BAR_X + OLED_BAR_WIDTH - 1U);
    uint8_t frame_y2 = (uint8_t)(y + OLED_BAR_HEIGHT - 1U);
    uint8_t fill_w = (uint8_t)((percent * (OLED_BAR_WIDTH - 2U)) / 100U);

    OLED_DrawDigit4x8(OLED_LABEL_X, (uint8_t)(y + 2U), label_digit);

    for (uint8_t xx = frame_x1; xx <= frame_x2; xx++) {
        ssd1306_DrawPixel(xx, frame_y1, White);
        ssd1306_DrawPixel(xx, frame_y2, White);
    }
    for (uint8_t yy = frame_y1; yy <= frame_y2; yy++) {
        ssd1306_DrawPixel(frame_x1, yy, White);
        ssd1306_DrawPixel(frame_x2, yy, White);
    }

    for (uint8_t yy = (uint8_t)(frame_y1 + 1U); yy < frame_y2; yy++) {
        for (uint8_t xx = (uint8_t)(frame_x1 + 1U); xx < (uint8_t)(frame_x1 + 1U + fill_w); xx++) {
            ssd1306_DrawPixel(xx, yy, White);
        }
    }

    if (winner_row) {
        for (uint8_t yy = frame_y1; yy <= frame_y2; yy++) {
            ssd1306_DrawPixel((uint8_t)(frame_x2 - 1U), yy, White);
        }
    }
}

void Display_UpdateScores(int8_t s1, int8_t s2, int8_t s3, int winner) {
    uint8_t p1 = ScoreToPercent(s1);
    uint8_t p2 = ScoreToPercent(s2);
    uint8_t p3 = ScoreToPercent(s3);

    ssd1306_Fill(Black);
    OLED_DrawBarRow(OLED_ROW0_Y, 1U, p1, (uint8_t)(winner == CLASS_DOWN));
    OLED_DrawBarRow(OLED_ROW1_Y, 2U, p2, (uint8_t)(winner == CLASS_LEFT));
    OLED_DrawBarRow(OLED_ROW2_Y, 3U, p3, (uint8_t)(winner == CLASS_RANDOM));
    ssd1306_UpdateScreen();
}

void Display_DrawWelcomeScreen(void) {
    ssd1306_Fill(Black);
    DrawHollowRect(4U, 4U, 120U, 56U, White);
    DrawText5x7(58U, 18U, "HI");
    DrawText5x7(25U, 30U, "SAY SOMETHING");
}

void Display_DrawLevelCompleteScreen(void) {
    ssd1306_Fill(Black);
    DrawHollowRect(4U, 4U, 120U, 56U, White);
    DrawText5x7(37U, 18U, "CONGRATS!");
    DrawText5x7(22U, 30U, "LEVEL COMPLETE");
}

void Display_DrawFinalCongratsScreen(void) {
    ssd1306_Fill(Black);
    DrawHollowRect(4U, 4U, 120U, 56U, White);
    DrawText5x7(37U, 18U, "CONGRATS!");
    DrawText5x7(16U, 30U, "GAME FINISHED...");
}

void Display_DrawGameScreen(int current_level, int player_x, int player_y, uint8_t show_congrats) {
    if (show_congrats) {
        Display_DrawFinalCongratsScreen();
        return;
    }

    ssd1306_Fill(Black);

    for (uint8_t y = 0; y < MAZE_HEIGHT; y++) {
        for (uint8_t x = 0; x < MAZE_WIDTH; x++) {
            uint8_t tile = levels[current_level][y][x];
            if (tile == 1U) {
                DrawFilledRect((uint8_t)(x * TILE_SIZE), (uint8_t)(y * TILE_SIZE), TILE_SIZE, TILE_SIZE, White);
            } else if (tile == 2U) {
                DrawGoalTile(x, y);
            }
        }
    }

    DrawFilledRect(
        (uint8_t)(player_x * TILE_SIZE + 1),
        (uint8_t)(player_y * TILE_SIZE + 1),
        6,
        6,
        White
    );
}
