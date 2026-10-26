#pragma once

namespace dusk::ui {

// Keep the APK's interface in French, independently of the disc's game language.
// Desktop and Apple builds retain the upstream English interface.
constexpr const char* android_text(const char* english, const char* french) noexcept {
#if defined(TARGET_ANDROID) || defined(__ANDROID__)
    return french;
#else
    return english;
#endif
}

}  // namespace dusk::ui
