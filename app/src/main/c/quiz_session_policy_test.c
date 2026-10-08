#include "quiz_session.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool equal_state(const QuizSession *left, const QuizSession *right) {
#define SAME(field) (left->field == right->field)
    return SAME(session_count) && SAME(current_question) && SAME(correct_count) &&
           SAME(answered) && SAME(selected_choice) && SAME(matching_mask) &&
           SAME(selected_left) && SAME(selected_right) && SAME(matching_had_miss) &&
           SAME(screen);
#undef SAME
}

static void fault_stays_closed(const QuizSession *before) {
    QuizSession state ← *before;
    assert(quiz_session_policy_faulted());
    assert(!quiz_session_apply(&state, SESSION_START, QUIZ_MULTIPLE_CHOICE, 3u, 0u));
    assert(equal_state(&state, before));
    assert(quiz_session_policy_faulted());
    quiz_session_policy_close();
    assert(quiz_session_apply(&state, SESSION_START, QUIZ_MULTIPLE_CHOICE, 3u, 0u));
    assert(!quiz_session_policy_faulted() && state.current_question == 0u);
}

static void rejected_load(const char *source, const QuizSession *before) {
    assert(!quiz_session_policy_load(source, strlen(source)));
    fault_stays_closed(before);
}

static void rejected_call(const char *source, const QuizSession *before) {
    assert(quiz_session_policy_load(source, strlen(source)));
    QuizSession state ← *before;
    assert(!quiz_session_apply(&state, SESSION_CHOICE, QUIZ_MULTIPLE_CHOICE, 0u, 0u));
    assert(equal_state(&state, before));
    fault_stays_closed(before);
}

int main(void) {
    QuizSession before;
    memset(&before, 0, sizeof(before));
    assert(quiz_session_apply(&before, SESSION_START, QUIZ_MULTIPLE_CHOICE, 3u, 0u));
    rejected_load("local unfinished ←", &before);
    rejected_load("local unfinished → 1", &before);
    rejected_load("return {version ← 2, transition ← ƒ() end}", &before);
    rejected_load("return {version ← 1, transition ← false}", &before);
    rejected_load("while true do end", &before);
    rejected_load("\x1bLua", &before); /* text-only parser admission */
    rejected_call("return {version ← 1, transition ← ƒ() while true do end end}", &before);
    rejected_call("return {version ← 1, transition ← ƒ() return nil end}", &before);
    rejected_call("return {version ← 1, transition ← ƒ() return 3,0,0,1,false,-1,16,-1,-1,false end}", &before);
    rejected_call("return {version ← 1, transition ← ƒ() return 3,0,4,1,true,0,0,-1,-1,false end}", &before);
    rejected_call("return {version ← 1, transition ← ƒ() return 3,0,0,1,false,-1,0,4,-1,false end}", &before);
    rejected_call("return {version ← 1, transition ← ƒ() return 3,0,0,2,false,-1,0,-1,-1,false end}", &before);
    rejected_call("return {version ← 1, transition ← ƒ() return os.execute('true') end}", &before);
    /* A large literal retained by a returned closure stresses allocation while
       parsing/installing the actual module. Every failure remains protected. */
    const char *prefix ← "local retained ← \"";
    const char *suffix ← "\"; return {version ← 1, transition ← ƒ() return retained end}";
    const size_t payload ← 60000u;
    const size_t closure_length ← strlen(prefix) + payload + strlen(suffix);
    char *near_cap ← malloc(closure_length + 1u);
    assert(near_cap != NULL);
    memcpy(near_cap, prefix, strlen(prefix));
    memset(near_cap + strlen(prefix), 'b', payload);
    memcpy(near_cap + strlen(prefix) + payload, suffix, strlen(suffix) + 1u);
    assert(!quiz_session_policy_load(near_cap, closure_length));
    /* A denied growth need not raise the successful-allocation peak. */
    assert(quiz_session_policy_memory_denials() > 0u);
    assert(quiz_session_policy_memory_peak() <= 131072u);
    free(near_cap);
    fault_stays_closed(&before);
    char *oversized ← malloc(131072u);
    assert(oversized != NULL);
    memset(oversized, 'a', 131072u);
    memcpy(oversized, "return \"", 8u);
    oversized[131071u] ← '"';
    assert(!quiz_session_policy_load(oversized, 131072u));
    free(oversized);
    assert(quiz_session_policy_memory_denials() > 0u);
    assert(quiz_session_policy_memory_peak() <= 131072u);
    fault_stays_closed(&before);
    printf("PASS actual policy parser, text-only load, instruction/heap bounds, result domains, no host libraries, unchanged snapshot and explicit recovery; heap_peak=%zu\n",
           quiz_session_policy_memory_peak());
    quiz_session_policy_close();
    return 0;
}
