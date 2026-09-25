#include "native_ui.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct screen_layout {
    int width;
    int height;
    int text_scale;
    struct rectangle choice_option[4];
    struct rectangle word_bank_option[6];
    struct rectangle match_left[4];
    struct rectangle match_right[4];
    struct rectangle next_button;
};

static int clamp_int(int value, int minimum, int maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static struct screen_layout calculate_layout(int width, int height) {
    struct screen_layout layout;
    int margin = clamp_int(width / 20, 12, 60);
    int gap = clamp_int(height / 50, 10, 36);
    int option_height = clamp_int(height / 11, 58, 140);
    int row;

    memset(&layout, 0, sizeof(layout));
    layout.width = width;
    layout.height = height;
    layout.text_scale = clamp_int(width / 150, 2, 5);

    for (row = 0; row < 4; row += 1) {
        int top = height * 40 / 100 + row * (option_height + gap);
        layout.choice_option[row] = rectangle_make(margin, top, width - margin, top + option_height);
    }

    for (row = 0; row < 4; row += 1) {
        int bank_width = (width - 3 * margin) / 2;
        int column = row % 2;
        int bank_row = row / 2;
        int left = margin + column * (bank_width + margin);
        int top = height * 55 / 100 + bank_row * (option_height + gap);
        layout.word_bank_option[row] = rectangle_make(left, top, left + bank_width, top + option_height);
    }

    for (row = 0; row < 4; row += 1) {
        int column_width = (width - 3 * margin) / 2;
        int top = height * 43 / 100 + row * (option_height + gap);
        layout.match_left[row] = rectangle_make(margin, top, margin + column_width, top + option_height);
        layout.match_right[row] = rectangle_make(
            2 * margin + column_width,
            top,
            width - margin,
            top + option_height
        );
    }

    layout.next_button = rectangle_make(margin, height * 84 / 100, width - margin, height * 94 / 100);
    return layout;
}

static bool matching_right_is_complete(
    const struct exercise *exercise,
    const struct exercise_progress *progress,
    size_t right_index
) {
    size_t left_index;
    for (left_index = 0; left_index < exercise->body.match_pairs.pair_count; left_index += 1) {
        if ((progress->matched_left_mask & (UINT32_C(1) << left_index)) != 0 &&
            exercise->body.match_pairs.correct_right_for_left[left_index] == right_index) {
            return true;
        }
    }
    return false;
}

static void draw_button(
    struct canvas *canvas,
    struct rectangle rectangle,
    int scale,
    const char *label,
    uint16_t fill,
    uint16_t border,
    uint16_t text
) {
    canvas_fill_rectangle(canvas, rectangle, fill);
    canvas_outline_rectangle(canvas, rectangle, clamp_int(scale, 2, 5), border);
    canvas_draw_text_centered(canvas, rectangle, scale, label, text);
}

static void draw_header(
    struct canvas *canvas,
    const struct native_ui_state *state,
    const struct exercise *exercise,
    const struct screen_layout *layout
) {
    char status[64];
    struct rectangle track = rectangle_make(
        layout->width / 12,
        layout->height * 16 / 100,
        layout->width * 11 / 12,
        layout->height * 18 / 100
    );
    struct rectangle filled = track;

    canvas_draw_text_centered(
        canvas,
        rectangle_make(0, layout->height * 3 / 100, layout->width, layout->height * 10 / 100),
        layout->text_scale,
        "TOKI PONA DRILLS",
        rgb565(31, 45, 38)
    );
    snprintf(
        status,
        sizeof(status),
        "LESSON %u   %zu / %zu",
        exercise->lesson_number,
        state->exercise_index + 1,
        exercise_count()
    );
    canvas_draw_text_centered(
        canvas,
        rectangle_make(0, layout->height * 10 / 100, layout->width, layout->height * 15 / 100),
        clamp_int(layout->text_scale - 1, 2, 5),
        status,
        rgb565(92, 105, 98)
    );
    canvas_fill_rectangle(canvas, track, rgb565(218, 222, 216));
    filled.right = track.left +
        (track.right - track.left) * (int)(state->exercise_index + 1) / (int)exercise_count();
    canvas_fill_rectangle(canvas, filled, rgb565(37, 107, 75));
}

static void draw_choose_one(
    struct canvas *canvas,
    const struct native_ui_state *state,
    const struct exercise *exercise,
    const struct screen_layout *layout
) {
    size_t index;
    for (index = 0; index < exercise->body.choose_one.choice_count; index += 1) {
        uint16_t fill = rgb565(255, 253, 248);
        uint16_t border = rgb565(187, 190, 183);
        if (state->progress.complete && index == exercise->body.choose_one.correct_choice_index) {
            fill = rgb565(220, 242, 227);
            border = rgb565(37, 107, 75);
        } else if (state->progress.complete && state->last_option_valid &&
                   (size_t)state->last_option == index) {
            fill = rgb565(252, 228, 222);
            border = rgb565(161, 72, 52);
        }
        draw_button(
            canvas,
            layout->choice_option[index],
            layout->text_scale,
            exercise->body.choose_one.choices[index],
            fill,
            border,
            rgb565(31, 45, 38)
        );
    }
}

static void draw_word_bank(
    struct canvas *canvas,
    const struct native_ui_state *state,
    const struct exercise *exercise,
    const struct screen_layout *layout
) {
    size_t index;
    for (index = 0; index < exercise->body.word_bank.token_count; index += 1) {
        uint16_t fill = rgb565(255, 253, 248);
        uint16_t border = rgb565(187, 190, 183);
        if (state->progress.complete && index == exercise->body.word_bank.correct_token_index) {
            fill = rgb565(220, 242, 227);
            border = rgb565(37, 107, 75);
        } else if (state->progress.complete && state->last_option_valid &&
                   (size_t)state->last_option == index) {
            fill = rgb565(252, 228, 222);
            border = rgb565(161, 72, 52);
        }
        draw_button(
            canvas,
            layout->word_bank_option[index],
            layout->text_scale + 1,
            exercise->body.word_bank.tokens[index],
            fill,
            border,
            rgb565(31, 45, 38)
        );
    }
}

static void draw_matching(
    struct canvas *canvas,
    const struct native_ui_state *state,
    const struct exercise *exercise,
    const struct screen_layout *layout
) {
    size_t index;
    for (index = 0; index < exercise->body.match_pairs.pair_count; index += 1) {
        bool left_done = (state->progress.matched_left_mask & (UINT32_C(1) << index)) != 0;
        bool right_done = matching_right_is_complete(exercise, &state->progress, index);
        uint16_t left_fill = left_done ? rgb565(220, 242, 227) : rgb565(255, 253, 248);
        uint16_t right_fill = right_done ? rgb565(220, 242, 227) : rgb565(255, 253, 248);
        uint16_t left_border = left_done ? rgb565(37, 107, 75) : rgb565(187, 190, 183);
        uint16_t right_border = right_done ? rgb565(37, 107, 75) : rgb565(187, 190, 183);

        if (!left_done && state->selected_left == (int)index) {
            left_fill = rgb565(230, 239, 249);
            left_border = rgb565(55, 91, 135);
        }
        if (!right_done && state->selected_right == (int)index) {
            right_fill = rgb565(230, 239, 249);
            right_border = rgb565(55, 91, 135);
        }

        draw_button(
            canvas,
            layout->match_left[index],
            layout->text_scale,
            exercise->body.match_pairs.left[index],
            left_fill,
            left_border,
            rgb565(31, 45, 38)
        );
        draw_button(
            canvas,
            layout->match_right[index],
            clamp_int(layout->text_scale - 1, 2, 5),
            exercise->body.match_pairs.right[index],
            right_fill,
            right_border,
            rgb565(31, 45, 38)
        );
    }

    if (state->match_error && !state->progress.complete) {
        canvas_draw_text_centered(
            canvas,
            rectangle_make(0, layout->height * 82 / 100, layout->width, layout->height * 90 / 100),
            layout->text_scale,
            "TRY AGAIN",
            rgb565(161, 72, 52)
        );
    }
}

static void draw_feedback(
    struct canvas *canvas,
    const struct native_ui_state *state,
    const struct screen_layout *layout
) {
    uint16_t border = state->progress.had_mistake ? rgb565(161, 72, 52) : rgb565(37, 107, 75);
    canvas_fill_rectangle(
        canvas,
        rectangle_make(0, layout->height * 72 / 100, layout->width, layout->height),
        state->progress.had_mistake ? rgb565(252, 228, 222) : rgb565(220, 242, 227)
    );
    canvas_draw_text_centered(
        canvas,
        rectangle_make(0, layout->height * 74 / 100, layout->width, layout->height * 82 / 100),
        layout->text_scale,
        state->progress.had_mistake ? "SAVED FOR REVIEW" : "CORRECT",
        rgb565(31, 45, 38)
    );
    draw_button(
        canvas,
        layout->next_button,
        layout->text_scale,
        "NEXT",
        border,
        border,
        rgb565(255, 255, 255)
    );
}

void native_ui_draw(struct native_ui_state *state, struct canvas *canvas) {
    const struct exercise *exercise = exercise_at(state->exercise_index);
    struct screen_layout layout = calculate_layout(canvas->width, canvas->height);
    struct rectangle instruction = rectangle_make(0, layout.height * 21 / 100, layout.width, layout.height * 27 / 100);
    struct rectangle prompt = rectangle_make(0, layout.height * 28 / 100, layout.width, layout.height * 38 / 100);

    if (exercise == NULL) return;
    canvas_fill_rectangle(canvas, rectangle_make(0, 0, canvas->width, canvas->height), rgb565(244, 239, 227));
    draw_header(canvas, state, exercise, &layout);
    canvas_draw_text_centered(
        canvas,
        instruction,
        clamp_int(layout.text_scale - 1, 2, 5),
        exercise->instruction,
        rgb565(92, 105, 98)
    );

    if (exercise->kind == EXERCISE_CHOOSE_ONE) {
        canvas_draw_text_centered(canvas, prompt, layout.text_scale + 1, exercise->body.choose_one.prompt, rgb565(31, 45, 38));
        draw_choose_one(canvas, state, exercise, &layout);
    } else if (exercise->kind == EXERCISE_WORD_BANK) {
        canvas_draw_text_centered(canvas, prompt, layout.text_scale + 1, exercise->body.word_bank.prompt, rgb565(31, 45, 38));
        draw_word_bank(canvas, state, exercise, &layout);
    } else if (exercise->kind == EXERCISE_MATCH_PAIRS) {
        canvas_draw_text_centered(canvas, prompt, layout.text_scale + 1, exercise->body.match_pairs.prompt, rgb565(31, 45, 38));
        draw_matching(canvas, state, exercise, &layout);
    }

    if (state->progress.complete) draw_feedback(canvas, state, &layout);
}

static void reset_exercise(struct native_ui_state *state) {
    exercise_progress_reset(&state->progress);
    state->selected_left = -1;
    state->selected_right = -1;
    state->last_option = -1;
    state->last_option_valid = false;
    state->match_error = false;
    state->completion_counted = false;
}

void native_ui_initialize(struct native_ui_state *state) {
    memset(state, 0, sizeof(*state));
    reset_exercise(state);
}

static void count_completion(struct native_ui_state *state) {
    if (!state->progress.complete || state->completion_counted) return;
    if (state->progress.had_mistake) state->review_exercises += 1;
    else state->correct_exercises += 1;
    state->completion_counted = true;
}

static void advance_exercise(struct native_ui_state *state) {
    state->exercise_index = (state->exercise_index + 1) % exercise_count();
    reset_exercise(state);
}

static void submit_option(
    struct native_ui_state *state,
    const struct exercise *exercise,
    size_t index
) {
    state->last_option = (int)index;
    state->last_option_valid = true;
    (void)exercise_submit_option(exercise, &state->progress, index);
    count_completion(state);
}

static void resolve_match_if_ready(
    struct native_ui_state *state,
    const struct exercise *exercise
) {
    bool correct;
    if (state->selected_left < 0 || state->selected_right < 0) return;
    correct = exercise_submit_match(
        exercise,
        &state->progress,
        (size_t)state->selected_left,
        (size_t)state->selected_right
    );
    state->match_error = !correct;
    state->selected_left = -1;
    state->selected_right = -1;
    count_completion(state);
}

void native_ui_handle_tap(
    struct native_ui_state *state,
    int width,
    int height,
    float x,
    float y
) {
    const struct exercise *exercise = exercise_at(state->exercise_index);
    struct screen_layout layout = calculate_layout(width, height);
    size_t index;

    if (exercise == NULL) return;
    if (state->progress.complete) {
        if (rectangle_contains(layout.next_button, x, y)) advance_exercise(state);
        return;
    }

    if (exercise->kind == EXERCISE_CHOOSE_ONE) {
        for (index = 0; index < exercise->body.choose_one.choice_count; index += 1) {
            if (rectangle_contains(layout.choice_option[index], x, y)) {
                submit_option(state, exercise, index);
                return;
            }
        }
    } else if (exercise->kind == EXERCISE_WORD_BANK) {
        for (index = 0; index < exercise->body.word_bank.token_count; index += 1) {
            if (rectangle_contains(layout.word_bank_option[index], x, y)) {
                submit_option(state, exercise, index);
                return;
            }
        }
    } else if (exercise->kind == EXERCISE_MATCH_PAIRS) {
        state->match_error = false;
        for (index = 0; index < exercise->body.match_pairs.pair_count; index += 1) {
            if (rectangle_contains(layout.match_left[index], x, y)) {
                if ((state->progress.matched_left_mask & (UINT32_C(1) << index)) == 0) {
                    state->selected_left = (int)index;
                    resolve_match_if_ready(state, exercise);
                }
                return;
            }
        }
        for (index = 0; index < exercise->body.match_pairs.pair_count; index += 1) {
            if (rectangle_contains(layout.match_right[index], x, y)) {
                if (!matching_right_is_complete(exercise, &state->progress, index)) {
                    state->selected_right = (int)index;
                    resolve_match_if_ready(state, exercise);
                }
                return;
            }
        }
    }
}
