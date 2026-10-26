# Android Shell

This directory contains Dusklight's Android shell built on top of Borealis.

## Prerequisites

- Android SDK with Platform 37.0 and Build Tools 36.0.0 installed (`ANDROID_HOME`)
- Android NDK version used by CMake presets (`ANDROID_NDK_VERSION`)
- JDK 17+

The SDK package ID is `platforms;android-37.0`. Gradle uses `compileSdk = 37`
and `compileSdkMinor = 0` to resolve this exact platform.

Example:

```bash
export ANDROID_HOME="$HOME/Android/Sdk"
export ANDROID_NDK_VERSION="29.0.14206865"
export JAVA_HOME="/usr/lib/jvm/java-17-openjdk"
```

## Build Native Libraries

```bash
cmake --preset android-arm64
cmake --build --preset android-arm64
```

This build produces `build/android-arm64/libmain.so`

## Build APK

```bash
cd platforms/android
./gradlew :app:assembleDebug
```

Output APK:

- `app/build/outputs/apk/debug/app-arm64-v8a-debug.apk`

Aurora needs a hardware-backed graphics adapter. If an AVD has GPU
acceleration disabled, launch it with `-gpu host`.

## Launch With Runtime Args (adb)

You can pass command-line args through the activity intent:

```bash
adb shell am start -n dev.twilitrealm.dusk/.DuskActivity \
  --es borealis_args "--backend vulkan"
```

Supported extras:

- `borealis_args`: single shell-like argument string
- `borealis_argv`: string-array argv

The legacy `dusk_args` and `dusk_argv` names remain accepted during the shell
transition.

## French APK interface

The Android interface uses French for settings, help text, controller configuration,
touch layout editing, graphics tuning, save management, and launch dialogs.
The game language is selected separately and depends on the languages available on
your disc. Desktop and Apple interfaces retain their original English text.

## Automatic builds and releases

Every branch push runs `.github/workflows/build.yml`, including documentation-only
commits. You can also run **Build** manually from the Actions page. Pull requests
build the same platforms without publishing a release or using signing secrets.

After all eight builds succeed, a prerelease tagged `build-<full commit SHA>` is
published with Android ARM64, Linux x86_64/ARM64, Windows x86_64/ARM64, macOS
x86_64/ARM64, and iOS ARM64 downloads. Each release includes SHA-256 checksums.
Rerunning a workflow updates the same release. Each push builds its tip commit;
push commits individually if you need a separate release for every intermediate
commit.

Linux packages include AppImages. Windows ZIPs contain the executable and its
runtime files. Apple tarballs preserve application bundle permissions and symlinks;
the iOS application still needs your own code signing before device installation.
Debug files and any generated SDK files from the original builds are retained.
Linux links mods directly against the executable and does not generate a separate
SDK directory; its debug archive includes the debug symbols.

Android releases use a persistent PKCS12 signing key (alias `dusklight`), so later
APKs can update earlier installations. Configure these repository Actions secrets:

- `ANDROID_KEYSTORE_BASE64`: Base64 of the PKCS12 keystore.
- `ANDROID_KEYSTORE_PASSWORD`: Password for both the keystore and its key.

Keep a secure backup of the signing key and password. They must never be committed.
For this fork, the initial local backup is in the ignored `.android-signing/`
directory. Pull request artifacts contain an unsigned APK; published releases
contain the signed APK, verified with `apksigner` and `zipalign`.
