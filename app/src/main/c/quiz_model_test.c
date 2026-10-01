#include "quiz_model.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fail(const char *message) {
    fprintf(stderr, "FAIL: %s\n", message);
    exit(1);
}

static char *read_file(const char *path, size_t *length) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    const long measured = ftell(file);
    if (measured < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    char *buffer = malloc((size_t)measured + 1u);
    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }

    const size_t read = fread(buffer, 1u, (size_t)measured, file);
    fclose(file);
    if (read != (size_t)measured) {
        free(buffer);
        return NULL;
    }
    buffer[read] = '\0';
    *length = read;
    return buffer;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fail("usage: quiz_model_test quizzes.txt");
    }

    size_t length = 0u;
    char *text = read_file(argv[1], &length);
    if (text == NULL) {
        fail("could not read quiz file");
    }

    QuizCourse course;
    char error[160];
    if (!quiz_course_parse(&course, text, length, error, sizeof(error))) {
        fprintf(stderr, "FAIL: parser: %s\n", error);
        free(text);
        return 1;
    }

    if (course.exercise_count != 56u) {
        fail("expected 56 exercises");
    }
    if (strcmp(course.exercises[0].id, "core-01") != 0 ||
        course.exercises[0].kind != QUIZ_MULTIPLE_CHOICE ||
        course.exercises[0].choice_count != 4u ||
        course.exercises[0].correct_choice_index != 0u ||
        strcmp(course.exercises[0].choices[0], "I eat.") != 0) {
        fail("first multiple-choice exercise changed");
    }
    if (strcmp(course.exercises[6].id, "core-07") != 0 ||
        course.exercises[6].kind != QUIZ_MATCHING ||
        course.exercises[6].pair_count != 4u ||
        strcmp(course.exercises[6].pair_left[0], "mi") != 0 ||
        strcmp(course.exercises[6].pair_right[0], "I, me, or we") != 0) {
        fail("first matching exercise changed");
    }

    for (size_t index = 0u; index < course.exercise_count; ++index) {
        const QuizExercise *exercise = &course.exercises[index];
        if (exercise->kind == QUIZ_MULTIPLE_CHOICE &&
            (exercise->choice_count != 4u ||
             exercise->correct_choice_index >= 4u)) {
            fail("invalid multiple-choice exercise");
        }
        if (exercise->kind == QUIZ_MATCHING && exercise->pair_count != 4u) {
            fail("invalid matching exercise");
        }
    }

    printf("PASS parsed %zu Toki Pona exercises\n", course.exercise_count);
    free(text);
    return 0;
}
