#include <assert.h>

#include "../app/native/native_ui.h"

static void tap(struct native_ui_state *state, float x, float y) {
    native_ui_handle_tap(state, 1080, 1920, x, y);
}

int main(void) {
    struct native_ui_state state;

    native_ui_initialize(&state);
    assert(state.exercise_index == 0);

    /* First answer in the choose-one exercise: I EAT. */
    tap(&state, 540.0f, 800.0f);
    assert(state.progress.complete);
    assert(!state.progress.had_mistake);
    assert(state.correct_exercises == 1);

    /* NEXT moves to lesson 2's grammar-particle word bank. */
    tap(&state, 540.0f, 1700.0f);
    assert(state.exercise_index == 1);
    assert(!state.progress.complete);

    /* First word-bank tile is E in MI MOKU [ ] KILI. */
    tap(&state, 100.0f, 1100.0f);
    assert(state.progress.complete);
    assert(!state.progress.had_mistake);
    assert(state.correct_exercises == 2);

    tap(&state, 540.0f, 1700.0f);
    assert(state.exercise_index == 2);

    /* Matching: deliberately miss once, then match MOKU E KILI -> EAT FRUIT. */
    tap(&state, 100.0f, 850.0f);
    tap(&state, 700.0f, 850.0f);
    assert(state.match_error);
    assert(state.progress.had_mistake);
    assert(!state.progress.complete);

    tap(&state, 100.0f, 850.0f);
    tap(&state, 700.0f, 1200.0f);
    assert((state.progress.matched_left_mask & 1u) != 0);

    return 0;
}
