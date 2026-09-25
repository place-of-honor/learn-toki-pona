#include <android/input.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "canvas.h"
#include "exercise_model.h"
#include "native_ui.h"

#define LOG_TAG "TokiPonaNative"

struct application_state {
    struct android_app *app;
    struct native_ui_state ui;
    bool redraw_requested;
};

static void draw_application(struct application_state *state) {
    ANativeWindow_Buffer buffer;
    struct canvas canvas;

    if (state->app->window == NULL) return;
    ANativeWindow_setBuffersGeometry(state->app->window, 0, 0, WINDOW_FORMAT_RGB_565);
    if (ANativeWindow_lock(state->app->window, &buffer, NULL) != 0) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "window_lock_failed");
        return;
    }

    canvas.pixels = (uint16_t *)buffer.bits;
    canvas.width = buffer.width;
    canvas.height = buffer.height;
    canvas.stride = buffer.stride;
    native_ui_draw(&state->ui, &canvas);

    ANativeWindow_unlockAndPost(state->app->window);
    state->redraw_requested = false;
}

static int32_t handle_input_event(struct android_app *app, AInputEvent *event) {
    struct application_state *state = (struct application_state *)app->userData;
    int32_t action;

    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) return 0;
    action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
    if (action == AMOTION_EVENT_ACTION_UP && app->window != NULL) {
        native_ui_handle_tap(
            &state->ui,
            ANativeWindow_getWidth(app->window),
            ANativeWindow_getHeight(app->window),
            AMotionEvent_getX(event, 0),
            AMotionEvent_getY(event, 0)
        );
        state->redraw_requested = true;
    }
    return 1;
}

static void handle_app_command(struct android_app *app, int32_t command) {
    struct application_state *state = (struct application_state *)app->userData;

    switch (command) {
        case APP_CMD_INIT_WINDOW:
            if (app->window != NULL) {
                state->redraw_requested = true;
                __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "native_ui_ready");
            }
            break;
        case APP_CMD_WINDOW_REDRAW_NEEDED:
        case APP_CMD_GAINED_FOCUS:
            state->redraw_requested = true;
            break;
        default:
            break;
    }
}

void android_main(struct android_app *app) {
    struct application_state state;

    memset(&state, 0, sizeof(state));
    state.app = app;
    native_ui_initialize(&state.ui);
    app->userData = &state;
    app->onAppCmd = handle_app_command;
    app->onInputEvent = handle_input_event;

    __android_log_print(
        ANDROID_LOG_INFO,
        LOG_TAG,
        "native_exercise_renderer_start exercises=%zu",
        exercise_count()
    );

    for (;;) {
        int events;
        struct android_poll_source *source = NULL;
        int timeout = state.redraw_requested ? 0 : -1;
        int ident;

        while ((ident = ALooper_pollOnce(timeout, NULL, &events, (void **)&source)) >= 0) {
            (void)ident;
            if (source != NULL) source->process(app, source);
            if (app->destroyRequested != 0) return;
            timeout = 0;
        }

        if (state.redraw_requested && app->window != NULL) draw_application(&state);
    }
}
