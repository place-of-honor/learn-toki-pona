#include "quiz_model.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static void set_error(char *error, size_t capacity, const char *message) {
    if (error == NULL || capacity == 0u) {
        return;
    }
    (void)snprintf(error, capacity, "%s", message);
}

static char *next_line(char **cursor) {
    if (cursor == NULL || *cursor == NULL || **cursor == '\0') {
        return NULL;
    }

    char *line = *cursor;
    char *newline = strchr(line, '\n');
    if (newline == NULL) {
        *cursor = line + strlen(line);
    } else {
        *newline = '\0';
        *cursor = newline + 1;
    }

    const size_t length = strlen(line);
    if (length > 0u && line[length - 1u] == '\r') {
        line[length - 1u] = '\0';
    }
    return line;
}

static bool exercise_header(char *line, char **exercise_id) {
    if (line == NULL || !isdigit((unsigned char)line[0])) {
        return false;
    }

    char *cursor = line;
    while (isdigit((unsigned char)*cursor)) {
        ++cursor;
    }
    if (*cursor != '.') {
        return false;
    }
    ++cursor;

    if (!isdigit((unsigned char)*cursor)) {
        return false;
    }
    while (isdigit((unsigned char)*cursor)) {
        ++cursor;
    }
    if (*cursor != ' ' || cursor[1] == '\0') {
        return false;
    }

    *exercise_id = cursor + 1;
    return true;
}

static bool choice_line(char *line, size_t *index, char **text) {
    if (line == NULL ||
        line[0] < 'A' || line[0] > 'D' ||
        line[1] != '.' || line[2] != ' ') {
        return false;
    }
    *index = (size_t)(line[0] - 'A');
    *text = line + 3;
    return true;
}

static bool validate_exercise(QuizExercise *exercise,
                              char *error, size_t error_capacity) {
    if (exercise->id == NULL || exercise->prompt == NULL || exercise->note == NULL) {
        set_error(error, error_capacity, "exercise is missing id, prompt, or note");
        return false;
    }

    if (exercise->choice_count > 0u) {
        if (exercise->choice_count != QUIZ_CHOICE_COUNT ||
            exercise->correct_choice_index >= QUIZ_CHOICE_COUNT) {
            set_error(error, error_capacity,
                      "multiple-choice exercise does not have four choices and one answer");
            return false;
        }
        exercise->kind = QUIZ_MULTIPLE_CHOICE;
        return true;
    }

    if (exercise->pair_count == QUIZ_PAIR_COUNT) {
        exercise->kind = QUIZ_MATCHING;
        return true;
    }

    set_error(error, error_capacity, "exercise is neither four-choice nor four-pair");
    return false;
}

bool quiz_course_parse(QuizCourse *course, char *text, size_t length,
                       char *error, size_t error_capacity) {
    if (course == NULL || text == NULL) {
        set_error(error, error_capacity, "null quiz parser input");
        return false;
    }

    memset(course, 0, sizeof(*course));
    text[length] = '\0';

    char *cursor = text;
    QuizExercise *current = NULL;

    for (char *line = next_line(&cursor);
         line != NULL;
         line = next_line(&cursor)) {
        char *identifier = NULL;
        if (exercise_header(line, &identifier)) {
            if (current != NULL &&
                !validate_exercise(current, error, error_capacity)) {
                return false;
            }
            if (course->exercise_count >= QUIZ_MAX_EXERCISES) {
                set_error(error, error_capacity, "too many exercises");
                return false;
            }
            current = &course->exercises[course->exercise_count++];
            memset(current, 0, sizeof(*current));
            current->id = identifier;
            current->correct_choice_index = QUIZ_CHOICE_COUNT;
            continue;
        }

        if (current == NULL) {
            continue;
        }

        if (strncmp(line, "prompt: ", 8u) == 0) {
            current->prompt = line + 8;
            continue;
        }

        size_t choice_index = 0u;
        char *choice_text = NULL;
        if (choice_line(line, &choice_index, &choice_text)) {
            current->choices[choice_index] = choice_text;
            if (choice_index + 1u > current->choice_count) {
                current->choice_count = choice_index + 1u;
            }
            continue;
        }

        if (strncmp(line, "answer: ", 8u) == 0 &&
            line[8] >= 'A' && line[8] <= 'D' && line[9] == '.') {
            current->correct_choice_index = (size_t)(line[8] - 'A');
            continue;
        }

        if (strncmp(line, "note: ", 6u) == 0) {
            current->note = line + 6;
            continue;
        }

        char *separator = strstr(line, " = ");
        if (separator != NULL && current->pair_count < QUIZ_PAIR_COUNT) {
            *separator = '\0';
            current->pair_left[current->pair_count] = line;
            current->pair_right[current->pair_count] = separator + 3;
            ++current->pair_count;
        }
    }

    if (current != NULL &&
        !validate_exercise(current, error, error_capacity)) {
        return false;
    }

    if (course->exercise_count == 0u) {
        set_error(error, error_capacity, "quiz contains no exercises");
        return false;
    }

    return true;
}
