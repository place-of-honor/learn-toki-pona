#include "quiz_session.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../../qualification/c-session/original-policy.inc"

static QuizSession session;
static AppContext original;
static unsigned sequence;

static bool original_event(QuizSessionEvent event, QuizExerciseKind kind,
                           size_t index, size_t correct_choice) {
    if (event == SESSION_START) {
        if (index == 0u || index > QUIZ_MAX_EXERCISES) return false;
        original.session_count = index;
        restart_session(&original);
        return true;
    }
    if (original.session_count == 0u ||
        original.current_question >= original.session_count) return false;
    if (event == SESSION_RESTART) { restart_session(&original); return true; }
    if (original.screen != SCREEN_QUIZ) return false;
    if (event == SESSION_NEXT) {
        if (!original.answered) return false;
        advance_question(&original);
        return true;
    }
    if (original.answered || index >= 4u) return false;
    if (event == SESSION_CHOICE) {
        if (kind != QUIZ_MULTIPLE_CHOICE || correct_choice >= 4u) return false;
        /* Original handle_input condition/action after the rectangle hit. */
        original.selected_choice = (int)index;
        original.answered = true;
        const bool was_correct = index == correct_choice;
        if (was_correct) ++original.correct_count;
        original.redraw = true;
        return true;
    }
    if (kind != QUIZ_MATCHING) return false;
    if (event == SESSION_LEFT &&
        (original.matching_mask & (1u << index)) == 0u) {
        original.selected_left = (int)index;
    } else if (event == SESSION_RIGHT &&
               (original.matching_mask & (1u << (3u - index))) == 0u) {
        original.selected_right = (int)index;
    } else return false;
    resolve_matching_selection(&original);
    original.redraw = true;
    return true;
}

static void same_state(void) {
#define SAME(field) assert(session.field == original.field)
    SAME(session_count); SAME(current_question); SAME(correct_count);
    SAME(answered); SAME(selected_choice); SAME(matching_mask);
    SAME(selected_left); SAME(selected_right); SAME(matching_had_miss);
    SAME(screen);
#undef SAME
}

static void step(QuizSessionEvent event, QuizExerciseKind kind,
                 size_t index, size_t correct_choice) {
    const bool expected = original_event(event, kind, index, correct_choice);
    const bool accepted = quiz_session_apply(&session, event, kind, index,
                                             correct_choice);
    assert(accepted == expected);
    same_state();
    printf("%u\t%d\t%zu\t%zu\t%d\t%zu\t%zu\t%zu\t%d\t%d\t%d\t%u\t%d\t%d\t%d\n",
           sequence++, (int)event, index, correct_choice, accepted ? 1 : 0,
           session.session_count, session.current_question, session.correct_count,
           (int)session.screen, session.answered ? 1 : 0, session.selected_choice,
           session.matching_mask, session.selected_left, session.selected_right,
           session.matching_had_miss ? 1 : 0);
}

static void completed_matching(bool miss_first, bool right_first) {
    if (miss_first) {
        step(SESSION_LEFT, QUIZ_MATCHING, 0u, 0u);
        step(SESSION_RIGHT, QUIZ_MATCHING, 0u, 0u);
        assert(session.matching_had_miss && session.matching_mask == 0u);
    }
    for (size_t left = 0u; left < 4u; ++left) {
        if (right_first) {
            step(SESSION_RIGHT, QUIZ_MATCHING, 3u - left, 0u);
            step(SESSION_LEFT, QUIZ_MATCHING, left, 0u);
        } else {
            step(SESSION_LEFT, QUIZ_MATCHING, left, 0u);
            /* Repeated selection of an unmatched row is the original behavior. */
            step(SESSION_LEFT, QUIZ_MATCHING, left, 0u);
            step(SESSION_RIGHT, QUIZ_MATCHING, 3u - left, 0u);
        }
        /* Completed pairs and their reversed visible right row reject repeats. */
        step(SESSION_LEFT, QUIZ_MATCHING, left, 0u);
        step(SESSION_RIGHT, QUIZ_MATCHING, 3u - left, 0u);
    }
    assert(session.answered && session.matching_mask == 0x0Fu);
}

int main(int argc, char **argv) {
#ifdef SESSION_LUA_POLICY_TEST_INPUT
    /* Test-only semantic mutation input; the native app uses embedded bytes. */
    if (argc == 2) {
        FILE *input = fopen(argv[1], "rb");
        assert(input != NULL && fseek(input, 0, SEEK_END) == 0);
        const long length = ftell(input);
        assert(length > 0 && length <= 131072 && fseek(input, 0, SEEK_SET) == 0);
        char *source = malloc((size_t)length);
        assert(source != NULL && fread(source, 1u, (size_t)length, input) == (size_t)length);
        assert(fclose(input) == 0);
        assert(quiz_session_policy_load(source, (size_t)length));
        free(source);
    } else assert(argc == 1);
#else
    (void)argv;
    assert(argc == 1);
#endif
    puts("sequence\tevent\tindex\tcorrect\taccepted\tlength\tquestion\tscore\tscreen\tanswered\tchoice\tmask\tleft\tright\tmiss");
    step(SESSION_START, QUIZ_MULTIPLE_CHOICE, 0u, 0u);
    step(SESSION_START, QUIZ_MULTIPLE_CHOICE, QUIZ_MAX_EXERCISES + 1u, 0u);
    step(SESSION_START, QUIZ_MULTIPLE_CHOICE, 2u, 0u);
    step(SESSION_NEXT, QUIZ_MULTIPLE_CHOICE, 0u, 0u);
    step(SESSION_CHOICE, QUIZ_MULTIPLE_CHOICE, 0u, 0u);
    assert(session.correct_count == 1u && session.answered);
    step(SESSION_CHOICE, QUIZ_MULTIPLE_CHOICE, 0u, 0u);
    step(SESSION_NEXT, QUIZ_MULTIPLE_CHOICE, 0u, 0u);
    assert(session.current_question == 1u && !session.answered);
    step(SESSION_CHOICE, QUIZ_MULTIPLE_CHOICE, 1u, 0u);
    assert(session.correct_count == 1u && session.selected_choice == 1);
    step(SESSION_NEXT, QUIZ_MULTIPLE_CHOICE, 0u, 0u);
    assert(session.screen == SCREEN_FINISHED && session.current_question == 1u);
    step(SESSION_CHOICE, QUIZ_MULTIPLE_CHOICE, 0u, 0u);
    /* Every tap on the finished native screen maps to this restart event. */
    step(SESSION_RESTART, QUIZ_MULTIPLE_CHOICE, 0u, 0u);
    assert(session.screen == SCREEN_QUIZ && session.correct_count == 0u);
    step(SESSION_RESTART, QUIZ_MULTIPLE_CHOICE, 0u, 0u);

    for (size_t correct = 0u; correct < 4u; ++correct) {
        for (size_t chosen = 0u; chosen < 4u; ++chosen) {
            step(SESSION_START, QUIZ_MULTIPLE_CHOICE, 1u, 0u);
            step(SESSION_CHOICE, QUIZ_MULTIPLE_CHOICE, chosen, correct);
            assert(session.correct_count == (chosen == correct ? 1u : 0u));
            step(SESSION_NEXT, QUIZ_MULTIPLE_CHOICE, 0u, 0u);
        }
    }
    for (unsigned mode = 0u; mode < 4u; ++mode) {
        step(SESSION_START, QUIZ_MATCHING, 1u, 0u);
        completed_matching((mode & 1u) != 0u, (mode & 2u) != 0u);
        assert(session.correct_count == ((mode & 1u) != 0u ? 0u : 1u));
        step(SESSION_NEXT, QUIZ_MATCHING, 0u, 0u);
        step(SESSION_RESTART, QUIZ_MATCHING, 0u, 0u);
        assert(session.matching_mask == 0u && !session.matching_had_miss);
    }
    step(SESSION_START, QUIZ_MULTIPLE_CHOICE, 12u, 0u);
    for (size_t question = 0u; question < 12u; ++question) {
        step(SESSION_CHOICE, QUIZ_MULTIPLE_CHOICE, question % 4u, question % 4u);
        step(SESSION_NEXT, QUIZ_MULTIPLE_CHOICE, 0u, 0u);
    }
    assert(session.screen == SCREEN_FINISHED && session.correct_count == 12u);
    step(SESSION_RESTART, QUIZ_MULTIPLE_CHOICE, 0u, 0u);
    step(SESSION_CHOICE, QUIZ_MULTIPLE_CHOICE, 4u, 0u);
    step(SESSION_CHOICE, QUIZ_MULTIPLE_CHOICE, 0u, 4u);
    step(SESSION_CHOICE, QUIZ_MATCHING, 0u, 0u);
    step(SESSION_LEFT, QUIZ_MULTIPLE_CHOICE, 0u, 0u);
    step(SESSION_RIGHT, QUIZ_MATCHING, 4u, 0u);
    step((QuizSessionEvent)99, QUIZ_MATCHING, 0u, 0u);
    puts("PASS original source oracle: every field, navigation, score, matching and rejection trace");
    return 0;
}
