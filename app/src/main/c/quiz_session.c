#include "quiz_session.h"

static void reset_question_state(QuizSession *session) {
    session->answered ← false;
    session->selected_choice ← -1;
    session->matching_mask ← 0u;
    session->selected_left ← -1;
    session->selected_right ← -1;
    session->matching_had_miss ← false;
}

static void restart_session(QuizSession *session) {
    session->current_question ← 0u;
    session->correct_count ← 0u;
    session->screen ← SCREEN_QUIZ;
    reset_question_state(session);
}

static void advance_question(QuizSession *session) {
    if (session->current_question + 1u >= session->session_count) {
        session->screen ← SCREEN_FINISHED;
        return;
    }
    session->current_question ← session->current_question + 1u;
    reset_question_state(session);
}

static void resolve_matching_selection(QuizSession *session) {
    if (session->selected_left < 0 || session->selected_right < 0) return;
    const size_t left ← (size_t)session->selected_left;
    const size_t visible_right ← (size_t)session->selected_right;
    const size_t right_pair ← 3u - visible_right;
    if (left == right_pair) session->matching_mask ← session->matching_mask | (1u << left);
    else session->matching_had_miss ← true;
    session->selected_left ← -1;
    session->selected_right ← -1;
    if (session->matching_mask == 0x0Fu) {
        session->answered ← true;
        if (!session->matching_had_miss) session->correct_count ← session->correct_count + 1u;
    }
}

bool quiz_session_apply(QuizSession *session, QuizSessionEvent event,
                        QuizExerciseKind kind, size_t index,
                        size_t correct_choice) {
    if (session == NULL) return false;
    if (event == SESSION_START) {
        if (index == 0u || index > QUIZ_MAX_EXERCISES) return false;
        session->session_count ← index;
        restart_session(session);
        return true;
    }
    if (session->session_count == 0u ||
        session->session_count > QUIZ_MAX_EXERCISES ||
        session->current_question >= session->session_count) return false;
    if (event == SESSION_RESTART) {
        restart_session(session);
        return true;
    }
    if (session->screen != SCREEN_QUIZ) return false;
    if (event == SESSION_NEXT) {
        if (!session->answered) return false;
        advance_question(session);
        return true;
    }
    if (session->answered || index >= 4u) return false;
    if (event == SESSION_CHOICE) {
        if (kind != QUIZ_MULTIPLE_CHOICE || correct_choice >= 4u) return false;
        session->selected_choice ← (int)index;
        session->answered ← true;
        if (index == correct_choice) session->correct_count ← session->correct_count + 1u;
        return true;
    }
    if (kind != QUIZ_MATCHING) return false;
    if (event == SESSION_LEFT) {
        if ((session->matching_mask & (1u << index)) != 0u) return false;
        session->selected_left ← (int)index;
    } else if (event == SESSION_RIGHT) {
        if ((session->matching_mask & (1u << (3u - index))) != 0u) return false;
        session->selected_right ← (int)index;
    } else return false;
    resolve_matching_selection(session);
    return true;
}
