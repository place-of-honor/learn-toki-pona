#include "quiz_session.h"
#include <string.h>
/* Exact original named functions from native_main.c at ef55a23; logging is an
   omitted host effect. This file is a test oracle, never linked into the app. */
#define LOG_INFO(...) ((void)0)
typedef struct {
    size_t session_count;
    size_t current_question;
    size_t correct_count;
    bool answered;
    int selected_choice;
    unsigned matching_mask;
    int selected_left;
    int selected_right;
    bool matching_had_miss;
    ScreenState screen;
    bool redraw;
} AppContext;
static void reset_question_state(AppContext *context) {
    context->answered = false;
    context->selected_choice = -1;
    context->matching_mask = 0u;
    context->selected_left = -1;
    context->selected_right = -1;
    context->matching_had_miss = false;
}

static void restart_session(AppContext *context) {
    context->current_question = 0u;
    context->correct_count = 0u;
    context->screen = SCREEN_QUIZ;
    reset_question_state(context);
    context->redraw = true;
}

static void advance_question(AppContext *context) {
    if (context->current_question + 1u >=
        context->session_count) {
        context->screen = SCREEN_FINISHED;
        context->redraw = true;
        return;
    }

    ++context->current_question;
    LOG_INFO("advanced to question %zu/%zu",
             context->current_question + 1u,
             context->session_count);
    reset_question_state(context);
    context->redraw = true;
}

static void resolve_matching_selection(
    AppContext *context) {
    if (context->selected_left < 0 ||
        context->selected_right < 0) {
        return;
    }

    const size_t left =
        (size_t)context->selected_left;
    const size_t visible_right =
        (size_t)context->selected_right;
    const size_t right_pair =
        3u - visible_right;

    if (left == right_pair) {
        context->matching_mask |= 1u << left;
    } else {
        context->matching_had_miss = true;
    }

    context->selected_left = -1;
    context->selected_right = -1;

    if (context->matching_mask == 0x0Fu) {
        context->answered = true;
        if (!context->matching_had_miss) {
            ++context->correct_count;
        }
    }
}
