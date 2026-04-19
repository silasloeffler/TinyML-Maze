/**
 * @file game.c
 * @brief Game logic for player movement and level transitions.
 * @author Silas Loeffler
 */

#include "game.h"
#include "config.h"
#include "maze_levels.h"
#include <stddef.h>

void Game_ResetPlayerToLevelStart(int level_index, int* player_x, int* player_y) {
    if (player_x == NULL || player_y == NULL) {
        return;
    }

    if (level_index < 0 || level_index >= (int)LEVEL_COUNT) {
        level_index = 0;
    }

    *player_x = (int)level_start_x[level_index];
    *player_y = (int)level_start_y[level_index];
}

void Game_ProcessMove(int class_index,
                      int* player_x,
                      int* player_y,
                      int* current_level,
                      uint8_t* show_congrats,
                      uint32_t* level_finish_time,
                      uint32_t now_tick) {
    if (player_x == NULL || player_y == NULL || current_level == NULL || show_congrats == NULL || level_finish_time == NULL) {
        return;
    }

    int next_x = *player_x;
    int next_y = *player_y;
    int target_level = *current_level;

    if (class_index == CLASS_DOWN) {
        next_y += 1;
    } else if (class_index == CLASS_LEFT) {
        next_x -= 1;
    } else if (class_index == CLASS_RIGHT) {
        next_x += 1;
    } else if (class_index == CLASS_UP) {
        next_y -= 1;
    } else {
        return;
    }

    if (next_x < 0 || next_x >= MAZE_WIDTH || next_y < 0 || next_y >= MAZE_HEIGHT) {
        return;
    }

    uint8_t tile = levels[*current_level][next_y][next_x];
    if (tile == 0U) {
        *player_x = next_x;
        *player_y = next_y;
        return;
    }

    if (tile == 2U) {
        *level_finish_time = now_tick;
        target_level = *current_level + 1;
        if (target_level >= (int)LEVEL_COUNT) {
            target_level = 0;
            *show_congrats = 1U;
        }

        *current_level = target_level;
        Game_ResetPlayerToLevelStart(*current_level, player_x, player_y);
    }
}
