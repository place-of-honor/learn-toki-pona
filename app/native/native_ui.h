#ifndef TOKI_PONA_NATIVE_UI_H
#define TOKI_PONA_NATIVE_UI_H

#include <stdbool.h>
#include <stddef.h>

#include "canvas.h"
#include "exercise_model.h"

struct native_ui_state {
    size_t exercise_index;
    struct exercise_progress progress;
    int selected_left;
    int selected_right;
    int last_option;
    bool last_option_valid;
    bool match_error;
    bool completion_counted;
    unsigned correct_exercises;
    unsigned review_exercises;
};

void native_ui_initialize(struct native_ui_state *state);
void native_ui_draw(struct native_ui_state *state, struct canvas *canvas);
void native_ui_handle_tap(
    struct native_ui_state *state,
    int width,
    int height,
    float x,
    float y
);

#endif
