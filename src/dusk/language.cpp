#include "dusk/language.hpp"

#include "dusk/ui/localization.hpp"

#include "dusk/version.hpp"

namespace dusk::language {
namespace {

constexpr GameLanguage kEnglishOnly[] = {GameLanguage::English};
constexpr GameLanguage kJapaneseOnly[] = {GameLanguage::Japanese};
constexpr GameLanguage kPalLanguages[] = {
    GameLanguage::English,
    GameLanguage::German,
    GameLanguage::French,
    GameLanguage::Spanish,
    GameLanguage::Italian,
};
constexpr GameLanguage kWiiUsaLanguages[] = {
    GameLanguage::English,
    GameLanguage::French,
    GameLanguage::Spanish,
};

}  // namespace

std::span<const GameLanguage> available_languages(const iso::DiscInfo& info) noexcept {
    switch (info.region) {
    case iso::Region::Japan:
        return kJapaneseOnly;
    case iso::Region::Europe:
        return kPalLanguages;
    case iso::Region::NorthAmerica:
        if (info.platform == iso::Platform::Wii && info.revision == 2) {
            return kWiiUsaLanguages;
        }
        return kEnglishOnly;
    default:
        return kEnglishOnly;
    }
}

const char* language_name(GameLanguage language) noexcept {
    switch (language) {
    case GameLanguage::English:
        return ui::android_text("English", "Anglais");
    case GameLanguage::German:
        return ui::android_text("German", "Allemand");
    case GameLanguage::French:
        return ui::android_text("French", "Français");
    case GameLanguage::Spanish:
        return ui::android_text("Spanish", "Espagnol");
    case GameLanguage::Italian:
        return ui::android_text("Italian", "Italien");
    case GameLanguage::Japanese:
        return ui::android_text("Japanese", "Japonais");
    }
    return ui::android_text("English", "Anglais");
}

const char* msg_folder() noexcept {
    using namespace version;

    switch (getSettings().game.language.getValue()) {
    case GameLanguage::German:
        return "Msgde";
    case GameLanguage::French:
        return "Msgfr";
    case GameLanguage::Spanish:
        return "Msgsp";
    case GameLanguage::Italian:
        return "Msgit";
    case GameLanguage::Japanese:
        return "Msgjp";
    case GameLanguage::English:
    default:
        return isRegionPal() ? "Msguk" : "Msgus";
    }
}

}  // namespace dusk::language
