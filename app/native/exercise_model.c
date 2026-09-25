#include "exercise_model.h"

static const struct exercise exercises[] = {
    {
        .id = "core-01-native",
        .lesson_number = 1,
        .kind = EXERCISE_CHOOSE_ONE,
        .instruction = "CHOOSE THE MEANING",
        .body.choose_one = {
            .prompt = "MI MOKU.",
            .choices = {"I EAT", "YOU EAT", "THEY WORK", "FOOD IS GOOD"},
            .choice_count = 4,
            .correct_choice_index = 0,
        },
    },
    {
        .id = "object-particle-native",
        .lesson_number = 2,
        .kind = EXERCISE_WORD_BANK,
        .instruction = "CHOOSE THE PARTICLE",
        .body.word_bank = {
            .prompt = "MI MOKU [ ] KILI.",
            .tokens = {"E", "LI", "LA", "PI"},
            .token_count = 4,
            .correct_token_index = 0,
        },
    },
    {
        .id = "subjects-native",
        .lesson_number = 1,
        .kind = EXERCISE_MATCH_PAIRS,
        .instruction = "MATCH THE PAIRS",
        .body.match_pairs = {
            .prompt = "SUBJECT WORDS",
            .left = {"MI", "SINA", "ONA"},
            .right = {"YOU", "THEY / IT", "I / WE"},
            .pair_count = 3,
            .correct_right_for_left = {2, 0, 1},
        },
    },
};

size_t exercise_count(void) {
    return sizeof(exercises) / sizeof(exercises[0]);
}

const struct exercise *exercise_at(size_t index) {
    if (index >= exercise_count()) {
        return NULL;
    }
    return &exercises[index];
}

void exercise_progress_reset(struct exercise_progress *progress) {
    progress->complete = false;
    progress->had_mistake = false;
    progress->matched_left_mask = 0;
}

bool exercise_submit_option(
    const struct exercise *exercise,
    struct exercise_progress *progress,
    size_t option_index
) {
    size_t option_count;
    size_t correct_option_index;

    if (exercise == NULL || progress == NULL || progress->complete) {
        return false;
    }

    if (exercise->kind == EXERCISE_CHOOSE_ONE) {
        option_count = exercise->body.choose_one.choice_count;
        correct_option_index = exercise->body.choose_one.correct_choice_index;
    } else if (exercise->kind == EXERCISE_WORD_BANK) {
        option_count = exercise->body.word_bank.token_count;
        correct_option_index = exercise->body.word_bank.correct_token_index;
    } else {
        return false;
    }

    if (option_index >= option_count) {
        return false;
    }

    progress->complete = true;
    if (option_index != correct_option_index) {
        progress->had_mistake = true;
        return false;
    }
    return true;
}

bool exercise_submit_match(
    const struct exercise *exercise,
    struct exercise_progress *progress,
    size_t left_index,
    size_t right_index
) {
    size_t pair_count;
    uint32_t completed_mask;

    if (exercise == NULL || progress == NULL || progress->complete) {
        return false;
    }
    if (exercise->kind != EXERCISE_MATCH_PAIRS) {
        return false;
    }

    pair_count = exercise->body.match_pairs.pair_count;
    if (left_index >= pair_count || right_index >= pair_count) {
        return false;
    }
    if ((progress->matched_left_mask & (UINT32_C(1) << left_index)) != 0) {
        return false;
    }

    if (exercise->body.match_pairs.correct_right_for_left[left_index] != right_index) {
        progress->had_mistake = true;
        return false;
    }

    progress->matched_left_mask |= UINT32_C(1) << left_index;
    completed_mask = (UINT32_C(1) << pair_count) - UINT32_C(1);
    if (progress->matched_left_mask == completed_mask) {
        progress->complete = true;
    }
    return true;
}
