#include "settings.hpp"
#include "localization.hpp"

#include "bool_button.hpp"
#include "controller_config.hpp"
#include "graphics_tuner.hpp"
#include "menu_bar.hpp"
#include "modal.hpp"
#include "number_button.hpp"
#include "pane.hpp"
#include "prelaunch.hpp"
#include "saves_window.hpp"
#include "touch_controls_editor.hpp"
#include "ui.hpp"

#include "dusk/app_info.hpp"
#include "dusk/audio/DuskAudioSystem.h"
#include "dusk/audio/DuskDsp.hpp"
#include "dusk/config.hpp"
#include "dusk/data.hpp"
#include "dusk/discord_presence.hpp"
#include "dusk/hotkeys.h"
#include "dusk/imgui/ImGuiEngine.hpp"
#include "dusk/language.hpp"
#include "dusk/livesplit.h"
#include "dusk/presentation.hpp"
#include "dusk/speedrun.h"

#include <aurora/gfx.h>
#include <aurora/lib/window.hpp>
#include <borealis/file_select.hpp>
#include <borealis/io.hpp>
#if BOREALIS_HAS_SENTRY
#include <borealis/sentry.hpp>
#endif
#include <SDL3/SDL_filesystem.h>
#include <fmt/format.h>

#include <algorithm>
#include <filesystem>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(TARGET_ANDROID) || defined(__ANDROID__) ||                                             \
    (defined(__APPLE__) && TARGET_OS_IOS && !TARGET_OS_MACCATALYST)
#define TOUCH_CONTROLS_AVAILABLE true
#else
#define TOUCH_CONTROLS_AVAILABLE false
#endif

namespace dusk::ui {
namespace {

constexpr std::array kCardFileTypes = {
    android_text("Card Image", "Image de carte mémoire"),
    android_text("GCI Folder", "Dossier GCI"),
};

constexpr std::array kFpsOverlayCornerNames = {
    android_text("Top Left", "En haut à gauche"),
    android_text("Top Right", "En haut à droite"),
    android_text("Bottom Left", "En bas à gauche"),
    android_text("Bottom Right", "En bas à droite"),
};

constexpr std::array kInterpolationModes = {
    android_text("Off", "Désactivé"),
    android_text("Capped", "Avec limite"),
    android_text("Unlimited", "Sans limite"),
};

constexpr std::array kAudioOutputModeNames = {
    android_text("Stereo (Speakers)", "Stéréo (haut-parleurs)"),
    android_text("Stereo (Headphones)", "Stéréo (casque)"),
    android_text("5.1 Surround", "Surround 5.1"),
    android_text("7.1 Surround", "Surround 7.1"),
};

constexpr std::array kLetterboxModes = {
    android_text("Off", "Désactivé"),
    android_text("On", "Activé"),
    android_text("Only During Gameplay", "Uniquement en jeu"),
    android_text("Only During Cutscenes", "Uniquement pendant les cinématiques"),
};

constexpr std::array kTouchTargetingLabels = {
    android_text("Hybrid", "Hybride"),
    android_text("Hold", "Maintenir"),
    android_text("Switch", "Basculer"),
};

constexpr std::array kTouchTargetingDescriptions = {
    android_text(
        "Tap once to lock on when a target is found. Double-tap when none is found to hold L.",
        "Touchez une fois pour verrouiller une cible. Sans cible, touchez deux fois pour maintenir "
        "L."),
    android_text("L stays held only while your finger is on the button.",
        "L reste enfoncé tant que votre doigt touche le bouton."),
    android_text("Tap L to keep it held. Tap again to release it.",
        "Touchez L pour le maintenir enfoncé. Touchez à nouveau pour le relâcher."),
};

constexpr std::array kGyroInputModeLabels = {
    android_text("Sensor", "Capteur"),
    android_text("Mouse", "Souris"),
};

constexpr std::array kMenuScalingModeLabels = {
    "GameCube",
    "Wii",
    "Dusklight",
};

constexpr std::array kAlwaysGreatspinModes = {
    android_text("Off", "Désactivé"),
    android_text("After Learning Skill", "Après avoir appris la technique"),
    android_text("Always", "Toujours"),
};

constexpr std::array kMagicArmorModes = {
    "Normal",
    android_text("On Damage", "Lors des dégâts"),
    android_text("Double Defense", "Défense doublée"),
    android_text("Invincible", "Invulnérable"),
    android_text("Cosmetic", "Apparence uniquement"),
};

bool try_parse_backend(std::string_view backend, AuroraBackend& outBackend) {
    if (backend == "auto") {
        outBackend = BACKEND_AUTO;
        return true;
    }
    if (backend == "d3d11") {
        outBackend = BACKEND_D3D11;
        return true;
    }
    if (backend == "d3d12") {
        outBackend = BACKEND_D3D12;
        return true;
    }
    if (backend == "metal") {
        outBackend = BACKEND_METAL;
        return true;
    }
    if (backend == "vulkan") {
        outBackend = BACKEND_VULKAN;
        return true;
    }
    if (backend == "opengl") {
        outBackend = BACKEND_OPENGL;
        return true;
    }
    if (backend == "opengles") {
        outBackend = BACKEND_OPENGLES;
        return true;
    }
    if (backend == "webgpu") {
        outBackend = BACKEND_WEBGPU;
        return true;
    }
    if (backend == "null") {
        outBackend = BACKEND_NULL;
        return true;
    }

    return false;
}

std::string_view backend_name(AuroraBackend backend) {
    switch (backend) {
    default:
        return "Auto";
    case BACKEND_D3D12:
        return "D3D12";
    case BACKEND_D3D11:
        return "D3D11";
    case BACKEND_METAL:
        return "Metal";
    case BACKEND_VULKAN:
        return "Vulkan";
    case BACKEND_OPENGL:
        return "OpenGL";
    case BACKEND_OPENGLES:
        return "OpenGL ES";
    case BACKEND_WEBGPU:
        return "WebGPU";
    case BACKEND_NULL:
        return "Null";
    }
}

std::string_view backend_id(AuroraBackend backend) {
    switch (backend) {
    default:
        return "auto";
    case BACKEND_D3D12:
        return "d3d12";
    case BACKEND_D3D11:
        return "d3d11";
    case BACKEND_METAL:
        return "metal";
    case BACKEND_VULKAN:
        return "vulkan";
    case BACKEND_OPENGL:
        return "opengl";
    case BACKEND_OPENGLES:
        return "opengles";
    case BACKEND_WEBGPU:
        return "webgpu";
    case BACKEND_NULL:
        return "null";
    }
}

std::vector<AuroraBackend> available_backends() {
    std::vector<AuroraBackend> backends;
    backends.emplace_back(BACKEND_AUTO);
    size_t backendCount = 0;
    const AuroraBackend* raw = aurora_get_available_backends(&backendCount);
    for (size_t i = 0; i < backendCount; ++i) {
        // Do not expose NULL
        if (raw[i] != BACKEND_NULL) {
            backends.emplace_back(raw[i]);
        }
    }
    return backends;
}

AuroraBackend configured_backend() {
    AuroraBackend configuredBackend = BACKEND_AUTO;
    const auto configuredId = getSettings().backend.graphicsBackend.getValue();
    if (!try_parse_backend(configuredId, configuredBackend)) {
        configuredBackend = BACKEND_AUTO;
    }
    return configuredBackend;
}

bool is_graphics_backend_restart_pending() {
    return getSettings().backend.graphicsBackend.getValue() !=
           prelaunch_state().initialGraphicsBackend;
}

Rml::String graphics_backend_display_name() {
    if (is_graphics_backend_restart_pending()) {
        return Rml::String{backend_name(configured_backend())};
    }
    return Rml::String{backend_name(aurora_get_backend())};
}

Rml::String configured_data_path_display_name() {
    const auto path = data::abbreviated_path_string(data::configured_data_path());
    if (path.empty()) {
        return android_text("(none)", "(aucun)");
    }

    auto display = borealis::io::display_name(path);
    if (display.empty()) {
        return path;
    }
    return display;
}

class DataFolderPathText : public Component {
public:
    explicit DataFolderPathText(Rml::Element* parent)
        : Component(append(parent, "data-folder-path")) {
        append_text_element(
            mRoot, "small", android_text("Current data folder:", "Dossier de données actuel :"));
        mPath = append(mRoot, "file-path");
    }

    void update() override {
        const Rml::String path = data::abbreviated_path_string(data::configured_data_path());
        if (path != mCurrentPath) {
            set_text_content(mPath, path);
            mCurrentPath = path;
        }
        Component::update();
    }

private:
    Rml::Element* mPath = nullptr;
    Rml::String mCurrentPath;
};

void show_data_folder_error_modal(std::string_view message) {
    auto dismiss = [](Modal& modal) {
        mDoAud_seStartMenu(kSoundWindowClose);
        modal.pop();
    };
    push_document(std::make_unique<Modal>(Modal::Props{
        .title = android_text("Data Folder Not Changed", "Dossier de données inchangé"),
        .bodyText = Rml::String{message},
        .actions =
            {
                ModalAction{
                    .label = "OK",
                    .onPressed = dismiss,
                },
            },
        .onDismiss = dismiss,
        .icon = "warning",
    }));
    if (auto* doc = top_document()) {
        doc->focus();
    }
}

void data_folder_dialog_callback(borealis::file_select::Result result) {
    if (result.status == borealis::file_select::Status::Canceled) {
        return;
    }
    if (result.status != borealis::file_select::Status::Selected || result.locations.empty()) {
        show_data_folder_error_modal(android_text("Dusklight could not open the folder picker.",
            "Dusklight n'a pas pu ouvrir le sélecteur de dossier."));
        return;
    }

    std::string dataPathError;
    if (data::set_custom_data_path(result.locations.front(), &dataPathError)) {
        mDoAud_seStartMenu(kSoundItemChange);
        return;
    }

    if (dataPathError.empty()) {
        dataPathError =
            fmt::format(android_text("{} could not use the selected folder as its data folder.",
                            "{} n'a pas pu utiliser le dossier sélectionné pour ses données."),
                AppName);
    }
    show_data_folder_error_modal(dataPathError);
}

const Rml::String kInternalResolutionHelpText = android_text(
    "Configure the resolution used for rendering the game. Higher values are more demanding on "
    "your graphics hardware.",
    "Définit la résolution de rendu du jeu. Les valeurs élevées sollicitent davantage le "
    "processeur graphique.");
const Rml::String kShadowResolutionHelpText = android_text(
    "Configure the shadow-map resolution. Higher values improve shadow quality but increase GPU "
    "and memory usage.",
    "Définit la résolution des ombres. Les valeurs élevées améliorent leur qualité, mais "
    "augmentent l'utilisation du processeur graphique et de la mémoire.");
const Rml::String kResamplerHelpText =
    android_text("Configure the sampling method used when scaling the internal resolution for "
                 "final presentation.",
        "Définit la méthode de rééchantillonnage utilisée pour adapter la résolution interne à "
        "l'affichage final.");
const Rml::String kBloomHelpText =
    android_text("Configure the post-processing bloom effect. Classic uses the original bloom "
                 "pass; Dusklight uses "
                 "a higher-quality bloom pass.",
        "Définit l'effet de halo lumineux. Classique utilise l'effet d'origine ; Dusklight utilise "
        "un effet de meilleure qualité.");
const Rml::String kBloomBrightnessHelpText =
    android_text("Configure bloom intensity. Higher values make bright areas glow more strongly.",
        "Définit l'intensité du halo lumineux. Les valeurs élevées accentuent la lueur des zones "
        "claires.");
const Rml::String kDepthOfFieldHelpText =
    android_text("Configure the post-processing depth-of-field effect. Classic uses the original "
                 "depth-of-field pass;"
                 " Dusklight uses a higher-quality depth-of-field pass.",
        "Définit l'effet de profondeur de champ. Classique utilise l'effet d'origine ; Dusklight "
        "utilise un effet de meilleure qualité.");
const Rml::String kUnlockFramerateHelpText =
    android_text("<br/>Uses inter-frame interpolation to enable higher frame rates.<br/><br/>May "
                 "introduce minor "
                 "visual artifacts or animation glitches.",
        "<br/>Utilise l'interpolation entre les images pour augmenter la fluidité.<br/><br/>Peut "
        "provoquer de légers défauts visuels ou d'animation.");
const Rml::String kTextureReplacementHelpText = android_text(
    "Enable installed texture replacements.", "Active les packs de textures installés.");

int float_setting_percent(ConfigVar<float>& var) {
    return static_cast<int>(var.getValue() * 100.0f + 0.5f);
}

bool gyro_enabled() {
    return getSettings().game.enableGyroAim || getSettings().game.enableGyroRollgoal;
}

Rml::String touch_targeting_label(TouchTargeting targeting) {
    const auto index = static_cast<size_t>(targeting);
    if (index >= kTouchTargetingLabels.size()) {
        return android_text("Unknown", "Inconnu");
    }
    return kTouchTargetingLabels[index];
}

struct ConfigBoolProps {
    Rml::String key;
    Rml::String icon;
    Rml::String helpText;
    std::function<void(bool)> onChange;
    std::function<bool()> isDisabled;
};

SelectButton& config_bool_select(
    Pane& leftPane, Pane& rightPane, ConfigVar<bool>& var, ConfigBoolProps props) {
    auto& button = leftPane.add_child<BoolButton>(BoolButton::Props{
        .key = std::move(props.key),
        .icon = std::move(props.icon),
        .getValue = [&var] { return var.getValue(); },
        .setValue =
            [&var, callback = std::move(props.onChange)](bool value) {
                if (value == var.getValue()) {
                    return;
                }
                var.setValue(value);
                config::save();
                if (callback) {
                    callback(value);
                }
            },
        .isDisabled = std::move(props.isDisabled),
        .isModified = [&var] { return var.getValue() != var.getDefaultValue(); },
    });
    leftPane.register_control(
        button, rightPane, [helpText = std::move(props.helpText)](Pane& pane) {
            pane.clear();
            pane.add_rml(helpText);
        });
    return button;
}

void add_speedrun_disabled_option(Pane& leftPane, Pane& rightPane, ConfigVar<bool>& var,
    const Rml::String& key, const Rml::String& helpText) {
    config_bool_select(leftPane, rightPane, var,
        {
            .key = key,
            .helpText = helpText,
            .isDisabled = [] { return speedrun::isActive(); },
        });
}

SelectButton& config_percent_select(Pane& leftPane, Pane& rightPane, ConfigVar<float>& var,
    Rml::String key, Rml::String helpText, int min, int max, int step = 5,
    std::function<bool()> isDisabled = {}) {
    auto& button = leftPane.add_child<NumberButton>(NumberButton::Props{
        .key = std::move(key),
        .getValue = [&var] { return float_setting_percent(var); },
        .setValue =
            [&var, min, max](int value) {
                var.setValue(std::clamp(value, min, max) / 100.0f);
                config::save();
            },
        .isDisabled = std::move(isDisabled),
        .isModified = [&var] { return var.getValue() != var.getDefaultValue(); },
        .min = min,
        .max = max,
        .step = step,
        .suffix = "%",
    });
    leftPane.register_control(button, rightPane, [helpText = std::move(helpText)](Pane& pane) {
        pane.clear();
        pane.add_rml(helpText);
    });
    return button;
}

SelectButton& config_int_select(Pane& leftPane, Pane& rightPane, ConfigVar<int>& var,
    Rml::String key, Rml::String helpText, int min, int max, int step = 5,
    std::function<bool()> isDisabled = {}, std::function<void(int)> onChange = {},
    std::string suffix = "") {
    auto& button = leftPane.add_child<NumberButton>(NumberButton::Props{
        .key = std::move(key),
        .getValue = [&var] { return var.getValue(); },
        .setValue =
            [&var, min, max, callback = std::move(onChange)](int value) {
                const int clampedValue = std::clamp(value, min, max);
                var.setValue(clampedValue);
                config::save();
                if (callback) {
                    callback(clampedValue);
                }
            },
        .isDisabled = std::move(isDisabled),
        .isModified = [&var] { return var.getValue() != var.getDefaultValue(); },
        .min = min,
        .max = max,
        .step = step,
        .suffix = suffix,
    });
    leftPane.register_control(button, rightPane, [helpText = std::move(helpText)](Pane& pane) {
        pane.clear();
        pane.add_text(helpText);
    });
    return button;
}

void graphics_tuner_control(
    Window& window, Pane& leftPane, Pane& rightPane, const GraphicsTunerProps& props) {
    const auto setting = GraphicsSetting::of(props.option);
    leftPane.register_control(leftPane
                                  .add_select_button({
                                      .key = props.title,
                                      .getValue = [setting] { return setting.text(); },
                                      .isModified = [setting] { return setting.isModified(); },
                                      .submit = false,
                                  })
                                  .on_nav_command([&window, props](Rml::Event&, NavCommand cmd) {
                                      if (cmd == NavCommand::Confirm || cmd == NavCommand::Left ||
                                          cmd == NavCommand::Right) {
                                          window.push(std::make_unique<GraphicsTuner>(props));
                                          return true;
                                      }
                                      return false;
                                  }),
        rightPane, [helpText = props.helpText](Pane& pane) {
            pane.clear();
            pane.add_text(helpText);
        });
}

}  // namespace

SettingsWindow::SettingsWindow(bool prelaunch) : mPrelaunch(prelaunch) {
    if (prelaunch) {
        add_tab(android_text("Prelaunch", "Démarrage"), [this](Rml::Element* content) {
            auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
            auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

            leftPane.register_control(
                leftPane
                    .add_select_button({
                        .key = android_text("Disc Image", "Image du disque"),
                        .getValue =
                            [] {
                                const auto& path = prelaunch_state().configuredDiscPath;
                                std::string display;
                                if (path.empty()) {
                                    display = android_text("(none)", "(aucun)");
                                } else {
                                    display = borealis::io::display_name(path);
                                    if (display.empty()) {
                                        display = path;
                                    }
                                }
                                return display;
                            },
                        .isModified =
                            [] {
                                const auto& state = prelaunch_state();
                                const auto& active = state.activeDiscPath;
                                return !active.empty() && state.configuredDiscPath != active;
                            },
                    })
                    .on_pressed([] { open_iso_picker(); }),
                rightPane, [](Pane& pane) {
                    pane.add_rml(android_text(
                        "Set the disc image that Dusklight uses to launch the game.<br/><br/>"
                        "Changes require a restart.",
                        "Choisissez l'image du disque utilisée par Dusklight pour lancer le "
                        "jeu.<br/><br/>Un redémarrage est nécessaire pour appliquer les "
                        "changements."));
                });
            if (data::manager().capabilities().canChangeLocation &&
                borealis::file_select::capabilities().canOpenFolder)
            {
                leftPane.register_control(
                    leftPane.add_select_button({
                        .key = android_text("Data Folder", "Dossier de données"),
                        .getValue = [] { return configured_data_path_display_name(); },
                        .isModified = [] { return data::is_data_path_restart_pending(); },
                    }),
                    rightPane, [](Pane& pane) {
                        pane.add_text(android_text(
                            "The data folder is where Dusklight stores settings, saves, "
                            "logs, texture replacements, and other app data.",
                            "Dusklight stocke dans ce dossier les paramètres, sauvegardes, "
                            "journaux, packs de textures et autres données de l'application."));
                        pane.add_child<DataFolderPathText>();
#if DUSK_CAN_OPEN_DATA_FOLDER
                        pane.add_button(
                                android_text("Open Data Folder", "Ouvrir le dossier de données"))
                            .on_pressed([] {
                                if (data::open_data_path()) {
                                    mDoAud_seStartMenu(kSoundClick);
                                }
                            });
#endif
                        pane.add_button(
                                android_text("Change Data Folder", "Changer le dossier de données"))
                            .on_pressed([] {
                                const auto defaultLocation =
                                    borealis::io::fs_path_to_string(data::configured_data_path());
                                borealis::file_select::open_folder(
                                    {
                                        .parentWindow = aurora::window::get_sdl_window(),
                                        .defaultLocation = defaultLocation,
                                        .requireRealPath = true,
                                    },
                                    &data_folder_dialog_callback);
                            });
#if defined(_WIN32)
                        pane.add_button(android_text("Portable Mode", "Mode portable"))
                            .on_pressed([] {
                                if (data::set_portable_data_path()) {
                                    mDoAud_seStartMenu(kSoundItemChange);
                                }
                            });
#endif
                        pane.add_button(
                                {
                                    .text = android_text(
                                        "Reset to Default", "Rétablir les valeurs par défaut"),
                                    .isDisabled = [] { return data::is_default_data_path(); },
                                })
                            .on_pressed([] {
                                if (data::reset_data_path()) {
                                    mDoAud_seStartMenu(kSoundItemChange);
                                }
                            });
                        pane.add_rml(android_text("Data will be migrated automatically on restart.",
                            "Les données seront transférées automatiquement au redémarrage."));
                    });
            }
            leftPane.register_control(
                leftPane.add_select_button({
                    .key = android_text("Language", "Langue du jeu"),
                    .getValue =
                        [] {
                            return language::language_name(getSettings().game.language.getValue());
                        },
                    .isDisabled =
                        [] {
                            const auto& state = prelaunch_state();
                            if (!state.configuredDiscCanLaunch) {
                                return true;
                            }
                            return language::available_languages(state.configuredDiscInfo).size() <=
                                   1;
                        },
                    .isModified =
                        [] {
                            return getSettings().game.language.getValue() !=
                                   prelaunch_state().initialLanguage;
                        },
                }),
                rightPane, [](Pane& pane) {
                    const auto& state = prelaunch_state();
                    const auto languages =
                        state.configuredDiscCanLaunch ?
                            language::available_languages(state.configuredDiscInfo) :
                            language::available_languages({});
                    for (const GameLanguage language : languages) {
                        pane.add_button({
                                            .text = language::language_name(language),
                                            .isSelected =
                                                [language] {
                                                    return getSettings().game.language.getValue() ==
                                                           language;
                                                },
                                        })
                            .on_pressed([language] {
                                mDoAud_seStartMenu(kSoundItemChange);
                                getSettings().game.language.setValue(language);
                                config::save();
                            });
                    }
                    pane.add_rml(android_text("<br/>Changes require a restart.",
                        "<br/>Un redémarrage est nécessaire pour appliquer les changements."));
                });
            leftPane.register_control(
                leftPane.add_select_button({
                    .key = android_text("Graphics Backend", "Moteur graphique"),
                    .getValue = [] { return graphics_backend_display_name(); },
                    .isModified = [] { return is_graphics_backend_restart_pending(); },
                }),
                rightPane, [](Pane& pane) {
                    const auto availableBackends = available_backends();
                    for (const auto backend : availableBackends) {
                        pane
                            .add_button({
                                .text = Rml::String{backend_name(backend)},
                                .isSelected = [backend] { return configured_backend() == backend; },
                            })
                            .on_pressed([backend] {
                                mDoAud_seStartMenu(kSoundItemChange);
                                getSettings().backend.graphicsBackend.setValue(
                                    std::string{backend_id(backend)});
                                config::save();
                            });
                    }
                    pane.add_rml(android_text("<br/>Changes require a restart.",
                        "<br/>Un redémarrage est nécessaire pour appliquer les changements."));
                });
            leftPane.register_control(
                leftPane.add_select_button({
                    .key = android_text("Save File Type", "Format des sauvegardes"),
                    .getValue =
                        [] {
                            return kCardFileTypes[getSettings().backend.cardFileType.getValue()];
                        },
                    .isModified =
                        [] {
                            return getSettings().backend.cardFileType.getValue() !=
                                   prelaunch_state().initialCardFileType;
                        },
                }),
                rightPane, [](Pane& pane) {
                    for (int i = 0; i < kCardFileTypes.size(); i++) {
                        pane
                            .add_button({
                                .text = kCardFileTypes[i],
                                .isSelected =
                                    [i] {
                                        return getSettings().backend.cardFileType.getValue() == i;
                                    },
                            })
                            .on_pressed([i] {
                                mDoAud_seStartMenu(kSoundItemChange);
                                getSettings().backend.cardFileType.setValue(i);
                                config::save();
                            });
                    }
                });
            add_save_files_control(leftPane, rightPane);
        });
    }

    add_tab(android_text("Video", "Vidéo"), [this](Rml::Element* content) {
        auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
        auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

        leftPane.add_section(android_text("Display", "Affichage"));

        leftPane.register_control(
            leftPane.add_button(android_text("Toggle Fullscreen", "Basculer en plein écran"))
                .on_pressed([] {
                    mDoAud_seStartMenu(kSoundItemChange);
                    getSettings().video.enableFullscreen.setValue(
                        !getSettings().video.enableFullscreen);
                    VISetWindowFullscreen(getSettings().video.enableFullscreen);
                    config::save();
                }),
            rightPane, [](Pane& pane) { pane.clear(); });
        leftPane.register_control(leftPane
                                      .add_button(android_text("Restore Default Window Size",
                                          "Rétablir la taille de fenêtre par défaut"))
                                      .on_pressed([] {
                                          mDoAud_seStartMenu(kSoundItemChange);
                                          getSettings().video.enableFullscreen.setValue(false);
                                          VISetWindowFullscreen(false);
                                          VISetWindowSize(FB_WIDTH * 2, FB_HEIGHT * 2);
                                          VICenterWindow();
                                      }),
            rightPane, [](Pane& pane) { pane.clear(); });
        config_bool_select(leftPane, rightPane, getSettings().video.enableVsync,
            {
                .key = android_text("Enable VSync", "Synchronisation verticale"),
                .helpText =
                    android_text("Synchronizes the frame rate to your monitor's refresh rate.",
                        "Synchronise la fréquence des images avec celle de l'écran."),
                .onChange = [](bool value) { aurora_enable_vsync(value); },
            });
        config_bool_select(leftPane, rightPane, getSettings().video.lockAspectRatio,
            {
                .key = android_text("Lock 4:3 Aspect Ratio", "Verrouiller le format 4:3"),
                .helpText = android_text("Lock the game's aspect ratio to the original.",
                    "Conserve le format d'image d'origine du jeu."),
                .onChange =
                    [](bool value) {
                        AuroraSetViewportPolicy(
                            value ? AURORA_VIEWPORT_FIT : AURORA_VIEWPORT_STRETCH);
                    },
            });
        config_bool_select(leftPane, rightPane, getSettings().game.pauseOnFocusLost,
            {
                .key = android_text("Pause on Focus Lost", "Pause lorsque la fenêtre est inactive"),
                .helpText = android_text("Pause the game when window focus is lost.",
                    "Met le jeu en pause lorsque sa fenêtre devient inactive."),
                .isDisabled = [] { return IsMobile || speedrun::isActive(); },
            });
        leftPane.register_control(
            leftPane.add_select_button({
                .key = android_text("Show FPS Counter", "Compteur d'images par seconde"),
                .getValue =
                    [] {
                        if (!getSettings().video.enableFpsOverlay.getValue()) {
                            return Rml::String{android_text("Off", "Désactivé")};
                        }
                        const int idx = getSettings().video.fpsOverlayCorner.getValue();
                        return Rml::String{kFpsOverlayCornerNames[idx]};
                    },
                .isModified =
                    [] {
                        const auto& enable = getSettings().video.enableFpsOverlay;
                        const auto& corner = getSettings().video.fpsOverlayCorner;
                        return enable.getValue() != enable.getDefaultValue() ||
                               (enable.getValue() && corner.getValue() != corner.getDefaultValue());
                    },
            }),
            rightPane, [](Pane& pane) {
                pane.add_button(
                        {
                            .text = android_text("Off", "Désactivé"),
                            .isSelected =
                                [] { return !getSettings().video.enableFpsOverlay.getValue(); },
                        })
                    .on_pressed([] {
                        mDoAud_seStartMenu(kSoundItemChange);
                        getSettings().video.enableFpsOverlay.setValue(false);
                        config::save();
                    });
                for (int i = 0; i < static_cast<int>(kFpsOverlayCornerNames.size()); ++i) {
                    pane.add_button(
                            {
                                .text = kFpsOverlayCornerNames[i],
                                .isSelected =
                                    [i] {
                                        return getSettings().video.enableFpsOverlay.getValue() &&
                                               getSettings().video.fpsOverlayCorner.getValue() == i;
                                    },
                            })
                        .on_pressed([i] {
                            mDoAud_seStartMenu(kSoundItemChange);
                            getSettings().video.enableFpsOverlay.setValue(true);
                            getSettings().video.fpsOverlayCorner.setValue(i);
                            config::save();
                        });
                }
                pane.add_rml(android_text(
                    "<br/>Display the current framerate in a corner of the screen while playing.",
                    "<br/>Affiche le nombre d'images par seconde dans un coin de l'écran pendant "
                    "le jeu."));
            });
        config_bool_select(leftPane, rightPane, getSettings().video.rememberWindowSize,
            {
                .key = android_text("Remember Window Size", "Mémoriser la taille de la fenêtre"),
                .helpText = android_text(
                    "Save and restore the previous session's window size when opening Dusklight.",
                    "Enregistre et rétablit la taille de la fenêtre de la session précédente à "
                    "l'ouverture de Dusklight."),
                .onChange =
                    [](bool value) {
                        if (value && !getSettings().video.enableFullscreen) {
                            const auto windowSize = aurora::window::get_window_size();
                            getSettings().video.lastWindowWidth.setValue(windowSize.width);
                            getSettings().video.lastWindowHeight.setValue(windowSize.height);
                            config::save();
                        }
                    },
                .isDisabled = [] { return IsMobile; },
            });

        config_int_select(leftPane, rightPane, getSettings().video.uiScale,
            android_text("UI Scale", "Taille de l'interface"),
            android_text("Scales the Dusklight interface relative to the display's DPI scale. Has "
                         "no effect on the game's UI and HUD.",
                "Ajuste la taille de l'interface Dusklight selon la densité de l'écran. N'affecte "
                "pas les menus ni l'affichage tête haute du jeu."),
            50, 200, 25, {}, {}, "%");

        leftPane.add_section(android_text("Resolution", "Résolution"));
        graphics_tuner_control(*this, leftPane, rightPane,
            GraphicsTunerProps{
                .option = GraphicsOption::InternalResolution,
                .title = android_text("Internal Resolution", "Résolution interne"),
                .helpText = kInternalResolutionHelpText,
            });
        graphics_tuner_control(*this, leftPane, rightPane,
            GraphicsTunerProps{
                .option = GraphicsOption::ShadowResolution,
                .title = android_text("Shadow Resolution", "Résolution des ombres"),
                .helpText = kShadowResolutionHelpText,
            });
        graphics_tuner_control(*this, leftPane, rightPane,
            GraphicsTunerProps{
                .option = GraphicsOption::Resampler,
                .title = android_text("Output Resampling", "Rééchantillonnage de sortie"),
                .helpText = kResamplerHelpText,
            });

        leftPane.add_section(android_text("Post-Processing", "Post-traitement"));
        graphics_tuner_control(*this, leftPane, rightPane,
            GraphicsTunerProps{
                .option = GraphicsOption::BloomMode,
                .title = android_text("Bloom", "Halo lumineux"),
                .helpText = kBloomHelpText,
            });
        graphics_tuner_control(*this, leftPane, rightPane,
            GraphicsTunerProps{
                .option = GraphicsOption::BloomMultiplier,
                .title = android_text("Bloom Brightness", "Intensité du halo"),
                .helpText = kBloomBrightnessHelpText,
            });
        graphics_tuner_control(*this, leftPane, rightPane,
            GraphicsTunerProps{
                .option = GraphicsOption::DepthOfFieldMode,
                .title = android_text("Depth of Field", "Profondeur de champ"),
                .helpText = kDepthOfFieldHelpText,
            });

        leftPane.add_section(android_text("Rendering", "Rendu"));
        graphics_tuner_control(*this, leftPane, rightPane,
            GraphicsTunerProps{
                .option = GraphicsOption::TextureReplacements,
                .title =
                    android_text("Enable Texture Replacements", "Activer les packs de textures"),
                .helpText = kTextureReplacementHelpText,
            });
        leftPane.register_control(
            leftPane.add_select_button({
                .key = android_text("Unlock Framerate", "Débloquer la fréquence des images"),
                .getValue =
                    [] {
                        return kInterpolationModes[static_cast<u8>(
                            getSettings().game.enableFrameInterpolation.getValue())];
                    },
                .isModified =
                    [] {
                        return getSettings().game.enableFrameInterpolation.getValue() !=
                               getSettings().game.enableFrameInterpolation.getDefaultValue();
                    },
            }),
            rightPane, [](Pane& pane) {
                for (int i = 0; i < kInterpolationModes.size(); i++) {
                    pane
                        .add_button({
                            .text = kInterpolationModes[i],
                            .isSelected =
                                [i] {
                                    return getSettings().game.enableFrameInterpolation.getValue() ==
                                           static_cast<FrameInterpMode>(i);
                                },
                        })
                        .on_pressed([i] {
                            mDoAud_seStartMenu(kSoundItemChange);
                            getSettings().game.enableFrameInterpolation.setValue(
                                static_cast<FrameInterpMode>(i));
                            presentation::update_frame_rate_preference();
                            config::save();
                        });
                }
                pane.add_rml(kUnlockFramerateHelpText);
            });
        config_int_select(
            leftPane, rightPane, getSettings().video.maxFrameRate,
            android_text("Framerate Cap", "Limite d'images par seconde"),
            android_text("Limit the framerate to the specified value.",
                "Limite le nombre d'images par seconde à la valeur indiquée."),
            30, 540, 1,
            [] {
                return getSettings().game.enableFrameInterpolation.getValue() !=
                       FrameInterpMode::Capped;
            },
            [](int) { presentation::update_frame_rate_preference(); });
        config_bool_select(leftPane, rightPane, getSettings().game.enableMapBackground,
            {.key = android_text("Enable Mini-Map Shadows", "Ombres de la mini-carte"),
                .helpText = android_text(
                    "Render a thick shadow around the mini-map. May impact performance.",
                    "Affiche une ombre épaisse autour de la mini-carte. Peut réduire les "
                    "performances.")});
        config_bool_select(leftPane, rightPane, getSettings().game.disableCutscenePillarboxing,
            {
                .key =
                    android_text("Disable Cutscene Pillarboxing", "Supprimer les bandes latérales"),
                .helpText =
                    android_text("Disable black bars on the left and right sides of the screen "
                                 "during some cutscenes, particularly on ultra-wide displays. "
                                 "Visuals beyond the original intended framing may appear buggy.",
                        "Supprime les bandes noires latérales de certaines cinématiques, notamment "
                        "sur les écrans très larges. Des défauts visuels peuvent apparaître "
                        "au-delà du cadrage d'origine."),
            });
        leftPane.register_control(
            leftPane.add_select_button({
                .key = android_text("Disable Letterboxing", "Supprimer les bandes horizontales"),
                .getValue =
                    [] {
                        return kLetterboxModes[static_cast<u8>(
                            getSettings().game.disableLetterboxing.getValue())];
                    },
                .isModified =
                    [] {
                        return getSettings().game.disableLetterboxing.getValue() !=
                               getSettings().game.disableLetterboxing.getDefaultValue();
                    },
            }),
            rightPane, [](Pane& pane) {
                for (int i = 0; i < static_cast<int>(kLetterboxModes.size()); i++) {
                    pane.add_button(
                            {
                                .text = kLetterboxModes[i],
                                .isSelected =
                                    [i] {
                                        return getSettings().game.disableLetterboxing.getValue() ==
                                               static_cast<LetterboxMode>(i);
                                    },
                            })
                        .on_pressed([i] {
                            mDoAud_seStartMenu(kSoundItemChange);
                            getSettings().game.disableLetterboxing.setValue(
                                static_cast<LetterboxMode>(i));
                            config::save();
                        });
                }
                pane.add_rml(android_text(
                    "<br/>Disable the top and bottom black bars during L-targeting, aiming, "
                    "cutscenes, dialogue, etc.",
                    "<br/>Supprime les bandes noires en haut et en bas pendant le ciblage L, la "
                    "visée, les cinématiques, les dialogues, etc."));
            });
    });

    add_tab(android_text("Input", "Commandes"), [this](Rml::Element* content) {
        auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
        auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

        auto addOption = [&](const Rml::String& key, ConfigVar<bool>& value,
                             const Rml::String& helpText, std::function<bool()> isDisabled = {}) {
            config_bool_select(leftPane, rightPane, value,
                {
                    .key = key,
                    .helpText = helpText,
                    .isDisabled = std::move(isDisabled),
                });
        };

        leftPane.add_section(android_text("Inputs", "Commandes"));
        leftPane.register_control(
            leftPane
                .add_group_button(
                    {.text = android_text("Configure Inputs", "Configurer les commandes")})
                .on_pressed([this] { push(std::make_unique<ControllerConfigWindow>()); }),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_text(android_text("Open input binding configuration.",
                    "Ouvre la configuration des touches et boutons."));
            });
        config_bool_select(leftPane, rightPane, getSettings().game.allowBackgroundInput,
            {
                .key = android_text("Allow Background Inputs", "Commandes en arrière-plan"),
                .helpText = android_text("Allow inputs even when the game window is not focused.",
                    "Accepte les commandes même lorsque la fenêtre du jeu est inactive."),
                .onChange = [](bool value) { aurora_set_background_input(value); },
            });

#if TOUCH_CONTROLS_AVAILABLE
        leftPane.add_section(android_text("Touch", "Écran tactile"));
        addOption(android_text("Touch Controls", "Commandes tactiles"),
            getSettings().game.enableTouchControls,
            android_text(
                "Enables controls overlay for touch screens.<br/><br/>Press and drag on the left "
                "side "
                "of the screen to move, and on the right side of the screen to control the camera.",
                "Affiche les commandes tactiles.<br/><br/>Faites glisser votre doigt à gauche pour "
                "vous déplacer et à droite pour contrôler la caméra."));
        auto& customizeTouchLayout = leftPane.add_group_button(GroupButton::Props{
            .text = android_text("Customize Layout", "Personnaliser la disposition"),
            .isDisabled = [] { return !getSettings().game.enableTouchControls; },
        });
        leftPane.register_control(customizeTouchLayout.on_pressed(
                                      [this] { push(std::make_unique<TouchControlsEditor>()); }),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_text(android_text("Open the touch controls layout editor.",
                    "Ouvre l'éditeur de disposition des commandes tactiles."));
            });
        leftPane.register_control(
            leftPane.add_select_button({
                .key = android_text("Touch Targeting", "Ciblage tactile"),
                .getValue =
                    [] {
                        return touch_targeting_label(getSettings().game.touchTargeting.getValue());
                    },
                .isDisabled = [] { return !getSettings().game.enableTouchControls; },
                .isModified =
                    [] {
                        const auto& targeting = getSettings().game.touchTargeting;
                        return targeting.getValue() != targeting.getDefaultValue();
                    },
            }),
            rightPane, [](Pane& pane) {
                pane.clear();
                for (int i = 0; i < static_cast<int>(kTouchTargetingLabels.size()); ++i) {
                    pane.add_button(
                            {
                                .text = kTouchTargetingLabels[i],
                                .isSelected =
                                    [i] {
                                        return getSettings().game.touchTargeting.getValue() ==
                                               static_cast<TouchTargeting>(i);
                                    },
                            })
                        .on_pressed([i] {
                            mDoAud_seStartMenu(kSoundItemChange);
                            getSettings().game.touchTargeting.setValue(
                                static_cast<TouchTargeting>(i));
                            config::save();
                        });
                }
                pane.add_rml(
                    fmt::format(android_text("<br/>Hybrid: {}<br/>Hold: {}<br/>Switch: {}",
                                    "<br/>Hybride : {}<br/>Maintenir : {}<br/>Basculer : {}"),
                        kTouchTargetingDescriptions[0], kTouchTargetingDescriptions[1],
                        kTouchTargetingDescriptions[2]));
            });
        config_percent_select(leftPane, rightPane, getSettings().game.touchCameraXSensitivity,
            android_text("Touch Camera X Sensitivity", "Sensibilité horizontale tactile"),
            android_text("Adjusts touch camera horizontal sensitivity.<br/><br/>Applies to touch "
                         "input only.",
                "Règle la sensibilité horizontale de la caméra.<br/><br/>S'applique uniquement aux "
                "commandes tactiles."),
            25, 400, 5, [] { return !getSettings().game.enableTouchControls; });
        config_percent_select(leftPane, rightPane, getSettings().game.touchCameraYSensitivity,
            android_text("Touch Camera Y Sensitivity", "Sensibilité verticale tactile"),
            android_text(
                "Adjusts touch camera vertical sensitivity.<br/><br/>Applies to touch input only.",
                "Règle la sensibilité verticale de la caméra.<br/><br/>S'applique uniquement aux "
                "commandes tactiles."),
            25, 400, 5, [] { return !getSettings().game.enableTouchControls; });
#endif

        leftPane.add_section(android_text("Camera", "Caméra"));
        addOption(android_text("Free Camera", "Caméra libre"), getSettings().game.freeCamera,
            android_text("Enables free camera control, letting you control the camera fully with "
                         "the C-Stick.",
                "Permet de contrôler librement la caméra avec le stick C."));
        config_percent_select(leftPane, rightPane, getSettings().game.freeCameraXSensitivity,
            android_text("Free Camera X Sensitivity", "Sensibilité horizontale de la caméra libre"),
            android_text("Adjusts horizontal free camera sensitivity.<br/><br/>Applies to the "
                         "control stick only.",
                "Règle la sensibilité horizontale de la caméra libre.<br/><br/>S'applique "
                "uniquement au stick."),
            50, 200, 5, [] { return !getSettings().game.freeCamera; });
        config_percent_select(leftPane, rightPane, getSettings().game.freeCameraYSensitivity,
            android_text("Free Camera Y Sensitivity", "Sensibilité verticale de la caméra libre"),
            android_text("Adjusts vertical free camera sensitivity.<br/><br/>Applies to the "
                         "control stick only.",
                "Règle la sensibilité verticale de la caméra libre.<br/><br/>S'applique uniquement "
                "au stick."),
            50, 200, 5, [] { return !getSettings().game.freeCamera; });
        addOption(android_text("Invert Camera X Axis", "Inverser l'axe horizontal de la caméra"),
            getSettings().game.invertCameraXAxis,
            android_text(
                "Invert horizontal camera movement.<br/><br/>Applies to the control stick only.",
                "Inverse les mouvements horizontaux de la caméra.<br/><br/>S'applique uniquement "
                "au stick."));
        addOption(android_text("Invert Camera Y Axis", "Inverser l'axe vertical de la caméra"),
            getSettings().game.invertCameraYAxis,
            android_text(
                "Invert vertical camera movement.<br/><br/>Applies to the control stick only.",
                "Inverse les mouvements verticaux de la caméra.<br/><br/>S'applique uniquement au "
                "stick."),
            [] { return !getSettings().game.freeCamera; });
        addOption(
            android_text("Invert First Person X Axis", "Inverser la vue subjective horizontale"),
            getSettings().game.invertFirstPersonXAxis,
            android_text("Invert horizontal movement while aiming with items or first person "
                         "camera.<br/><br/>Applies to the control stick only.",
                "Inverse les mouvements horizontaux pendant la visée ou en vue "
                "subjective.<br/><br/>S'applique uniquement au stick."));
        addOption(
            android_text("Invert First Person Y Axis", "Inverser la vue subjective verticale"),
            getSettings().game.invertFirstPersonYAxis,
            android_text("Invert vertical movement while aiming with items or first person "
                         "camera.<br/><br/>Applies to the control stick only.",
                "Inverse les mouvements verticaux pendant la visée ou en vue "
                "subjective.<br/><br/>S'applique uniquement au stick."));

        leftPane.add_section(android_text("Gyro", "Gyroscope"));
        addOption(android_text("Gyro Aim", "Visée gyroscopique"), getSettings().game.enableGyroAim,
            android_text(
                "Enables gyro controls while in look mode, aiming a hawk, and aiming "
                "supported items.<br/><br/>Supported items include the Slingshot, Gale Boomerang, "
                "Hero's Bow, Clawshot(s), Ball and Chain, and Dominion Rod.",
                "Active le gyroscope en vue subjective, pour diriger un faucon et pour viser avec "
                "les objets compatibles.<br/><br/>Objets compatibles : lance-pierres, boomerang "
                "tornade, arc du Héros, grappin(s), boulet et bâton Anima."));
        addOption(android_text("Gyro Rollgoal", "Gyroscope pour Kororinpa"),
            getSettings().game.enableGyroRollgoal,
            android_text("Enables gyro controls for Rollgoal in Hena's Cabin.",
                "Active le gyroscope pour Kororinpa dans la cabane de Hena."));
        config_percent_select(leftPane, rightPane, getSettings().game.gyroSensitivityY,
            android_text("Gyro Pitch Sensitivity", "Sensibilité verticale du gyroscope"),
            android_text("Controls vertical gyro aiming sensitivity.",
                "Règle la sensibilité verticale de la visée gyroscopique."),
            25, 400, 5, [] { return !gyro_enabled(); });
        config_percent_select(leftPane, rightPane, getSettings().game.gyroSensitivityX,
            android_text("Gyro Yaw Sensitivity", "Sensibilité horizontale du gyroscope"),
            android_text("Controls horizontal gyro aiming sensitivity.",
                "Règle la sensibilité horizontale de la visée gyroscopique."),
            25, 400, 5, [] { return !gyro_enabled(); });
        config_percent_select(leftPane, rightPane, getSettings().game.gyroSensitivityRollgoal,
            android_text("Rollgoal Sensitivity", "Sensibilité de Kororinpa"),
            android_text("Controls how strongly gyro input tilts the Rollgoal table.",
                "Règle l'inclinaison du plateau de Kororinpa en fonction des mouvements du "
                "gyroscope."),
            25, 400, 5, [] { return !getSettings().game.enableGyroRollgoal; });
        config_percent_select(leftPane, rightPane, getSettings().game.gyroDeadband,
            android_text("Gyro Deadband", "Zone morte du gyroscope"),
            android_text("Ignores small gyro movement to reduce drift and jitter.",
                "Ignore les petits mouvements du gyroscope pour réduire la dérive et les "
                "tremblements."),
            0, 50, 1, [] { return !gyro_enabled(); });
        config_percent_select(leftPane, rightPane, getSettings().game.gyroSmoothing,
            android_text("Gyro Smoothing", "Lissage du gyroscope"),
            android_text("Higher values smooth gyro input over time.",
                "Les valeurs élevées lissent davantage les mouvements du gyroscope."),
            0, 100, 1, [] { return !gyro_enabled(); });
        addOption(android_text("Invert Gyro Pitch", "Inverser le gyroscope vertical"),
            getSettings().game.gyroInvertPitch,
            android_text(
                "Invert vertical gyro aiming.", "Inverse la visée gyroscopique verticale."),
            [] { return !gyro_enabled(); });
        addOption(android_text("Invert Gyro Yaw", "Inverser le gyroscope horizontal"),
            getSettings().game.gyroInvertYaw,
            android_text(
                "Invert horizontal gyro aiming.", "Inverse la visée gyroscopique horizontale."),
            [] { return !gyro_enabled(); });

        leftPane.add_section(android_text("Mouse", "Souris"));
        addOption(android_text("Mouse Aim", "Visée à la souris"), getSettings().game.enableMouseAim,
            android_text(
                "Enables mouse input while in look mode, aiming a hawk, and aiming "
                "supported items.<br/><br/>Supported items include the Slingshot, Gale Boomerang, "
                "Hero's Bow, Clawshot(s), Ball and Chain, and Dominion Rod.",
                "Active la souris en vue subjective, pour diriger un faucon et pour viser avec les "
                "objets compatibles.<br/><br/>Objets compatibles : lance-pierres, boomerang "
                "tornade, arc du Héros, grappin(s), boulet et bâton Anima."));
        addOption(android_text("Mouse Camera", "Caméra à la souris"),
            getSettings().game.enableMouseCamera,
            android_text("Enables mouse input for controlling the third-person camera.",
                "Permet de contrôler la caméra à la troisième personne avec la souris."));
        config_percent_select(leftPane, rightPane, getSettings().game.mouseAimSensitivity,
            android_text("Mouse Aim Sensitivity", "Sensibilité de visée à la souris"),
            android_text(
                "Controls mouse aim sensitivity.", "Règle la sensibilité de visée à la souris."),
            25, 400, 5, [] { return !getSettings().game.enableMouseAim; });
        config_percent_select(leftPane, rightPane, getSettings().game.mouseCameraSensitivity,
            android_text("Mouse Camera Sensitivity", "Sensibilité de la caméra à la souris"),
            android_text("Controls mouse camera sensitivity.",
                "Règle la sensibilité de la caméra à la souris."),
            25, 400, 5, [] { return !getSettings().game.enableMouseCamera; });
        addOption(android_text("Invert Mouse Y", "Inverser l'axe vertical de la souris"),
            getSettings().game.invertMouseY,
            android_text("Invert vertical mouse control for both aiming and camera.",
                "Inverse l'axe vertical de la souris pour la visée et la caméra."),
            [] {
                return !getSettings().game.enableMouseAim || !getSettings().game.enableMouseCamera;
            });

        leftPane.add_section(android_text("Gameplay", "Jouabilité"));
        addOption(android_text("Mouse/Touch in Menus", "Souris et tactile dans les menus"),
            getSettings().game.enableMenuPointer,
            android_text("Enables mouse and touch input for supported in-game menus.",
                "Active la souris et l'écran tactile dans les menus du jeu compatibles."));
        addOption(
            android_text("Invert Air/Swim X Axis", "Inverser l'axe horizontal en vol et nage"),
            getSettings().game.invertAirSwimX,
            android_text("Invert horizontal movement while flying or swimming.",
                "Inverse les mouvements horizontaux en vol et à la nage."));
        addOption(android_text("Invert Air/Swim Y Axis", "Inverser l'axe vertical en vol et nage"),
            getSettings().game.invertAirSwimY,
            android_text("Invert vertical movement while flying or swimming.",
                "Inverse les mouvements verticaux en vol et à la nage."));
        addOption(android_text("Swap Direct Select Input", "Inverser la sélection directe"),
            getSettings().game.swapDirectSelect,
            android_text("Swap the controls for using Direct Select on the item wheel, making "
                         "Direct Select the default and holding L to scroll the wheel.",
                "Inverse les commandes de la roue des objets : la sélection directe devient le "
                "mode par défaut et maintenir L fait défiler la roue."));

        leftPane.add_section(android_text("Tools", "Outils"));
        addOption(android_text("Turbo Key", "Touche turbo"), getSettings().game.enableTurboKeybind,
            android_text("Hold Tab to increase game speed by up to 4x.",
                "Maintenez Tab pour accélérer le jeu jusqu'à 4 fois."),
            [] { return speedrun::isActive(); });
        addOption(android_text("Reset Key (", "Touche de réinitialisation (") +
                      Rml::String{hotkeys::DO_RESET} + ")",
            getSettings().game.enableResetKeybind,
            android_text("Press ", "Appuyez sur ") + Rml::String{hotkeys::DO_RESET} +
                android_text(" to reset the game.", " pour réinitialiser le jeu."));
    });

    add_tab("Audio", [this](Rml::Element* content) {
        auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
        auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

        leftPane.add_section(android_text("Output", "Sortie"));
        leftPane.register_control(
            leftPane.add_select_button({
                .key = android_text("Output Mode", "Mode de sortie"),
                .getValue =
                    [] {
                        const auto idx =
                            static_cast<int>(getSettings().audio.outputMode.getValue());
                        return Rml::String{kAudioOutputModeNames[idx]};
                    },
                .isModified =
                    [] {
                        const auto& setting = getSettings().audio.outputMode;
                        return setting.getValue() != setting.getDefaultValue();
                    },
            }),
            rightPane, [](Pane& pane) {
                for (int i = 0; i < static_cast<int>(kAudioOutputModeNames.size()); ++i) {
                    pane
                        .add_button({
                            .text = kAudioOutputModeNames[i],
                            .isSelected =
                                [i] {
                                    const auto& setting = getSettings().audio.outputMode;
                                    return setting.getValue() == static_cast<AudioOutputMode>(i);
                                },
                        })
                        .on_pressed([i] {
                            mDoAud_seStartMenu(kSoundItemChange);
                            getSettings().audio.outputMode.setValue(
                                static_cast<AudioOutputMode>(i));
                            config::save();
                            audio::Reinitialize();
                        });
                }
            });

        // TODO: Individual sliders for Sub Music, Sound Effects, and Fanfare.
        leftPane.add_section("Volume");
        leftPane.register_control(
            leftPane.add_child<NumberButton>(NumberButton::Props{
                .key = android_text("Master Volume", "Volume général"),
                .getValue = [] { return getSettings().audio.masterVolume.getValue(); },
                .setValue =
                    [](int value) {
                        getSettings().audio.masterVolume.setValue(value);
                        config::save();
                        audio::SetMasterVolume(audio::MasterVolumeToLinear(value / 100.0f));
                    },
                .isModified =
                    [] {
                        return getSettings().audio.masterVolume.getValue() !=
                               getSettings().audio.masterVolume.getDefaultValue();
                    },
                .max = 100,
                .suffix = "%",
            }),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_text(android_text("Adjusts the volume of all sounds in the game.",
                    "Règle le volume de tous les sons du jeu."));
            });
        leftPane.register_control(
            leftPane.add_child<NumberButton>(NumberButton::Props{
                .key = android_text("Main Music Volume", "Volume de la musique"),
                .getValue = [] { return getSettings().audio.mainMusicVolume.getValue(); },
                .setValue =
                    [](int value) {
                        getSettings().audio.mainMusicVolume.setValue(value);
                        config::save();
                    },
                .isModified =
                    [] {
                        return getSettings().audio.mainMusicVolume.getValue() !=
                               getSettings().audio.mainMusicVolume.getDefaultValue();
                    },
                .max = 100,
                .suffix = "%",
            }),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_text(android_text("Adjusts the volume of all music in the game.",
                    "Règle le volume de toutes les musiques du jeu."));
            });

        leftPane.add_section(android_text("Effects", "Effets"));
        config_bool_select(leftPane, rightPane, getSettings().audio.enableReverb,
            {
                .key = android_text("Enable Reverb", "Activer la réverbération"),
                .helpText = android_text("Enables the reverb effect in game audio.",
                    "Active la réverbération des sons du jeu."),
                .onChange = [](bool value) { audio::SetEnableReverb(value); },
            });
        config_bool_select(leftPane, rightPane, getSettings().audio.menuSounds,
            {
                .key = android_text("Dusklight Menu Sounds", "Sons des menus Dusklight"),
                .helpText = android_text("Play sound effects when navigating the Dusklight menu.",
                    "Joue des effets sonores pendant la navigation dans les menus Dusklight."),
            });

        leftPane.add_section(android_text("Tweaks", "Ajustements"));
        config_bool_select(leftPane, rightPane, getSettings().game.noLowHpSound,
            {
                .key = android_text("No Low HP Sound", "Désactiver l'alerte de vie faible"),
                .helpText = android_text("Disable the beeping sound when having low health.",
                    "Désactive le signal sonore lorsque votre vie est faible."),
            });
        config_bool_select(leftPane, rightPane, getSettings().game.midnasLamentNonStop,
            {
                .key = android_text(
                    "Non-Stop Midna's Lament", "Lamentation de Midona sans interruption"),
                .helpText = android_text("Prevents enemy music while Midna's Lament is playing.",
                    "Empêche la musique des ennemis d'interrompre la Lamentation de Midona."),
            });
    });

    add_tab(android_text("Gameplay", "Jouabilité"), [this](Rml::Element* content) {
        auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
        auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

        auto addOption = [&](const Rml::String& key, ConfigVar<bool>& value,
                             const Rml::String& helpText) {
            config_bool_select(leftPane, rightPane, value,
                {
                    .key = key,
                    .helpText = helpText,
                });
        };
        auto addSpeedrunDisabledOption = [&](const Rml::String& key, ConfigVar<bool>& value,
                                             const Rml::String& helpText) {
            add_speedrun_disabled_option(leftPane, rightPane, value, key, helpText);
        };

        leftPane.add_section(android_text("General", "Général"));
        addOption(android_text("Mirror Mode", "Mode miroir"), getSettings().game.enableMirrorMode,
            android_text("Mirrors the world horizontally, matching the Wii version of the game.",
                "Inverse le monde horizontalement, comme dans la version Wii du jeu."));
        addOption(android_text("Minimal HUD", "Affichage tête haute minimal"),
            getSettings().game.minimalHUD,
            android_text("Disables the elements of the main HUD of the game.<br/>Useful for a more "
                         "immersive "
                         "experience.",
                "Masque les éléments de l'affichage tête haute du jeu.<br/>Utile pour une "
                "expérience plus immersive."));
        config_percent_select(leftPane, rightPane, getSettings().game.hudScale,
            android_text("HUD Scale", "Taille de l'affichage tête haute"),
            android_text("Scales the size of the gameplay HUD (hearts, buttons, mini-map, etc.). "
                         "Does not affect dialog boxes or menus.",
                "Règle la taille de l'affichage tête haute (cœurs, boutons, mini-carte, etc.). "
                "N'affecte pas les dialogues ni les menus."),
            50, 200, 5, [] { return getSettings().game.minimalHUD.getValue(); });
        addOption(android_text("Restore Wii 1.0 Glitches", "Rétablir les bugs de la Wii 1.0"),
            getSettings().game.restoreWiiGlitches,
            android_text("Restores patched glitches from Wii USA 1.0, the first released version.",
                "Rétablit les bugs corrigés de la version Wii USA 1.0, première version "
                "commercialisée."));
        addOption(android_text("Enable Rotating Link Doll", "Faire pivoter Link dans l'inventaire"),
            getSettings().game.enableLinkDollRotation,
            android_text("Enables rotating Link in the collection menu with the C-Stick.",
                "Permet de faire pivoter Link avec le stick C dans le menu de collection."));
        addOption(android_text("Hide Owl Statue Markers", "Masquer les statues de hibou terminées"),
            getSettings().game.removeQuestMapMarkers,
            android_text("Removes completed Owl Statue markers from the map and Minimap.",
                "Masque les marqueurs des statues de hibou terminées sur la carte et la "
                "mini-carte."));

        leftPane.add_section(android_text("Difficulty", "Difficulté"));
        leftPane.register_control(
            leftPane.add_child<NumberButton>(NumberButton::Props{
                .key = android_text("Damage Multiplier", "Multiplicateur de dégâts"),
                .getValue = [] { return getSettings().game.damageMultiplier.getValue(); },
                .setValue =
                    [](int value) {
                        getSettings().game.damageMultiplier.setValue(value);
                        config::save();
                    },
                .isDisabled = [] { return speedrun::isActive(); },
                .isModified =
                    [] {
                        return getSettings().game.damageMultiplier.getValue() !=
                               getSettings().game.damageMultiplier.getDefaultValue();
                    },
                .min = 1,
                .max = 8,
                .suffix = "×",
            }),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_text(
                    android_text("Multiplies incoming damage.", "Multiplie les dégâts reçus."));
            });
        addSpeedrunDisabledOption(android_text("Instant Death", "Mort instantanée"),
            getSettings().game.instantDeath,
            android_text(
                "Any hit will instantly kill you.", "Le moindre dégât vous tue instantanément."));
        addSpeedrunDisabledOption(android_text("No Heart Drops", "Aucun cœur à ramasser"),
            getSettings().game.noHeartDrops,
            android_text("Hearts will never drop from enemies, pots, and various other places.",
                "Empêche l'apparition de cœurs sur les ennemis, dans les pots et ailleurs."));

        leftPane.add_section(android_text("Quality of Life", "Confort de jeu"));
        addOption(android_text("Bigger Wallets", "Bourses agrandies"),
            getSettings().game.biggerWallets,
            android_text("Wallet sizes are like in the HD version. (500, 1000, 2000)",
                "Utilise les capacités des bourses de la version HD (500, 1000, 2000)."));
        addOption(android_text("Disable Rupee Cutscenes", "Supprimer les animations des rubis"),
            getSettings().game.disableRupeeCutscenes,
            android_text(
                "Rupees will not play cutscenes after you have collected them the first time.",
                "Les rubis ne déclenchent plus de cinématique après leur première collecte."));
        addSpeedrunDisabledOption(
            android_text("Faster Scene Transitions", "Transitions accélérées"),
            getSettings().game.fastTransitions,
            android_text("Reduces how long the transitions take when changing maps.",
                "Réduit la durée des transitions entre les zones."));
        addOption(android_text("Faster Climbing", "Escalade accélérée"),
            getSettings().game.fastClimbing,
            android_text("Quicker climbing on ladders and vines like the HD version.",
                "Accélère l'escalade des échelles et des lianes, comme dans la version HD."));
        addOption(android_text("Faster Tears of Light", "Perles de lumière accélérées"),
            getSettings().game.fastTears,
            android_text(
                "Tears of Light dropped by Shadow Insects pop out faster like the HD version.",
                "Les perles de lumière des insectes des ombres apparaissent plus vite, comme dans "
                "la version HD."));
        addSpeedrunDisabledOption(android_text("Autosave", "Sauvegarde automatique"),
            getSettings().game.autoSave,
            android_text("Autosaves the game when going to a new area or opening a dungeon door.",
                "Sauvegarde automatiquement en changeant de zone ou en ouvrant une porte de "
                "donjon."));
        addOption(android_text("Instant Saves", "Sauvegardes instantanées"),
            getSettings().game.instantSaves,
            android_text("Skips the delay when writing to the Memory Card.",
                "Supprime le délai d'écriture sur la carte mémoire."));
        addOption(android_text("Hold B for Instant Text", "Maintenir B pour le texte instantané"),
            getSettings().game.instantText,
            android_text("Makes text scroll immediately by holding B.",
                "Affiche le texte immédiatement lorsque vous maintenez B."));
        addSpeedrunDisabledOption(
            android_text("Hold Button to Mash", "Répétition automatique des boutons"),
            getSettings().game.holdToMash,
            android_text("Hold the indicated button to mash automatically.",
                "Maintenez le bouton indiqué pour répéter automatiquement les pressions."));
        addOption(
            android_text("No Climbing Miss Animation", "Supprimer les hésitations en escalade"),
            getSettings().game.noMissClimbing,
            android_text("Prevents Link from playing a struggle animation when grabbing ledges or "
                         "climbing on vines.",
                "Supprime l'animation d'hésitation de Link lorsqu'il s'accroche à un rebord ou "
                "grimpe aux lianes."));
        addOption(android_text("No Rupee Returns", "Conserver tous les rubis"),
            getSettings().game.noReturnRupees,
            android_text("Always collect Rupees even if your Wallet is too full.",
                "Permet de ramasser les rubis même si votre bourse est pleine."));
        addOption(android_text("No Sword Recoil", "Aucun recul de l'épée"),
            getSettings().game.noSwordRecoil,
            android_text("Link will not recoil when his sword hits walls.",
                "Link ne recule plus lorsque son épée frappe un mur."));
        addOption(android_text("No 2nd Fish for Cat", "Un seul poisson pour le chat"),
            getSettings().game.no2ndFishForCat,
            android_text("Skip needing to catch a second fish for Sera's cat.",
                "Supprime la nécessité de pêcher un second poisson pour le chat de Sera."));
        addOption(android_text("Button Fishing", "Pêcher avec un bouton"),
            getSettings().game.buttonFishing,
            android_text(
                "Allow fishing with the Fishing Rod using the button the item is assigned to.",
                "Permet de pêcher avec le bouton auquel la canne à pêche est assignée."));
        addOption(android_text("Show Poe Count on Map", "Compteur d'âmes sur la carte"),
            getSettings().game.enhancedMapMenus,
            android_text("Displays collected/total number of Poe Souls for a region on the map.",
                "Affiche sur la carte le nombre d'âmes de spectre obtenues et le total par "
                "région."));
        addSpeedrunDisabledOption(android_text("Sun's Song (R+X)", "Chant du Soleil (R+X)"),
            getSettings().game.sunsSong,
            android_text("Allows Wolf Link to howl and change the time of day.",
                "Permet à Link loup de hurler pour changer l'heure de la journée."));
        addOption(android_text("Quick Transform (R+Y)", "Transformation rapide (R+Y)"),
            getSettings().game.enableQuickTransform,
            android_text("Transform instantly by pressing R and Y simultaneously.",
                "Transformez-vous instantanément en appuyant sur R et Y simultanément."));
        addOption(android_text("Aiming Reticle", "Réticule de visée"),
            getSettings().game.aimingReticle,
            android_text("Shows the aiming reticle for bow and slingshot.",
                "Affiche le réticule de visée de l'arc et du lance-pierres."));
        addOption(android_text("No Hit Stun", "Supprimer les pauses d'impact"),
            getSettings().game.noHitStun,
            android_text("Disables hit stun frames after attacks.",
                "Supprime les images de pause après les attaques."));

        leftPane.add_section(android_text("Speedrunning", "Speedrun"));
        config_bool_select(leftPane, rightPane, getSettings().game.speedrunMode,
            {
                .key = android_text("Speedrun Mode", "Mode speedrun"),
                .helpText =
                    android_text("Enables Speedrun game mode option in the Dusklight launch menu.",
                        "Ajoute le mode speedrun au menu de démarrage de Dusklight."),
                .onChange =
                    [this](bool enabled) {
                        if (enabled) {
                            speedrun::registerSpeedrunGameMode();
                        } else {
                            if (speedrun::isActive()) {
                                pop();
                            }
                            speedrun::unregisterSpeedrunGameMode();
                        }
                        MenuBar::refresh_tabs();
                    },
            });
        config_bool_select(leftPane, rightPane, getSettings().game.liveSplitEnabled,
            {
                .key = android_text("LiveSplit Connection", "Connexion à LiveSplit"),
                .helpText = android_text(
                    "Connect to LiveSplit server on localhost:16834. For this to work you must "
                    "right click LiveSplit, and turn on Control -> Start TCP Server."
                    " To see IGT in LiveSplit you must change your comparison to Game Time.",
                    "Se connecte au serveur LiveSplit sur localhost:16834. Dans LiveSplit, faites "
                    "un clic droit, puis activez Control -> Start TCP Server. Pour afficher le "
                    "temps de jeu, choisissez la comparaison Game Time."),
                .onChange =
                    [](bool enabled) {
                        if (enabled) {
                            speedrun::connectLiveSplit();
                        } else {
                            speedrun::disconnectLiveSplit();
                        }
                    },
                .isDisabled = [] { return IsMobile || !speedrun::isActive(); },
            });
        config_bool_select(leftPane, rightPane, getSettings().game.showSpeedrunRTATimer,
            {
                .key = android_text("Show RTA", "Afficher le temps réel (RTA)"),
                .helpText = android_text("Display the RTA timer. IGT is always visible.",
                    "Affiche le chronomètre en temps réel (RTA). Le temps de jeu (IGT) reste "
                    "toujours visible."),
                .isDisabled = [] { return !speedrun::isActive(); },
            });
    });

    add_tab(android_text("Cheats", "Triches"), [this](Rml::Element* content) {
        auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
        auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

        auto addCheat = [&](const Rml::String& key, ConfigVar<bool>& value,
                            const Rml::String& helpText) {
            add_speedrun_disabled_option(leftPane, rightPane, value, key, helpText);
        };

        leftPane.add_section(android_text("Resources", "Ressources"));
        addCheat(android_text("Infinite Hearts", "Cœurs infinis"),
            getSettings().game.infiniteHearts,
            android_text("Keeps your health full.", "Maintient votre vie au maximum."));
        addCheat(android_text("Infinite Arrows", "Flèches infinies"),
            getSettings().game.infiniteArrows,
            android_text(
                "Keeps your arrow count full.", "Maintient votre réserve de flèches au maximum."));
        addCheat(android_text("Infinite Seeds", "Graines infinies"),
            getSettings().game.infiniteSeeds,
            android_text("Keeps your slingshot pellets (seeds) full.",
                "Maintient votre réserve de graines de lance-pierres au maximum."));
        addCheat(android_text("Infinite Bombs", "Bombes infinies"),
            getSettings().game.infiniteBombs,
            android_text("Keeps all bomb bags full.", "Maintient tous vos sacs de bombes pleins."));
        addCheat(android_text("Infinite Oil", "Huile infinie"), getSettings().game.infiniteOil,
            android_text("Keeps your lantern oil full.",
                "Maintient votre réserve d'huile de lanterne au maximum."));
        addCheat(android_text("Infinite Oxygen", "Oxygène infini"),
            getSettings().game.infiniteOxygen,
            android_text("Keeps your underwater oxygen meter full.",
                "Maintient votre jauge d'oxygène pleine sous l'eau."));
        addCheat(android_text("Infinite Rupees", "Rubis infinis"),
            getSettings().game.infiniteRupees,
            android_text(
                "Keeps your rupee count full.", "Maintient votre réserve de rubis au maximum."));
        addCheat(android_text("No Item Timer", "Objets au sol permanents"),
            getSettings().game.enableIndefiniteItemDrops,
            android_text(
                "Item drops such as rupees and hearts will never disappear after they drop.",
                "Les objets au sol, comme les rubis et les cœurs, ne disparaissent plus."));

        leftPane.add_section(android_text("Abilities", "Capacités"));

        addCheat(android_text("Moon Jump (R+A)", "Saut lunaire (R+A)"), getSettings().game.moonJump,
            android_text("Hold R and A to rise into the air.",
                "Maintenez R et A pour vous élever dans les airs."));
        addCheat(android_text("Easy Quick Spin (R+B)", "Attaque circulaire facile (R+B)"),
            getSettings().game.easyQuickSpin,
            android_text("Hold R to always do a Quick Spin when attacking with B.",
                "Maintenez R pour toujours effectuer une attaque circulaire en attaquant avec B."));

        addCheat(android_text("Super Clawshot", "Super grappin"), getSettings().game.superClawshot,
            android_text("Extends Clawshot behavior beyond the normal game rules.",
                "Étend les possibilités du grappin au-delà des limites habituelles du jeu."));
        leftPane.register_control(
            leftPane.add_select_button({
                .key = android_text("Always Greatspin", "Super attaque circulaire"),
                .getValue =
                    [] {
                        return kAlwaysGreatspinModes[static_cast<u8>(
                            getSettings().game.alwaysGreatspin.getValue())];
                    },
                .isDisabled = [] { return dusk::speedrun::isActive(); },
                .isModified =
                    [] {
                        return getSettings().game.alwaysGreatspin.getValue() !=
                               getSettings().game.alwaysGreatspin.getDefaultValue();
                    },
            }),
            rightPane, [](Pane& pane) {
                for (int i = 0; i < static_cast<int>(kAlwaysGreatspinModes.size()); i++) {
                    pane.add_button(
                            {
                                .text = kAlwaysGreatspinModes[i],
                                .isSelected =
                                    [i] {
                                        return getSettings().game.alwaysGreatspin.getValue() ==
                                               static_cast<AlwaysGreatspinMode>(i);
                                    },
                            })
                        .on_pressed([i] {
                            mDoAud_seStartMenu(kSoundItemChange);
                            getSettings().game.alwaysGreatspin.setValue(
                                static_cast<AlwaysGreatspinMode>(i));
                            config::save();
                        });
                }
                pane.add_rml(
                    android_text("<br/>Allows the Great Spin attack without requiring full health.",
                        "<br/>Permet la super attaque circulaire même sans avoir toute sa vie."));
            });
        addCheat(android_text("Fast Iron Boots", "Bottes de fer accélérées"),
            getSettings().game.enableFastIronBoots,
            android_text("Speeds up movement while heavy, including wearing the Iron Boots, "
                         "holding the Ball and Chain, wearing Magic Armor without rupees, etc.",
                "Accélère les déplacements sous une charge lourde : bottes de fer, boulet, armure "
                "magique sans rubis, etc."));
        addCheat(android_text("Can Transform Anywhere", "Transformation partout"),
            getSettings().game.canTransformAnywhere,
            android_text("Allows transforming even if NPCs are looking.",
                "Permet de se transformer même sous le regard des personnages."));
        addCheat(android_text("Fast Roll", "Roulade accélérée"), getSettings().game.fastRoll,
            android_text("Makes Link's roll animation and movement twice as fast.",
                "Double la vitesse de l'animation et du déplacement des roulades de Link."));
        addCheat(android_text("Fast Spinner", "Aérouage accéléré"), getSettings().game.fastSpinner,
            android_text("Speeds up Spinner movement while holding R.",
                "Accélère l'aérouage lorsque vous maintenez R."));
        leftPane.register_control(
            leftPane.add_select_button({
                .key = android_text("Magic Armor Behavior", "Comportement de l'armure magique"),
                .getValue =
                    [] {
                        return kMagicArmorModes[static_cast<u8>(
                            getSettings().game.armorRupeeDrain.getValue())];
                    },
                .isDisabled = [] { return speedrun::isActive(); },
                .isModified =
                    [] {
                        return getSettings().game.armorRupeeDrain.getValue() !=
                               getSettings().game.armorRupeeDrain.getDefaultValue();
                    },
            }),
            rightPane, [](Pane& pane) {
                for (int i = 0; i < kMagicArmorModes.size(); i++) {
                    pane.add_button(
                            {
                                .text = kMagicArmorModes[i],
                                .isSelected =
                                    [i] {
                                        return getSettings().game.armorRupeeDrain.getValue() ==
                                               static_cast<MagicArmorMode>(i);
                                    },
                            })
                        .on_pressed([i] {
                            mDoAud_seStartMenu(kSoundItemChange);
                            getSettings().game.armorRupeeDrain.setValue(
                                static_cast<MagicArmorMode>(i));
                            config::save();
                        });
                }
                pane.add_rml(android_text("<br/>Control the behavior of the Magic Armor.",
                    "<br/>Définit le comportement de l'armure magique."));
            });
        addCheat(android_text("Invincible Enemies", "Ennemis invulnérables"),
            getSettings().game.invincibleEnemies,
            android_text("Prevents enemies from taking damage.",
                "Empêche les ennemis de subir des dégâts."));
        addCheat(android_text("Infinite Epona Stamina", "Endurance d'Épona infinie"),
            getSettings().game.infiniteEponaStamina,
            android_text("Prevents Epona's stamina from depleting.",
                "Empêche l'endurance d'Épona de diminuer."));
    });

    add_tab("Interface", [this](Rml::Element* content) {
        auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
        auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

        leftPane.add_section("Dusklight");
#if DUSK_CAN_OPEN_DATA_FOLDER
        leftPane.register_control(
            leftPane.add_button(android_text("Open Data Folder", "Ouvrir le dossier de données"))
                .on_pressed([] {
                    mDoAud_seStartMenu(kSoundClick);
                    data::open_data_path();
                }),
            rightPane, [](Pane& pane) {
                pane.add_text(android_text(
                    "Open the folder where Dusklight stores settings, saves, logs, texture "
                    "replacements, and other app data.",
                    "Ouvre le dossier contenant les paramètres, sauvegardes, journaux, packs de "
                    "textures et autres données de Dusklight."));
            });
#endif
        leftPane.register_control(
            leftPane.add_button(android_text("Restart to Main Menu", "Revenir au menu principal"))
                .on_pressed([this] {
                    mDoAud_seStartMenu(kSoundClick);
                    pop();
                    prelaunch_state().returnToPrelaunchOnReset = true;
                    JUTGamePad::C3ButtonReset::sResetSwitchPushing = true;
                }),
            rightPane, [](Pane& pane) {
                pane.add_text(android_text(
                    "Restart Dusklight to the pre-launch menu to change settings, game "
                    "modes, or mods.",
                    "Redémarre Dusklight au menu principal pour modifier les paramètres, modes de "
                    "jeu ou mods."));
            });
        leftPane.register_control(
            leftPane.add_select_button({
                .key = "Notifications",
                .getValue =
                    [] {
                        const bool ach = getSettings().game.enableAchievementToasts.getValue();
                        const bool ctl = getSettings().game.enableControllerToasts.getValue();
                        if (!ach && !ctl) {
                            return Rml::String{android_text("Off", "Désactivé")};
                        }
                        if (ach && ctl) {
                            return Rml::String{android_text("All", "Toutes")};
                        }
                        return Rml::String{android_text("Some", "Certaines")};
                    },
                .isModified =
                    [] {
                        const auto& ach = getSettings().game.enableAchievementToasts;
                        const auto& ctl = getSettings().game.enableControllerToasts;
                        return ach.getValue() != ach.getDefaultValue() ||
                               ctl.getValue() != ctl.getDefaultValue();
                    },
            }),
            rightPane, [](Pane& pane) {
                pane.clear();
                pane.add_button(android_text("Select All", "Tout sélectionner")).on_pressed([] {
                    mDoAud_seStartMenu(kSoundItemChange);
                    getSettings().game.enableAchievementToasts.setValue(true);
                    getSettings().game.enableControllerToasts.setValue(true);
                    config::save();
                });
                pane.add_button(android_text("Select None", "Tout désélectionner")).on_pressed([] {
                    mDoAud_seStartMenu(kSoundItemChange);
                    getSettings().game.enableAchievementToasts.setValue(false);
                    getSettings().game.enableControllerToasts.setValue(false);
                    config::save();
                });

                pane.add_section("Types");
                pane
                    .add_button({
                        .text = android_text("Achievements", "Succès"),
                        .isSelected =
                            [] { return getSettings().game.enableAchievementToasts.getValue(); },
                    })
                    .on_pressed([] {
                        mDoAud_seStartMenu(kSoundItemChange);
                        auto& v = getSettings().game.enableAchievementToasts;
                        v.setValue(!v.getValue());
                        config::save();
                    });
                pane.add_button(
                        {
                            .text = android_text("Missing Device", "Périphérique manquant"),
                            .isSelected =
                                [] { return getSettings().game.enableControllerToasts.getValue(); },
                        })
                    .on_pressed([] {
                        mDoAud_seStartMenu(kSoundItemChange);
                        auto& v = getSettings().game.enableControllerToasts;
                        v.setValue(!v.getValue());
                        config::save();
                    });
                pane.add_rml(android_text("<br/>Choose which notifications can be displayed.",
                    "<br/>Choisissez les notifications à afficher."));
            });
#if BOREALIS_HAS_SENTRY
        auto& crashReporting = leftPane.add_child<BoolButton>(BoolButton::Props{
            .key = android_text("Crash Reporting", "Rapports de plantage"),
            .getValue =
                [] { return borealis::sentry::get_consent() == borealis::sentry::Consent::Given; },
            .setValue = [](bool enabled) { borealis::sentry::set_consent(enabled); },
            .isDisabled =
                [] {
                    return borealis::sentry::get_consent() ==
                           borealis::sentry::Consent::Unavailable;
                },
            .isModified = [] { return false; },
        });
        leftPane.register_control(crashReporting, rightPane, [](Pane& pane) {
            pane.clear();
            pane.add_rml(android_text(
                "Dusklight can automatically send crash reports to the developers. Crash "
                "reports contain the following:<br/>• Operating system version<br/>• CPU "
                "architecture<br/>• GPU model & driver version<br/>• File paths (may "
                "include account username)<br/>• Stack trace",
                "Dusklight peut envoyer automatiquement des rapports de plantage aux développeurs. "
                "Ces rapports contiennent :<br/>• La version du système d'exploitation<br/>"
                "• L'architecture du processeur<br/>• Le modèle du processeur graphique et la "
                "version "
                "du pilote<br/>• Des chemins de fichiers (pouvant inclure le nom du compte)<br/>"
                "• La trace d'appels"));
        });
#endif
        config_bool_select(leftPane, rightPane, getSettings().backend.skipPreLaunchUI,
            {
                .key = android_text(
                    "Skip Dusklight Main Menu", "Ignorer le menu principal de Dusklight"),
                .helpText = android_text(
                    "When starting Dusklight, skip the main menu and boot straight into the "
                    "game if a disc image is available.<br/><br/>Note: If any mods register game "
                    "modes, this option will be ignored.",
                    "Au démarrage, lance directement le jeu si une image du disque est "
                    "disponible.<br/><br/>Cette option est ignorée si des mods ajoutent des modes "
                    "de jeu."),
            });
        config_bool_select(leftPane, rightPane, getSettings().backend.showPipelineCompilation,
            {
                .key =
                    android_text("Show Shader Compilation", "Afficher la compilation des shaders"),
                .helpText = android_text(
                    "Show an overlay when shaders are being compiled for your hardware.",
                    "Affiche un indicateur pendant la compilation des shaders pour votre "
                    "matériel."),
            });
        config_bool_select(leftPane, rightPane, getSettings().backend.checkForUpdates,
            {
                .key = android_text(
                    "Check for Dusklight Updates", "Rechercher les mises à jour de Dusklight"),
                .helpText = android_text(
                    "Checks GitHub releases for a new Dusklight version on startup.<br/><br/>"
                    "No personal information is transmitted or collected.",
                    "Recherche une nouvelle version dans les releases GitHub au "
                    "démarrage.<br/><br/>Aucune donnée personnelle n'est transmise ni collectée."),
            });
#if BOREALIS_HAS_DISCORD
        config_bool_select(leftPane, rightPane, getSettings().game.enableDiscordPresence,
            {
                .key = android_text(
                    "Enable Discord Rich Presence", "Activer la présence enrichie Discord"),
                .helpText =
                    android_text("Enable Dusklight to integrate with Discord Rich Presence. This "
                                 "allows Discord to show your status in-game.",
                        "Permet à Discord d'afficher votre activité dans Dusklight."),
                .onChange =
                    [](bool enabled) {
                        if (enabled) {
                            discord::initialize();
                        } else {
                            discord::shutdown();
                        }
                    },
            });
#endif
        config_bool_select(leftPane, rightPane, getSettings().backend.enableAdvancedSettings,
            {
                .key = android_text("Enable Advanced Settings", "Activer les paramètres avancés"),
                .icon = "warning",
                .helpText = android_text(
                    "Show advanced settings and debugging tools with "
                    "Shift+F1.<br/><br/><icon class=\"warning\"/> WARNING: Debugging tools "
                    "can easily break your game. Do not use on a regular save!",
                    "Affiche les paramètres avancés et les outils de débogage avec "
                    "Maj+F1.<br/><br/><icon class=\"warning\"/> ATTENTION : ces outils peuvent "
                    "endommager votre partie. Ne les utilisez pas sur une sauvegarde habituelle !"),
                .onChange = [](bool) { MenuBar::refresh_tabs(); },
                .isDisabled = [] { return speedrun::isActive(); },
            });
        config_bool_select(leftPane, rightPane, getSettings().game.showInputViewer,
            {
                .key = android_text("Show Input Viewer", "Afficher les commandes à l'écran"),
                .helpText = android_text("Display a controller input overlay while playing.",
                    "Affiche les commandes de la manette à l'écran pendant le jeu."),
            });
        config_bool_select(leftPane, rightPane, getSettings().game.showInputViewerGyro,
            {
                .key = android_text("Show Gyro Input Viewer", "Afficher les valeurs du gyroscope"),
                .helpText = android_text("Show gyro sensor values in the input viewer.",
                    "Affiche les valeurs du gyroscope dans la visualisation des commandes."),
                .isDisabled = [] { return !getSettings().game.showInputViewer; },
            });
        leftPane.add_section(android_text("Game", "Jeu"));
        leftPane.register_control(
            leftPane.add_select_button({
                .key = android_text("Menu Scaling Mode", "Adaptation de la taille des menus"),
                .getValue =
                    [] {
                        return kMenuScalingModeLabels[static_cast<u8>(
                            getSettings().game.menuScalingMode.getValue())];
                    },
                .isModified =
                    [] {
                        const auto& mode = getSettings().game.menuScalingMode;
                        return mode.getValue() != mode.getDefaultValue();
                    },
            }),
            rightPane, [](Pane& pane) {
                for (int i = 0; i < static_cast<int>(kMenuScalingModeLabels.size()); ++i) {
                    pane.add_button(
                            {
                                .text = kMenuScalingModeLabels[i],
                                .isSelected =
                                    [i] {
                                        return getSettings().game.menuScalingMode.getValue() ==
                                               static_cast<MenuScaling>(i);
                                    },
                            })
                        .on_pressed([i] {
                            mDoAud_seStartMenu(kSoundItemChange);
                            getSettings().game.menuScalingMode.setValue(
                                static_cast<MenuScaling>(i));
                            config::save();
                        });
                }
                pane.add_rml(android_text(
                    "<br/>Changes how the Collection and File Select menus scale to your "
                    "aspect ratio.",
                    "<br/>Définit l'adaptation des menus de collection et de sélection des "
                    "sauvegardes au format de votre écran."));
            });
        config_bool_select(leftPane, rightPane, getSettings().game.hideTvSettingsScreen,
            {
                .key = android_text("Skip TV Settings Screen", "Ignorer le réglage du téléviseur"),
                .helpText =
                    android_text("Skips the TV calibration screen shown when loading a save.",
                        "Ignore l'écran de réglage du téléviseur au chargement d'une sauvegarde."),
            });
        add_speedrun_disabled_option(leftPane, rightPane, getSettings().game.recordingMode,
            android_text("Recording Mode", "Mode enregistrement"),
            android_text("Disables the game HUD and all background music.<br/><br/>Useful for "
                         "recording footage.",
                "Masque l'affichage tête haute et coupe la musique.<br/><br/>Utile pour "
                "enregistrer des vidéos."));
    });

    add_tab(android_text("Tools", "Outils"), [this](Rml::Element* content) {
        auto& leftPane = add_child<Pane>(content, Pane::Type::Controlled);
        auto& rightPane = add_child<Pane>(content, Pane::Type::Uncontrolled);

        leftPane.add_section("Link");
        add_speedrun_disabled_option(leftPane, rightPane, getSettings().game.enableMoveLinkCombo,
            android_text("Move Link (L+R+Y)", "Déplacer Link (L+R+Y)"),
            android_text("Enables the L+R+Y button combo to toggle freely repositioning Link.",
                "Active le raccourci L+R+Y pour déplacer Link librement."));
        add_speedrun_disabled_option(leftPane, rightPane, getSettings().game.enableTeleportCombo,
            android_text("Teleport (R+D-pad Up/Down)", "Téléportation (R+croix haut/bas)"),
            android_text("R+D-pad Up stores Link's current position.<br/>"
                         "R+D-pad Down teleports Link back to it.",
                "R+croix haut mémorise la position de Link.<br/>R+croix bas le téléporte à cette "
                "position."));
    });
}

void SettingsWindow::update() {
    if (mPrelaunch && top_document() == this) {
        try_push_verification_modal(*this);
        try_push_language_unavailable_modal(*this);
    }

    Window::update();
}

void SettingsWindow::hide(bool close) {
    config::save();
    Window::hide(close);
}

}  // namespace dusk::ui
