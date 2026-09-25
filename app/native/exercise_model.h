#ifndef TOKI_PONA_EXERCISE_MODEL_H
#define TOKI_PONA_EXERCISE_MODEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum exercise_kind {
    EXERCISE_CHOOSE_ONE,
    EXERCISE_MATCH_PAIRS,
    EXERCISE_WORD_BANK
};

struct choose_one_exercise {
    const char *prompt;
    const char *choices[4];
    size_t choice_count;
    size_t correct_choice_index;
};

struct match_pair_exercise {
    const char *prompt;
    const char *left[4];
    const char *right[4];
    size_t pair_count;
    size_t correct_right_for_left[4];
};

struct word_bank_exercise {
    const char *prompt;
    const char *tokens[6];
    size_t token_count;
    size_t correct_token_index;
};

struct exercise {
    const char *id;
    unsigned lesson_number;
    enum exercise_kind kind;
    const char *instruction;
    union {
        struct choose_one_exercise choose_one;
        struct match_pair_exercise match_pairs;
        struct word_bank_exercise word_bank;
    } body;
};

struct exercise_progress {
    bool complete;
    bool had_mistake;
    uint32_t matched_left_mask;
};

size_t exercise_count(void);
const struct exercise *exercise_at(size_t index);

void exercise_progress_reset(struct exercise_progress *progress);

bool exercise_submit_option(
    const struct exercise *exercise,
    struct exercise_progress *progress,
    size_t option_index
);

bool exercise_submit_match(
    const struct exercise *exercise,
    struct exercise_progress *progress,
    size_t left_index,
    size_t right_index
);

#endif
