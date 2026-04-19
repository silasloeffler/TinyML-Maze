#ifndef DISPLAY_H
#define DISPLAY_H

/**
 * @file display.h
 * @brief OLED rendering helpers for score bars and maze/game screens.
 * @author Silas Loeffler
 */

#include <stdint.h>

void Display_UpdateScores(int8_t s1, int8_t s2, int8_t s3, int winner);
void Display_DrawWelcomeScreen(void);
void Display_DrawLevelCompleteScreen(void);
void Display_DrawFinalCongratsScreen(void);
void Display_DrawGameScreen(int current_level, int player_x, int player_y, uint8_t show_congrats);

#endif
