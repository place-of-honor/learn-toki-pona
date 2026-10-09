#ifndef TOKI_PONA_QUIZ_SESSION_H
#define TOKI_PONA_QUIZ_SESSION_H

#include "quiz_model.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum { SCREEN_INVALID, SCREEN_QUIZ, SCREEN_FINISHED } ScreenState;
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
} QuizSession;

typedef enum {
    SESSION_START, SESSION_RESTART, SESSION_NEXT,
    SESSION_CHOICE, SESSION_LEFT, SESSION_RIGHT
} QuizSessionEvent;

/* Only admitted native hits enter this pure composition. */
bool quiz_session_apply(QuizSession *session, QuizSessionEvent event,
                        QuizExerciseKind kind, size_t index,
                        size_t correct_choice);

/* Explicit close/reinitialization is the only reset after a policy fault. */
bool quiz_session_policy_load(const void *source, size_t length);
void quiz_session_policy_close(void);
bool quiz_session_policy_faulted(void);
size_t quiz_session_policy_memory_peak(void);
size_t quiz_session_policy_memory_denials(void);

#endif
