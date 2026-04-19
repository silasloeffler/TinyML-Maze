#ifndef GAME_H
#define GAME_H

/**
 * @file game.h
 * @brief Game-state helpers for player movement and level progression.
 * @author Silas Loeffler
 */

#include <stdint.h>

void Game_ResetPlayerToLevelStart(int level_index, int* player_x, int* player_y);
void Game_ProcessMove(int class_index,
                      int* player_x,
                      int* player_y,
                      int* current_level,
                      uint8_t* show_congrats,
                      uint32_t* level_finish_time,
                      uint32_t now_tick);

#endif
