#ifndef TOKI_PONA_CANVAS_H
#define TOKI_PONA_CANVAS_H

#include <stdbool.h>
#include <stdint.h>

struct rectangle {
    int left;
    int top;
    int right;
    int bottom;
};

struct canvas {
    uint16_t *pixels;
    int width;
    int height;
    int stride;
};

uint16_t rgb565(unsigned red, unsigned green, unsigned blue);
struct rectangle rectangle_make(int left, int top, int right, int bottom);
bool rectangle_contains(struct rectangle rectangle, float x, float y);

void canvas_fill_rectangle(
    struct canvas *canvas,
    struct rectangle rectangle,
    uint16_t color
);
void canvas_outline_rectangle(
    struct canvas *canvas,
    struct rectangle rectangle,
    int thickness,
    uint16_t color
);
void canvas_draw_text(
    struct canvas *canvas,
    int x,
    int y,
    int scale,
    const char *text,
    uint16_t color
);
void canvas_draw_text_centered(
    struct canvas *canvas,
    struct rectangle area,
    int scale,
    const char *text,
    uint16_t color
);

#endif
