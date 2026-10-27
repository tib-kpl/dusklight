# Android menu layout checks

This standalone check uses the project's pinned RmlUi and FreeType versions and
the fonts in `res/`. It reads the French translations from the actual C++ sources,
then checks that complete labels and selected values fit their allocated widths.
It covers 55 settings (including an active numeric input) and all eight settings
tabs at four DPI/UI scales, including the Odin 2 Portal's
1920×1080 display at 369 DPI and enlarged interface sizes.

Run from the repository root on Linux or WSL, with CMake 3.25+, Ninja, Python 3,
and a C++20 compiler:

```sh
python3 tools/ui_layout_check/generate.py
cmake -S tools/ui_layout_check -B build/ui-layout-check -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/ui-layout-check --parallel 4
build/ui-layout-check/layout-check "$PWD" build/ui-layout-labels.tsv
```

Dependencies are downloaded when CMake first runs. Append `--before` to the last
command to compare against the upstream styles without `android.rcss`; that
comparison is expected to report clipped labels or values.

This measures the native layout engine without a GPU renderer. Also verify the
actual APK on the console, including scrolling, tab changes, and controller input.
