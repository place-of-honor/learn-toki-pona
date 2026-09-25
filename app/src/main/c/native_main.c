#include "quiz_model.h"

#include <android/asset_manager.h>
#include <android/input.h>
#include <android/log.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android_native_app_glue.h>
#include <jni.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "TokiPonaNative"
#define LOG_ERROR(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOG_INFO(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define DAILY_QUESTION_COUNT 12u

static const int32_t COLOR_PAPER = (int32_t)0xFFF4EFE3u;
static const int32_t COLOR_CARD = (int32_t)0xFFFFFDF8u;
static const int32_t COLOR_INK = (int32_t)0xFF202722u;
static const int32_t COLOR_MUTED = (int32_t)0xFF657067u;
static const int32_t COLOR_LINE = (int32_t)0xFFD3C9B7u;
static const int32_t COLOR_GREEN = (int32_t)0xFF256B4Bu;
static const int32_t COLOR_GREEN_PALE = (int32_t)0xFFDCEADEu;
static const int32_t COLOR_RED = (int32_t)0xFF9C3D35u;
static const int32_t COLOR_RED_PALE = (int32_t)0xFFF4DED9u;
static const int32_t COLOR_AMBER_PALE = (int32_t)0xFFF6E7CEu;
static const int32_t COLOR_WHITE = (int32_t)0xFFFFFFFFu;

typedef struct {
    float left;
    float top;
    float right;
    float bottom;
} RectF;

typedef enum {
    SCREEN_QUIZ = 1,
    SCREEN_FINISHED = 2
} ScreenState;

typedef struct {
    struct android_app *app;
    QuizCourse course;
    char *quiz_text;
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
    bool redraw;
} AppContext;

typedef struct {
    JNIEnv *env;
    bool detach;
    jobject surface;
    jobject canvas;
    jobject paint;
    jclass surface_class;
    jclass canvas_class;
    jclass paint_class;
    jmethodID unlock_canvas;
    jmethodID draw_color;
    jmethodID draw_rect;
    jmethodID draw_text;
    jmethodID paint_set_color;
    jmethodID paint_set_text_size;
    jmethodID paint_measure_text;
    jmethodID paint_ascent;
    jmethodID paint_descent;
} CanvasSession;

static RectF rect(float left, float top, float right, float bottom) {
    const RectF value = {left, top, right, bottom};
    return value;
}

static bool contains(RectF area, float x, float y) {
    return x >= area.left && x <= area.right &&
           y >= area.top && y <= area.bottom;
}

static float width_of(RectF area) {
    return area.right - area.left;
}

static float height_of(RectF area) {
    return area.bottom - area.top;
}

static RectF content_rect(const struct android_app *app) {
    const float window_width = (float)ANativeWindow_getWidth(app->window);
    const float window_height = (float)ANativeWindow_getHeight(app->window);
    const int32_t content_width =
        app->contentRect.right - app->contentRect.left;
    const int32_t content_height =
        app->contentRect.bottom - app->contentRect.top;

    if (content_width <= 0 || content_height <= 0) {
        return rect(0.0f, 0.0f, window_width, window_height);
    }

    return rect((float)app->contentRect.left,
                (float)app->contentRect.top,
                (float)app->contentRect.right,
                (float)app->contentRect.bottom);
}

static RectF place(RectF local, RectF viewport) {
    return rect(local.left + viewport.left,
                local.top + viewport.top,
                local.right + viewport.left,
                local.bottom + viewport.top);
}

static bool attach_environment(ANativeActivity *activity,
                               JNIEnv **environment,
                               bool *detach) {
    *detach = false;
    const jint status = (*activity->vm)->GetEnv(
        activity->vm, (void **)environment, JNI_VERSION_1_6);
    if (status == JNI_OK) {
        return true;
    }

    if (status != JNI_EDETACHED ||
        (*activity->vm)->AttachCurrentThread(
            activity->vm, environment, NULL) != JNI_OK) {
        LOG_ERROR("could not attach native thread to Android runtime");
        return false;
    }

    *detach = true;
    return true;
}

static bool clear_exception(JNIEnv *environment, const char *operation) {
    if ((*environment)->ExceptionCheck(environment) == JNI_FALSE) {
        return false;
    }

    LOG_ERROR("Android framework exception during %s", operation);
    (*environment)->ExceptionDescribe(environment);
    (*environment)->ExceptionClear(environment);
    return true;
}

static void end_canvas(AppContext *context, CanvasSession *session) {
    JNIEnv *environment = session->env;

    if (session->canvas != NULL &&
        session->surface != NULL &&
        session->unlock_canvas != NULL) {
        (*environment)->CallVoidMethod(
            environment,
            session->surface,
            session->unlock_canvas,
            session->canvas);
        (void)clear_exception(environment, "Surface.unlockCanvasAndPost");
    }

    if (session->paint != NULL) {
        (*environment)->DeleteLocalRef(environment, session->paint);
    }
    if (session->canvas != NULL) {
        (*environment)->DeleteLocalRef(environment, session->canvas);
    }
    if (session->surface != NULL) {
        (*environment)->DeleteLocalRef(environment, session->surface);
    }
    if (session->paint_class != NULL) {
        (*environment)->DeleteLocalRef(environment, session->paint_class);
    }
    if (session->canvas_class != NULL) {
        (*environment)->DeleteLocalRef(environment, session->canvas_class);
    }
    if (session->surface_class != NULL) {
        (*environment)->DeleteLocalRef(environment, session->surface_class);
    }

    if (session->detach) {
        (void)(*context->app->activity->vm)->DetachCurrentThread(
            context->app->activity->vm);
    }

    memset(session, 0, sizeof(*session));
}

static bool begin_canvas(AppContext *context, CanvasSession *session) {
    memset(session, 0, sizeof(*session));

    if (!attach_environment(
            context->app->activity,
            &session->env,
            &session->detach)) {
        return false;
    }

    JNIEnv *environment = session->env;
    session->surface =
        ANativeWindow_toSurface(environment, context->app->window);
    if (session->surface == NULL ||
        clear_exception(environment, "ANativeWindow_toSurface")) {
        end_canvas(context, session);
        return false;
    }

    session->surface_class =
        (*environment)->GetObjectClass(environment, session->surface);
    session->canvas_class =
        (*environment)->FindClass(environment, "android/graphics/Canvas");
    session->paint_class =
        (*environment)->FindClass(environment, "android/graphics/Paint");

    if (session->surface_class == NULL ||
        session->canvas_class == NULL ||
        session->paint_class == NULL ||
        clear_exception(environment, "finding graphics classes")) {
        end_canvas(context, session);
        return false;
    }

    const jmethodID lock_canvas = (*environment)->GetMethodID(
        environment,
        session->surface_class,
        "lockCanvas",
        "(Landroid/graphics/Rect;)Landroid/graphics/Canvas;");
    session->unlock_canvas = (*environment)->GetMethodID(
        environment,
        session->surface_class,
        "unlockCanvasAndPost",
        "(Landroid/graphics/Canvas;)V");
    const jmethodID paint_constructor = (*environment)->GetMethodID(
        environment, session->paint_class, "<init>", "()V");
    const jmethodID paint_set_antialias = (*environment)->GetMethodID(
        environment, session->paint_class, "setAntiAlias", "(Z)V");

    session->paint_set_color = (*environment)->GetMethodID(
        environment, session->paint_class, "setColor", "(I)V");
    session->paint_set_text_size = (*environment)->GetMethodID(
        environment, session->paint_class, "setTextSize", "(F)V");
    session->paint_measure_text = (*environment)->GetMethodID(
        environment,
        session->paint_class,
        "measureText",
        "(Ljava/lang/String;)F");
    session->paint_ascent = (*environment)->GetMethodID(
        environment, session->paint_class, "ascent", "()F");
    session->paint_descent = (*environment)->GetMethodID(
        environment, session->paint_class, "descent", "()F");

    session->draw_color = (*environment)->GetMethodID(
        environment, session->canvas_class, "drawColor", "(I)V");
    session->draw_rect = (*environment)->GetMethodID(
        environment,
        session->canvas_class,
        "drawRect",
        "(FFFFLandroid/graphics/Paint;)V");
    session->draw_text = (*environment)->GetMethodID(
        environment,
        session->canvas_class,
        "drawText",
        "(Ljava/lang/String;FFLandroid/graphics/Paint;)V");

    if (lock_canvas == NULL ||
        session->unlock_canvas == NULL ||
        paint_constructor == NULL ||
        paint_set_antialias == NULL ||
        session->paint_set_color == NULL ||
        session->paint_set_text_size == NULL ||
        session->paint_measure_text == NULL ||
        session->paint_ascent == NULL ||
        session->paint_descent == NULL ||
        session->draw_color == NULL ||
        session->draw_rect == NULL ||
        session->draw_text == NULL ||
        clear_exception(environment, "resolving graphics methods")) {
        end_canvas(context, session);
        return false;
    }

    session->canvas = (*environment)->CallObjectMethod(
        environment, session->surface, lock_canvas, NULL);
    if (session->canvas == NULL ||
        clear_exception(environment, "Surface.lockCanvas")) {
        session->canvas = NULL;
        end_canvas(context, session);
        return false;
    }

    session->paint = (*environment)->NewObject(
        environment, session->paint_class, paint_constructor);
    if (session->paint == NULL ||
        clear_exception(environment, "creating Paint")) {
        end_canvas(context, session);
        return false;
    }

    (*environment)->CallVoidMethod(
        environment,
        session->paint,
        paint_set_antialias,
        JNI_TRUE);

    if (clear_exception(environment, "configuring Paint")) {
        end_canvas(context, session);
        return false;
    }

    return true;
}

static void set_paint(CanvasSession *session,
                      int32_t color,
                      float text_size) {
    (*session->env)->CallVoidMethod(
        session->env,
        session->paint,
        session->paint_set_color,
        (jint)color);
    (*session->env)->CallVoidMethod(
        session->env,
        session->paint,
        session->paint_set_text_size,
        text_size);
}

static void draw_rectangle(CanvasSession *session,
                           RectF area,
                           int32_t color) {
    set_paint(session, color, 1.0f);
    (*session->env)->CallVoidMethod(
        session->env,
        session->canvas,
        session->draw_rect,
        area.left,
        area.top,
        area.right,
        area.bottom,
        session->paint);
}

static RectF inset(RectF area, float amount) {
    area.left += amount;
    area.top += amount;
    area.right -= amount;
    area.bottom -= amount;
    return area;
}

static void draw_card(CanvasSession *session,
                      RectF area,
                      int32_t fill) {
    draw_rectangle(session, area, COLOR_LINE);
    draw_rectangle(session, inset(area, 2.0f), fill);
}

static void draw_centered_line(CanvasSession *session,
                               RectF area,
                               const char *text,
                               float desired_size,
                               int32_t color) {
    JNIEnv *environment = session->env;
    const jstring value =
        (*environment)->NewStringUTF(environment, text);
    if (value == NULL ||
        clear_exception(environment, "creating text")) {
        return;
    }

    float size = desired_size;
    set_paint(session, color, size);

    float measured = (*environment)->CallFloatMethod(
        environment,
        session->paint,
        session->paint_measure_text,
        value);
    const float allowed = width_of(area) * 0.92f;

    if (measured > allowed && measured > 0.0f) {
        size *= allowed / measured;
        if (size < 11.0f) {
            size = 11.0f;
        }
        set_paint(session, color, size);
        measured = (*environment)->CallFloatMethod(
            environment,
            session->paint,
            session->paint_measure_text,
            value);
    }

    const float ascent = (*environment)->CallFloatMethod(
        environment, session->paint, session->paint_ascent);
    const float descent = (*environment)->CallFloatMethod(
        environment, session->paint, session->paint_descent);

    const float x =
        area.left + (width_of(area) - measured) * 0.5f;
    const float y =
        area.top + (height_of(area) - (descent + ascent)) * 0.5f;

    (*environment)->CallVoidMethod(
        environment,
        session->canvas,
        session->draw_text,
        value,
        x,
        y,
        session->paint);
    (*environment)->DeleteLocalRef(environment, value);
}

static size_t wrap_text(const char *text,
                        char lines[][192],
                        size_t maximum_lines,
                        size_t columns) {
    if (text == NULL || maximum_lines == 0u) {
        return 0u;
    }

    size_t count = 1u;
    size_t current = 0u;
    size_t used = 0u;
    lines[0][0] = '\0';

    const char *cursor = text;
    while (*cursor != '\0') {
        while (*cursor == ' ') {
            ++cursor;
        }
        if (*cursor == '\0') {
            break;
        }

        const char *word_end = cursor;
        while (*word_end != '\0' && *word_end != ' ') {
            ++word_end;
        }

        size_t word_bytes = (size_t)(word_end - cursor);
        if (word_bytes > 180u) {
            word_bytes = 180u;
        }

        const size_t separator = used == 0u ? 0u : 1u;
        if (used > 0u &&
            used + separator + word_bytes > columns) {
            if (count >= maximum_lines) {
                break;
            }
            current = count;
            ++count;
            used = 0u;
            lines[current][0] = '\0';
        }

        if (used != 0u &&
            used + 1u < sizeof(lines[current])) {
            lines[current][used++] = ' ';
        }

        const size_t room =
            sizeof(lines[current]) - 1u - used;
        const size_t copied =
            word_bytes < room ? word_bytes : room;
        memcpy(lines[current] + used, cursor, copied);
        used += copied;
        lines[current][used] = '\0';
        cursor = word_end;
    }

    return count;
}

static void draw_wrapped(CanvasSession *session,
                         RectF area,
                         const char *text,
                         float size,
                         int32_t color,
                         size_t maximum_lines,
                         size_t columns) {
    char lines[8][192];
    memset(lines, 0, sizeof(lines));

    if (maximum_lines > 8u) {
        maximum_lines = 8u;
    }

    const size_t count =
        wrap_text(text, lines, maximum_lines, columns);
    if (count == 0u) {
        return;
    }

    const float line_height =
        height_of(area) / (float)count;
    for (size_t index = 0u; index < count; ++index) {
        RectF line_area = area;
        line_area.top =
            area.top + (float)index * line_height;
        line_area.bottom =
            line_area.top + line_height;
        draw_centered_line(
            session, line_area, lines[index], size, color);
    }
}

static RectF header_rect(float width) {
    return rect(16.0f, 14.0f, width - 16.0f, 58.0f);
}

static RectF prompt_rect(float width) {
    return rect(18.0f, 70.0f, width - 18.0f, 188.0f);
}

static RectF next_rect(float width, float height) {
    return rect(
        18.0f, height - 70.0f, width - 18.0f, height - 16.0f);
}

static RectF note_rect(float width, float height) {
    return rect(
        18.0f, height - 150.0f, width - 18.0f, height - 78.0f);
}

static RectF multiple_choice_rect(float width,
                                  float height,
                                  size_t index,
                                  bool answered) {
    const float top = 202.0f;
    const float bottom =
        answered ? height - 162.0f : height - 24.0f;
    const float gap = 9.0f;
    const float button_height =
        (bottom - top - 3.0f * gap) / 4.0f;
    const float y =
        top + (float)index * (button_height + gap);

    return rect(18.0f, y, width - 18.0f, y + button_height);
}

static RectF matching_rect(float width,
                           float height,
                           bool right_column,
                           size_t index,
                           bool answered) {
    const float top = 206.0f;
    const float bottom =
        answered ? height - 162.0f : height - 24.0f;
    const float gap = 8.0f;
    const float row_height =
        (bottom - top - 3.0f * gap) / 4.0f;
    const float middle = width * 0.5f;
    const float y =
        top + (float)index * (row_height + gap);

    if (right_column) {
        return rect(
            middle + 5.0f, y, width - 18.0f, y + row_height);
    }
    return rect(
        18.0f, y, middle - 5.0f, y + row_height);
}

static void reset_question_state(AppContext *context) {
    context->answered = false;
    context->selected_choice = -1;
    context->matching_mask = 0u;
    context->selected_left = -1;
    context->selected_right = -1;
    context->matching_had_miss = false;
}

static void restart_session(AppContext *context) {
    context->current_question = 0u;
    context->correct_count = 0u;
    context->screen = SCREEN_QUIZ;
    reset_question_state(context);
    context->redraw = true;
}

static void advance_question(AppContext *context) {
    if (context->current_question + 1u >=
        context->session_count) {
        context->screen = SCREEN_FINISHED;
        context->redraw = true;
        return;
    }

    ++context->current_question;
    reset_question_state(context);
    context->redraw = true;
}

static const QuizExercise *current_exercise(
    const AppContext *context) {
    if (context->current_question >= context->session_count ||
        context->current_question >=
            context->course.exercise_count) {
        return NULL;
    }

    return &context->course.exercises[
        context->current_question];
}

static void render_multiple_choice(
    AppContext *context,
    CanvasSession *session,
    RectF viewport,
    const QuizExercise *exercise) {
    const float width = width_of(viewport);
    const float height = height_of(viewport);

    for (size_t index = 0u; index < 4u; ++index) {
        int32_t fill = COLOR_CARD;

        if (context->answered) {
            if (index == exercise->correct_choice_index) {
                fill = COLOR_GREEN_PALE;
            } else if ((int)index ==
                       context->selected_choice) {
                fill = COLOR_RED_PALE;
            }
        }

        const RectF local =
            multiple_choice_rect(
                width, height, index, context->answered);
        const RectF area = place(local, viewport);
        draw_card(session, area, fill);

        char label[320];
        (void)snprintf(
            label,
            sizeof(label),
            "%c. %s",
            (char)('A' + (int)index),
            exercise->choices[index]);

        draw_wrapped(
            session,
            inset(area, 8.0f),
            label,
            24.0f,
            COLOR_INK,
            3u,
            42u);
    }
}

static void render_matching(
    AppContext *context,
    CanvasSession *session,
    RectF viewport,
    const QuizExercise *exercise) {
    const float width = width_of(viewport);
    const float height = height_of(viewport);

    for (size_t index = 0u; index < 4u; ++index) {
        const bool matched =
            (context->matching_mask & (1u << index)) != 0u;

        int32_t left_fill =
            matched ? COLOR_GREEN_PALE : COLOR_CARD;
        if (!matched &&
            context->selected_left == (int)index) {
            left_fill = COLOR_AMBER_PALE;
        }

        const RectF left = place(
            matching_rect(
                width, height, false, index,
                context->answered),
            viewport);

        draw_card(session, left, left_fill);
        draw_wrapped(
            session,
            inset(left, 6.0f),
            exercise->pair_left[index],
            21.0f,
            COLOR_INK,
            4u,
            24u);

        const size_t pair_index = 3u - index;
        const bool right_matched =
            (context->matching_mask &
             (1u << pair_index)) != 0u;

        int32_t right_fill =
            right_matched ? COLOR_GREEN_PALE : COLOR_CARD;
        if (!right_matched &&
            context->selected_right == (int)index) {
            right_fill = COLOR_AMBER_PALE;
        }

        const RectF right = place(
            matching_rect(
                width, height, true, index,
                context->answered),
            viewport);

        draw_card(session, right, right_fill);
        draw_wrapped(
            session,
            inset(right, 6.0f),
            exercise->pair_right[pair_index],
            19.0f,
            COLOR_INK,
            4u,
            25u);
    }
}

static void render_finished(
    AppContext *context,
    CanvasSession *session,
    RectF viewport) {
    const float width = width_of(viewport);
    const float height = height_of(viewport);

    draw_wrapped(
        session,
        place(
            rect(
                24.0f,
                height * 0.26f,
                width - 24.0f,
                height * 0.42f),
            viewport),
        "Done",
        44.0f,
        COLOR_GREEN,
        1u,
        20u);

    char score[80];
    (void)snprintf(
        score,
        sizeof(score),
        "Score: %zu / %zu",
        context->correct_count,
        context->session_count);

    draw_wrapped(
        session,
        place(
            rect(
                24.0f,
                height * 0.43f,
                width - 24.0f,
                height * 0.53f),
            viewport),
        score,
        30.0f,
        COLOR_INK,
        1u,
        32u);

    const RectF restart = place(
        rect(
            24.0f,
            height * 0.62f,
            width - 24.0f,
            height * 0.72f),
        viewport);

    draw_card(session, restart, COLOR_GREEN_PALE);
    draw_centered_line(
        session,
        restart,
        "Tap to restart",
        26.0f,
        COLOR_GREEN);
}

static void render(AppContext *context) {
    if (context->app->window == NULL) {
        return;
    }

    CanvasSession session;
    if (!begin_canvas(context, &session)) {
        return;
    }

    const RectF viewport = content_rect(context->app);
    const float width = width_of(viewport);
    const float height = height_of(viewport);

    (*session.env)->CallVoidMethod(
        session.env,
        session.canvas,
        session.draw_color,
        (jint)COLOR_PAPER);

    if (context->screen == SCREEN_FINISHED) {
        render_finished(context, &session, viewport);
        end_canvas(context, &session);
        context->redraw = false;
        return;
    }

    const QuizExercise *exercise =
        current_exercise(context);
    if (exercise == NULL) {
        draw_centered_line(
            &session,
            place(
                rect(
                    20.0f,
                    20.0f,
                    width - 20.0f,
                    100.0f),
                viewport),
            "Quiz data unavailable",
            28.0f,
            COLOR_RED);
        end_canvas(context, &session);
        context->redraw = false;
        return;
    }

    char header[96];
    (void)snprintf(
        header,
        sizeof(header),
        "Daily drill  %zu / %zu",
        context->current_question + 1u,
        context->session_count);

    draw_centered_line(
        &session,
        place(header_rect(width), viewport),
        header,
        24.0f,
        COLOR_GREEN);

    draw_wrapped(
        &session,
        place(prompt_rect(width), viewport),
        exercise->prompt,
        30.0f,
        COLOR_INK,
        4u,
        44u);

    if (exercise->kind == QUIZ_MULTIPLE_CHOICE) {
        render_multiple_choice(
            context, &session, viewport, exercise);
    } else {
        render_matching(
            context, &session, viewport, exercise);
    }

    if (context->answered) {
        draw_wrapped(
            &session,
            place(note_rect(width, height), viewport),
            exercise->note,
            18.0f,
            COLOR_MUTED,
            3u,
            68u);

        const RectF next =
            place(next_rect(width, height), viewport);
        draw_card(&session, next, COLOR_GREEN);
        draw_centered_line(
            &session,
            next,
            "Next",
            25.0f,
            COLOR_WHITE);
    }

    end_canvas(context, &session);
    context->redraw = false;
}

static bool load_quiz_asset(AppContext *context) {
    AAsset *asset = AAssetManager_open(
        context->app->activity->assetManager,
        "quizzes.txt",
        AASSET_MODE_BUFFER);

    if (asset == NULL) {
        LOG_ERROR("could not open assets/quizzes.txt");
        return false;
    }

    const off_t measured = AAsset_getLength(asset);
    if (measured <= 0) {
        AAsset_close(asset);
        LOG_ERROR("quizzes.txt is empty");
        return false;
    }

    const size_t length = (size_t)measured;
    char *buffer = malloc(length + 1u);
    if (buffer == NULL) {
        AAsset_close(asset);
        LOG_ERROR("could not allocate quiz buffer");
        return false;
    }

    const int bytes_read =
        AAsset_read(asset, buffer, length);
    AAsset_close(asset);

    if (bytes_read < 0 ||
        (size_t)bytes_read != length) {
        free(buffer);
        LOG_ERROR("short read of quizzes.txt");
        return false;
    }

    buffer[length] = '\0';

    char error[192];
    if (!quiz_course_parse(
            &context->course,
            buffer,
            length,
            error,
            sizeof(error))) {
        LOG_ERROR("quiz parser failed: %s", error);
        free(buffer);
        return false;
    }

    context->quiz_text = buffer;
    context->session_count =
        context->course.exercise_count <
                DAILY_QUESTION_COUNT
            ? context->course.exercise_count
            : DAILY_QUESTION_COUNT;

    LOG_INFO(
        "loaded %zu exercises; daily session=%zu",
        context->course.exercise_count,
        context->session_count);

    return true;
}

static void resolve_matching_selection(
    AppContext *context) {
    if (context->selected_left < 0 ||
        context->selected_right < 0) {
        return;
    }

    const size_t left =
        (size_t)context->selected_left;
    const size_t visible_right =
        (size_t)context->selected_right;
    const size_t right_pair =
        3u - visible_right;

    if (left == right_pair) {
        context->matching_mask |= 1u << left;
    } else {
        context->matching_had_miss = true;
    }

    context->selected_left = -1;
    context->selected_right = -1;

    if (context->matching_mask == 0x0Fu) {
        context->answered = true;
        if (!context->matching_had_miss) {
            ++context->correct_count;
        }
    }
}

static int32_t handle_input(
    struct android_app *app,
    AInputEvent *event) {
    AppContext *context = app->userData;

    if (context == NULL ||
        AInputEvent_getType(event) !=
            AINPUT_EVENT_TYPE_MOTION) {
        return 0;
    }

    const int32_t action =
        AMotionEvent_getAction(event) &
        AMOTION_EVENT_ACTION_MASK;

    if (action != AMOTION_EVENT_ACTION_UP) {
        return 1;
    }

    const RectF viewport = content_rect(app);
    const float x =
        AMotionEvent_getX(event, 0u) - viewport.left;
    const float y =
        AMotionEvent_getY(event, 0u) - viewport.top;
    const float width = width_of(viewport);
    const float height = height_of(viewport);

    if (context->screen == SCREEN_FINISHED) {
        restart_session(context);
        return 1;
    }

    const QuizExercise *exercise =
        current_exercise(context);
    if (exercise == NULL) {
        return 1;
    }

    if (context->answered) {
        if (contains(next_rect(width, height), x, y)) {
            advance_question(context);
        }
        return 1;
    }

    if (exercise->kind == QUIZ_MULTIPLE_CHOICE) {
        for (size_t index = 0u; index < 4u; ++index) {
            if (contains(
                    multiple_choice_rect(
                        width, height, index, false),
                    x,
                    y)) {
                context->selected_choice = (int)index;
                context->answered = true;
                if (index ==
                    exercise->correct_choice_index) {
                    ++context->correct_count;
                }
                context->redraw = true;
                break;
            }
        }
        return 1;
    }

    for (size_t index = 0u; index < 4u; ++index) {
        if ((context->matching_mask &
             (1u << index)) == 0u &&
            contains(
                matching_rect(
                    width, height, false, index, false),
                x,
                y)) {
            context->selected_left = (int)index;
            resolve_matching_selection(context);
            context->redraw = true;
            return 1;
        }

        const size_t pair_index = 3u - index;
        if ((context->matching_mask &
             (1u << pair_index)) == 0u &&
            contains(
                matching_rect(
                    width, height, true, index, false),
                x,
                y)) {
            context->selected_right = (int)index;
            resolve_matching_selection(context);
            context->redraw = true;
            return 1;
        }
    }

    return 1;
}

static void handle_app_command(
    struct android_app *app,
    int32_t command) {
    AppContext *context = app->userData;
    if (context == NULL) {
        return;
    }

    switch (command) {
        case APP_CMD_INIT_WINDOW:
        case APP_CMD_WINDOW_RESIZED:
        case APP_CMD_CONTENT_RECT_CHANGED:
        case APP_CMD_CONFIG_CHANGED:
        case APP_CMD_GAINED_FOCUS:
            context->redraw = true;
            break;
        default:
            break;
    }
}

void android_main(struct android_app *app) {
    app_dummy();

    AppContext context;
    memset(&context, 0, sizeof(context));

    context.app = app;
    context.screen = SCREEN_QUIZ;
    context.selected_choice = -1;
    context.selected_left = -1;
    context.selected_right = -1;
    context.redraw = true;

    app->userData = &context;
    app->onAppCmd = handle_app_command;
    app->onInputEvent = handle_input;

    if (!load_quiz_asset(&context)) {
        context.session_count = 0u;
    }

    while (true) {
        int events = 0;
        struct android_poll_source *source = NULL;
        const int timeout =
            context.redraw ? 0 : -1;

        const int result = ALooper_pollOnce(
            timeout,
            NULL,
            &events,
            (void **)&source);

        if (result >= 0 && source != NULL) {
            source->process(app, source);
        }

        if (app->destroyRequested != 0) {
            free(context.quiz_text);
            context.quiz_text = NULL;
            return;
        }

        if (context.redraw && app->window != NULL) {
            render(&context);
        }
    }
}
