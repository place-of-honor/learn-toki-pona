#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "../app/native/exercise_model.h"

static void choose_one_records_a_wrong_answer(void) {
    struct exercise_progress progress;
    const struct exercise *exercise = exercise_at(0);

    assert(exercise != NULL);
    assert(exercise->kind == EXERCISE_CHOOSE_ONE);
    exercise_progress_reset(&progress);

    assert(!exercise_submit_option(exercise, &progress, 1));
    assert(progress.complete);
    assert(progress.had_mistake);
}

static void word_bank_accepts_the_object_particle(void) {
    struct exercise_progress progress;
    const struct exercise *exercise = exercise_at(1);

    assert(exercise != NULL);
    assert(exercise->kind == EXERCISE_WORD_BANK);
    exercise_progress_reset(&progress);

    assert(exercise_submit_option(exercise, &progress, 0));
    assert(progress.complete);
    assert(!progress.had_mistake);
}

static void matching_requires_the_declared_pairs(void) {
    struct exercise_progress progress;
    const struct exercise *exercise = exercise_at(2);

    assert(exercise != NULL);
    assert(exercise->kind == EXERCISE_MATCH_PAIRS);
    exercise_progress_reset(&progress);

    assert(!exercise_submit_match(exercise, &progress, 0, 0));
    assert(!progress.complete);
    assert(progress.had_mistake);

    assert(exercise_submit_match(exercise, &progress, 0, 2));
    assert(exercise_submit_match(exercise, &progress, 1, 0));
    assert(exercise_submit_match(exercise, &progress, 2, 1));
    assert(progress.complete);
    assert(progress.had_mistake);
}

int main(void) {
    assert(exercise_count() == 3);
    choose_one_records_a_wrong_answer();
    word_bank_accepts_the_object_particle();
    matching_requires_the_declared_pairs();
    return 0;
}
