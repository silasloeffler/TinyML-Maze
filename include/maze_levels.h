#ifndef MAZE_LEVELS_H
#define MAZE_LEVELS_H

/**
 * @file maze_levels.h
 * @brief Static maze layouts and spawn positions for the voice-controlled game.
 * @author Silas Loeffler
 */

#include <stdint.h>

/* Maze width in tiles. */
#define MAZE_WIDTH   16
/* Maze height in tiles. */
#define MAZE_HEIGHT  8
/* Total number of levels in the campaign. */
#define LEVEL_COUNT  3U

/* Tile values: 0 = path, 1 = wall, 2 = goal. */
static const uint8_t levels[LEVEL_COUNT][MAZE_HEIGHT][MAZE_WIDTH] = {
    {
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 2, 1, 1, 0, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
    },

    {
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1 },
        { 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1 },
        { 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 2, 1 },
        { 1, 0,
            0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1 },
        { 1, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1 },
        { 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
    },
    {
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
        { 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1 },
        { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1 },
        { 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 2, 1, 1, 1 },
        { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
    }
};

/* Starting x-position for each level spawn point. */
static const uint8_t level_start_x[LEVEL_COUNT] = { 6U, 4U, 4U };
/* Starting y-position for each level spawn point. */
static const uint8_t level_start_y[LEVEL_COUNT] = { 2U, 3U, 2U };

#endif
