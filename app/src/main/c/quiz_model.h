#ifndef TOKI_PONA_QUIZ_MODEL_H
#define TOKI_PONA_QUIZ_MODEL_H

#include <stdbool.h>
#include <stddef.h>

#define QUIZ_MAX_EXERCISES 64u
#define QUIZ_CHOICE_COUNT 4u
#define QUIZ_PAIR_COUNT 4u

typedef enum {
    QUIZ_MULTIPLE_CHOICE = 1,
    QUIZ_MATCHING = 2
} QuizExerciseKind;

typedef struct {
    char *id;
    char *prompt;
    QuizExerciseKind kind;
    char *choices[QUIZ_CHOICE_COUNT];
    size_t choice_count;
    size_t correct_choice_index;
    char *pair_left[QUIZ_PAIR_COUNT];
    char *pair_right[QUIZ_PAIR_COUNT];
    size_t pair_count;
    char *note;
} QuizExercise;

typedef struct {
    QuizExercise exercises[QUIZ_MAX_EXERCISES];
    size_t exercise_count;
} QuizCourse;

/*
 * Parse quizzes.txt in place.
 *
 * The caller owns the text buffer and must keep it alive while using course.
 * The parser replaces line terminators and matching separators with NUL bytes,
 * so all strings in QuizCourse point directly into the caller's buffer.
 */
bool quiz_course_parse(QuizCourse *course, char *text, size_t length,
                       char *error, size_t error_capacity);

#endif
