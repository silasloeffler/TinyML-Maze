#ifndef UI_ASSETS_H
#define UI_ASSETS_H

/**
 * @file ui_assets.h
 * @brief Shared OLED font glyphs and text drawing helpers.
 * @author Silas Loeffler
 */

#include "ssd1306.h"
#include <stdint.h>

static const uint8_t kDigitFont4x8[10][4] = {
    {126, 129, 129, 126},
    {8, 4, 2, 255},
    {226, 145, 137, 134},
    {130, 137, 137, 118},
    {66, 137, 137, 118},
    {15, 8, 252, 8},
    {79, 137, 137, 113},
    {126, 137, 137, 114},
    {1, 241, 9, 7},
    {118, 137, 137, 118}
};

static inline void DrawChar5x7(uint8_t x, uint8_t y, char ch) {
    static const uint8_t glyph_space[7] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    static const uint8_t glyph_A[7] = {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11};
    static const uint8_t glyph_C[7] = {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E};
    static const uint8_t glyph_D[7] = {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E};
    static const uint8_t glyph_E[7] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F};
    static const uint8_t glyph_F[7] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10};
    static const uint8_t glyph_G[7] = {0x0E,0x11,0x10,0x17,0x11,0x11,0x0E};
    static const uint8_t glyph_H[7] = {0x11,0x11,0x11,0x1F,0x11,0x11,0x11};
    static const uint8_t glyph_I[7] = {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E};
    static const uint8_t glyph_L[7] = {0x10,0x10,0x10,0x10,0x10,0x10,0x1F};
    static const uint8_t glyph_M[7] = {0x11,0x1B,0x15,0x11,0x11,0x11,0x11};
    static const uint8_t glyph_N[7] = {0x11,0x19,0x1D,0x15,0x13,0x11,0x11};
    static const uint8_t glyph_O[7] = {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E};
    static const uint8_t glyph_P[7] = {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10};
    static const uint8_t glyph_R[7] = {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11};
    static const uint8_t glyph_S[7] = {0x0E,0x11,0x10,0x0E,0x01,0x11,0x0E};
    static const uint8_t glyph_T[7] = {0x1F,0x04,0x04,0x04,0x04,0x04,0x04};
    static const uint8_t glyph_V[7] = {0x11,0x11,0x11,0x11,0x0A,0x0A,0x04};
    static const uint8_t glyph_Y[7] = {0x11,0x11,0x11,0x0A,0x04,0x04,0x04};
    static const uint8_t glyph_excl[7] = {0x04,0x04,0x04,0x04,0x00,0x00,0x04};
    static const uint8_t glyph_dot[7]  = {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C};

    const uint8_t* glyph = glyph_space;

    switch (ch) {
    case 'A': glyph = glyph_A; break;
    case 'C': glyph = glyph_C; break;
    case 'D': glyph = glyph_D; break;
    case 'E': glyph = glyph_E; break;
    case 'F': glyph = glyph_F; break;
    case 'G': glyph = glyph_G; break;
    case 'H': glyph = glyph_H; break;
    case 'I': glyph = glyph_I; break;
    case 'L': glyph = glyph_L; break;
    case 'M': glyph = glyph_M; break;
    case 'N': glyph = glyph_N; break;
    case 'O': glyph = glyph_O; break;
    case 'P': glyph = glyph_P; break;
    case 'R': glyph = glyph_R; break;
    case 'S': glyph = glyph_S; break;
    case 'T': glyph = glyph_T; break;
    case 'V': glyph = glyph_V; break;
    case 'Y': glyph = glyph_Y; break;
    case '!': glyph = glyph_excl; break;
    case '.': glyph = glyph_dot; break;
    case ' ': glyph = glyph_space; break;
    default:  glyph = glyph_space; break;
    }

    for (uint8_t row = 0U; row < 7U; row++) {
        uint8_t bits = glyph[row];
        for (uint8_t col = 0U; col < 5U; col++) {
            if ((bits >> (4U - col)) & 0x01U) {
                ssd1306_DrawPixel((uint8_t)(x + col), (uint8_t)(y + row), White);
            }
        }
    }
}

#endif
