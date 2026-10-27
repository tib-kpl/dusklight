#pragma once

#if defined(__ANDROID__)
#include <SDL3/SDL_system.h>
#include <jni.h>
#endif

namespace dusk::ui::android_loading {

// Android draws this view on its UI thread, independently of blocking game initialization.
inline void show(const char* stage, const char* detail = "", int percent = -1,
    bool cancellable = false) noexcept {
#if defined(__ANDROID__)
    auto* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (!env) {
        return;
    }
    auto activity = static_cast<jobject>(SDL_GetAndroidActivity());
    if (!activity) {
        return;
    }
    auto cls = env->GetObjectClass(activity);
    auto method = env->GetMethodID(cls, "showLoading", "(Ljava/lang/String;Ljava/lang/String;IZ)V");
    if (method) {
        auto title = env->NewStringUTF(stage);
        auto subtitle = env->NewStringUTF(detail);
        env->CallVoidMethod(
            activity, method, title, subtitle, percent, static_cast<jboolean>(cancellable));
        env->DeleteLocalRef(title);
        env->DeleteLocalRef(subtitle);
    }
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    env->DeleteLocalRef(cls);
    env->DeleteLocalRef(activity);
#else
    (void)stage;
    (void)detail;
    (void)percent;
    (void)cancellable;
#endif
}

inline bool call(const char* name, bool returnsBool = false) noexcept {
#if defined(__ANDROID__)
    auto* env = static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
    if (!env) {
        return false;
    }
    auto activity = static_cast<jobject>(SDL_GetAndroidActivity());
    if (!activity) {
        return false;
    }
    auto cls = env->GetObjectClass(activity);
    auto method = env->GetMethodID(cls, name, returnsBool ? "()Z" : "()V");
    bool result = false;
    if (method) {
        if (returnsBool) {
            result = env->CallBooleanMethod(activity, method);
        } else {
            env->CallVoidMethod(activity, method);
        }
    }
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    env->DeleteLocalRef(cls);
    env->DeleteLocalRef(activity);
    return result;
#else
    (void)name;
    (void)returnsBool;
    return false;
#endif
}

inline void hide() noexcept {
    call("hideLoading");
}
inline bool cancellation_requested() noexcept {
    return call("isLoadingCancelled", true);
}

}  // namespace dusk::ui::android_loading
