#include "canvas.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static int clamp_int(int value, int minimum, int maximum) {
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

uint16_t rgb565(unsigned red, unsigned green, unsigned blue) {
    return (uint16_t)(((red & 0xf8u) << 8) | ((green & 0xfcu) << 3) | (blue >> 3));
}

struct rectangle rectangle_make(int left, int top, int right, int bottom) {
    struct rectangle rectangle = {left, top, right, bottom};
    return rectangle;
}

bool rectangle_contains(struct rectangle rectangle, float x, float y) {
    return x >= (float)rectangle.left && x < (float)rectangle.right &&
        y >= (float)rectangle.top && y < (float)rectangle.bottom;
}

void canvas_fill_rectangle(
    struct canvas *canvas,
    struct rectangle rectangle,
    uint16_t color
) {
    int left = clamp_int(rectangle.left, 0, canvas->width);
    int right = clamp_int(rectangle.right, 0, canvas->width);
    int top = clamp_int(rectangle.top, 0, canvas->height);
    int bottom = clamp_int(rectangle.bottom, 0, canvas->height);
    int y;

    for (y = top; y < bottom; y += 1) {
        uint16_t *row = canvas->pixels + y * canvas->stride;
        int x;
        for (x = left; x < right; x += 1) {
            row[x] = color;
        }
    }
}

void canvas_outline_rectangle(
    struct canvas *canvas,
    struct rectangle rectangle,
    int thickness,
    uint16_t color
) {
    canvas_fill_rectangle(
        canvas,
        rectangle_make(rectangle.left, rectangle.top, rectangle.right, rectangle.top + thickness),
        color
    );
    canvas_fill_rectangle(
        canvas,
        rectangle_make(rectangle.left, rectangle.bottom - thickness, rectangle.right, rectangle.bottom),
        color
    );
    canvas_fill_rectangle(
        canvas,
        rectangle_make(rectangle.left, rectangle.top, rectangle.left + thickness, rectangle.bottom),
        color
    );
    canvas_fill_rectangle(
        canvas,
        rectangle_make(rectangle.right - thickness, rectangle.top, rectangle.right, rectangle.bottom),
        color
    );
}

static const uint64_t letter_glyphs[26] = {
    UINT64_C(0x4631fc62e), UINT64_C(0x3e317c62f), UINT64_C(0x3a210862e),
    UINT64_C(0x3e318c62f), UINT64_C(0x7c217843f), UINT64_C(0x04217843f),
    UINT64_C(0x7a31e862e), UINT64_C(0x4631fc631), UINT64_C(0x38842108e),
    UINT64_C(0x19294211c), UINT64_C(0x452519531), UINT64_C(0x7c2108421),
    UINT64_C(0x4631ad771), UINT64_C(0x4631cd671), UINT64_C(0x3a318c62e),
    UINT64_C(0x04217c62f), UINT64_C(0x59358c62e), UINT64_C(0x45257c62f),
    UINT64_C(0x3e107043e), UINT64_C(0x10842109f), UINT64_C(0x3a318c631),
    UINT64_C(0x11518c631), UINT64_C(0x2ab5ac631), UINT64_C(0x462a22a31),
    UINT64_C(0x108422a31), UINT64_C(0x7c222221f),
};

static const uint64_t digit_glyphs[10] = {
    UINT64_C(0x3a33ae62e), UINT64_C(0x3884210c4), UINT64_C(0x7c444422e),
    UINT64_C(0x3e107420f), UINT64_C(0x211f4a988), UINT64_C(0x3e107843f),
    UINT64_C(0x3a317842e), UINT64_C(0x08422221f), UINT64_C(0x3a317462e),
    UINT64_C(0x3a10f462e),
};

static uint64_t glyph_bits(char character) {
    if (character >= 'a' && character <= 'z') {
        character = (char)(character - 'a' + 'A');
    }
    if (character >= 'A' && character <= 'Z') {
        return letter_glyphs[character - 'A'];
    }
    if (character >= '0' && character <= '9') {
        return digit_glyphs[character - '0'];
    }
    switch (character) {
        case '.': return UINT64_C(0x18c000000);
        case '/': return UINT64_C(0x044222110);
        case '-': return UINT64_C(0x0000f8000);
        case '_': return UINT64_C(0x7c0000000);
        case '[': return UINT64_C(0x38421084e);
        case ']': return UINT64_C(0x39084210e);
        case ':': return UINT64_C(0x00c6018c0);
        case ' ': return UINT64_C(0);
        default: return UINT64_C(0x10044422e);
    }
}

static void draw_glyph(
    struct canvas *canvas,
    int x,
    int y,
    int scale,
    char character,
    uint16_t color
) {
    uint64_t bits = glyph_bits(character);
    int row;
    for (row = 0; row < 7; row += 1) {
        int column;
        for (column = 0; column < 5; column += 1) {
            unsigned bit = (unsigned)(row * 5 + column);
            if ((bits & (UINT64_C(1) << bit)) != 0) {
                canvas_fill_rectangle(
                    canvas,
                    rectangle_make(
                        x + column * scale,
                        y + row * scale,
                        x + (column + 1) * scale,
                        y + (row + 1) * scale
                    ),
                    color
                );
            }
        }
    }
}

static int text_width(const char *text, int scale) {
    size_t length = strlen(text);
    return length == 0 ? 0 : (int)(length * (size_t)(6 * scale) - (size_t)scale);
}

void canvas_draw_text(
    struct canvas *canvas,
    int x,
    int y,
    int scale,
    const char *text,
    uint16_t color
) {
    while (*text != '\0') {
        draw_glyph(canvas, x, y, scale, *text, color);
        x += 6 * scale;
        text += 1;
    }
}

void canvas_draw_text_centered(
    struct canvas *canvas,
    struct rectangle area,
    int scale,
    const char *text,
    uint16_t color
) {
    int width = text_width(text, scale);
    int height = 7 * scale;
    canvas_draw_text(
        canvas,
        area.left + (area.right - area.left - width) / 2,
        area.top + (area.bottom - area.top - height) / 2,
        scale,
        text,
        color
    );
}
