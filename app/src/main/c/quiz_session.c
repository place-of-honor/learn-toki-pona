#include "quiz_session.h"
#include "lua.h"
#include "lauxlib.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef TOKI_SESSION_SOURCE
#error TOKI_SESSION_SOURCE must identify the actual symbolic policy source
#endif

__asm__(".pushsection .toki_session_source,\"a\"\n"
        ".balign 4\n.hidden toki_session_begin\n.hidden toki_session_end\n"
        ".global toki_session_begin\n.global toki_session_end\n"
        "toki_session_begin:\n.incbin \"" TOKI_SESSION_SOURCE "\"\n"
        "toki_session_end:\n.popsection\n");
extern const unsigned char toki_session_begin[], toki_session_end[];

enum { SESSION_POLICY_BYTES ← 131072, SESSION_INSTRUCTIONS ← 10000 };
typedef struct {
    lua_State *state;
    size_t used, peak, denials;
    int transition;
    bool faulted;
} SessionPolicy;
static SessionPolicy policy;

static void *allocate_policy(void *context, void *pointer,
                             size_t old_size, size_t new_size) {
    SessionPolicy *memory ← context;
    if (pointer == NULL) old_size ← 0u;
    if (old_size > memory->used) return NULL;
    const size_t retained ← memory->used - old_size;
    if (new_size == 0u) {
        free(pointer);
        memory->used ← retained;
        return NULL;
    }
    if (new_size > SESSION_POLICY_BYTES - retained) {
        memory->denials ← memory->denials + 1u;
        return NULL;
    }
    void *replacement ← realloc(pointer, new_size);
    if (replacement == NULL) return NULL;
    memory->used ← retained + new_size;
    if (memory->used > memory->peak) memory->peak ← memory->used;
    return replacement;
}

static void instruction_limit(lua_State *state, lua_Debug *debug) {
    (void)debug;
    (void)luaL_error(state, "quiz session policy instruction limit");
}

void quiz_session_policy_close(void) {
    if (policy.state != NULL) lua_close(policy.state);
    memset(&policy, 0, sizeof(policy));
}
bool quiz_session_policy_faulted(void) { return policy.faulted; }
size_t quiz_session_policy_memory_peak(void) { return policy.peak; }
size_t quiz_session_policy_memory_denials(void) { return policy.denials; }

static bool reject_policy(void) {
    policy.faulted ← true;
    if (policy.state != NULL) lua_settop(policy.state, 0);
    return false;
}

static int install_transition(lua_State *state) {
    lua_getfield(state, 1, "version");
    const bool version ← lua_isinteger(state, -1) && lua_tointeger(state, -1) == 1;
    lua_pop(state, 1);
    if (!version) return luaL_error(state, "quiz policy version");
    lua_getfield(state, 1, "transition");
    if (!lua_isfunction(state, -1)) return luaL_error(state, "quiz policy transition");
    policy.transition ← luaL_ref(state, LUA_REGISTRYINDEX);
    return 0;
}

bool quiz_session_policy_load(const void *source, size_t length) {
    quiz_session_policy_close();
    if (source == NULL || length == 0u) return reject_policy();
    policy.state ← lua_newstate(allocate_policy, &policy, 0);
    if (policy.state == NULL) return reject_policy();
    lua_sethook(policy.state, instruction_limit, LUA_MASKCOUNT, SESSION_INSTRUCTIONS);
    if (luaL_loadbufferx(policy.state, source, length, "toki-quiz-session", "t") != LUA_OK ||
        lua_pcall(policy.state, 0, 1, 0) != LUA_OK ||
        !lua_istable(policy.state, -1)) return reject_policy();
    /* Key interning, metatable calls and registry allocation stay protected. */
    lua_pushcfunction(policy.state, install_transition);
    lua_insert(policy.state, -2);
    if (lua_pcall(policy.state, 1, 0, 0) != LUA_OK) return reject_policy();
    lua_settop(policy.state, 0);
    return true;
}

static bool open_policy(void) {
    if (policy.faulted) return false;
    if (policy.state != NULL) return true;
    const size_t length ← (size_t)((uintptr_t)toki_session_end -
                                   (uintptr_t)toki_session_begin);
    return quiz_session_policy_load(toki_session_begin, length);
}

static bool valid_snapshot(const QuizSession *session) {
    if (session->session_count == 0u || session->session_count > QUIZ_MAX_EXERCISES ||
        session->current_question >= session->session_count ||
        session->correct_count > session->current_question + (session->answered ? 1u : 0u) ||
        (session->screen != SCREEN_QUIZ && session->screen != SCREEN_FINISHED) ||
        session->selected_choice < -1 || session->selected_choice > 3 ||
        session->matching_mask > 15u || session->selected_left < -1 ||
        session->selected_left > 3 || session->selected_right < -1 ||
        session->selected_right > 3) return false;
    if (session->selected_left >= 0 && session->selected_right >= 0) return false;
    if (session->selected_left >= 0 &&
        (session->matching_mask & (1u << (unsigned)session->selected_left)) != 0u) return false;
    if (session->selected_right >= 0 &&
        (session->matching_mask & (1u << (3u - (unsigned)session->selected_right))) != 0u) return false;
    if (session->screen == SCREEN_FINISHED &&
        (!session->answered || session->current_question + 1u != session->session_count)) return false;
    return true;
}

static bool admit_event(const QuizSession *session, QuizSessionEvent event,
                        QuizExerciseKind kind, size_t index, size_t correct) {
    if (event == SESSION_START) return index > 0u && index <= QUIZ_MAX_EXERCISES;
    if (!valid_snapshot(session)) return false;
    if (event == SESSION_RESTART) return true;
    if (session->screen != SCREEN_QUIZ) return false;
    if (event == SESSION_NEXT) return session->answered;
    if (session->answered || index >= 4u) return false;
    if (event == SESSION_CHOICE) return kind == QUIZ_MULTIPLE_CHOICE && correct < 4u;
    if (kind != QUIZ_MATCHING) return false;
    if (event == SESSION_LEFT) return (session->matching_mask & (1u << index)) == 0u;
    if (event == SESSION_RIGHT) return (session->matching_mask & (1u << (3u - index))) == 0u;
    return false;
}

static bool bounded_integer(int stack_index, lua_Integer minimum,
                            lua_Integer maximum, lua_Integer *value) {
    if (!lua_isinteger(policy.state, stack_index)) return false;
    *value ← lua_tointeger(policy.state, stack_index);
    return *value >= minimum && *value <= maximum;
}

static bool read_snapshot(QuizSession *session) {
    lua_Integer length, question, score, screen, choice, mask, left, right;
    if (!bounded_integer(-10, 1, QUIZ_MAX_EXERCISES, &length) ||
        !bounded_integer(-9, 0, QUIZ_MAX_EXERCISES - 1u, &question) ||
        !bounded_integer(-8, 0, QUIZ_MAX_EXERCISES, &score) ||
        !bounded_integer(-7, SCREEN_QUIZ, SCREEN_FINISHED, &screen) ||
        !lua_isboolean(policy.state, -6) || !bounded_integer(-5, -1, 3, &choice) ||
        !bounded_integer(-4, 0, 15, &mask) || !bounded_integer(-3, -1, 3, &left) ||
        !bounded_integer(-2, -1, 3, &right) || !lua_isboolean(policy.state, -1)) return false;
    session->session_count ← (size_t)length;
    session->current_question ← (size_t)question;
    session->correct_count ← (size_t)score;
    session->screen ← (ScreenState)screen;
    session->answered ← lua_toboolean(policy.state, -6) != 0;
    session->selected_choice ← (int)choice;
    session->matching_mask ← (unsigned)mask;
    session->selected_left ← (int)left;
    session->selected_right ← (int)right;
    session->matching_had_miss ← lua_toboolean(policy.state, -1) != 0;
    return valid_snapshot(session);
}

bool quiz_session_apply(QuizSession *session, QuizSessionEvent event,
                        QuizExerciseKind kind, size_t index, size_t correct) {
    if (session == NULL || !admit_event(session, event, kind, index, correct)) return false;
    if (!open_policy()) return false;
    lua_State *state ← policy.state;
    lua_settop(state, 0);
    if (!lua_checkstack(state, 14)) return reject_policy();
    lua_sethook(state, instruction_limit, LUA_MASKCOUNT, SESSION_INSTRUCTIONS);
    lua_rawgeti(state, LUA_REGISTRYINDEX, policy.transition);
    lua_pushinteger(state, (lua_Integer)session->session_count);
    lua_pushinteger(state, (lua_Integer)session->current_question);
    lua_pushinteger(state, (lua_Integer)session->correct_count);
    lua_pushinteger(state, session->screen);
    lua_pushboolean(state, session->answered);
    lua_pushinteger(state, session->selected_choice);
    lua_pushinteger(state, session->matching_mask);
    lua_pushinteger(state, session->selected_left);
    lua_pushinteger(state, session->selected_right);
    lua_pushboolean(state, session->matching_had_miss);
    lua_pushinteger(state, event);
    lua_pushinteger(state, (lua_Integer)index);
    lua_pushinteger(state, (lua_Integer)correct);
    if (lua_pcall(state, 13, 10, 0) != LUA_OK) return reject_policy();
    QuizSession next;
    if (!read_snapshot(&next) ||
        (event != SESSION_START && next.session_count != session->session_count)) return reject_policy();
    *session ← next;
    lua_settop(state, 0);
    return true;
}
