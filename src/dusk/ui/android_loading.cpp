#if defined(__ANDROID__)
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <jni.h>

extern "C" JNIEXPORT void JNICALL Java_dev_twilitrealm_dusk_DuskActivity_nativeWakeRenderer(
    JNIEnv*, jclass) {
    if (!SDL_WasInit(SDL_INIT_EVENTS)) {
        return;
    }
    // Surface readiness changes after SDL's resume callback. Wake SDL_WaitEvent once the
    // Android surface callbacks have finished, so Aurora can recheck its pause reasons.
    static const Uint32 wakeEvent = SDL_RegisterEvents(1);
    if (wakeEvent != 0) {
        SDL_Event event{};
        event.type = wakeEvent;
        SDL_PushEvent(&event);
    }
}
#endif
