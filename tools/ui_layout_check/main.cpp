#include <RmlUi/Core.h>
#include <RmlUi/Core/ElementText.h>
#include <RmlUi/Core/ElementUtilities.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct NullRenderer : Rml::RenderInterface {
    uintptr_t handle = 0;
    Rml::CompiledGeometryHandle CompileGeometry(
        Rml::Span<const Rml::Vertex>, Rml::Span<const int>) override {
        return ++handle;
    }
    void RenderGeometry(Rml::CompiledGeometryHandle, Rml::Vector2f, Rml::TextureHandle) override {}
    void ReleaseGeometry(Rml::CompiledGeometryHandle) override {}
    Rml::TextureHandle LoadTexture(Rml::Vector2i& size, const Rml::String&) override {
        size = {1024, 256};
        return ++handle;
    }
    Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte>, Rml::Vector2i) override {
        return ++handle;
    }
    void ReleaseTexture(Rml::TextureHandle) override {}
    void EnableScissorRegion(bool) override {}
    void SetScissorRegion(Rml::Rectanglei) override {}
};

struct System : Rml::SystemInterface {
    bool badStyles = false;
    bool LogMessage(Rml::Log::Type type, const Rml::String& text) override {
        if (text.find("android.rcss") != std::string::npos) {
            std::cerr << text << '\n';
            badStyles = true;
        }
        if (type == Rml::Log::LT_ERROR)
            std::cerr << text << '\n';
        return true;
    }
};

std::string normalize(std::string text) {
    text.erase(std::remove_if(text.begin(), text.end(),
                   [](unsigned char c) { return c < 128 && std::isspace(c); }),
        text.end());
    for (char& c : text)
        if (c >= 'A' && c <= 'Z')
            c += 'a' - 'A';
    return text;
}

int main(int argc, char** argv) {
    if (argc < 3)
        return 2;
    std::filesystem::current_path(argv[1]);
    bool before = argc > 3 && std::string(argv[3]) == "--before";
    NullRenderer renderer;
    System system;
    Rml::SetRenderInterface(&renderer);
    Rml::SetSystemInterface(&system);
    if (!Rml::Initialise())
        return 2;
    for (const char* font :
        {"FiraSans-Regular.ttf", "FiraSans-Bold.ttf", "FiraSansCondensed-Regular.ttf",
            "FiraSansCondensed-Bold.ttf", "MaterialSymbolsRounded-Regular.ttf"})
    {
        if (!Rml::LoadFontFace(std::string("res/") + font, true))
            return 2;
    }
    std::ifstream input(argv[2]);
    std::vector<std::pair<std::string, std::string>> rows;
    std::vector<std::string> tabs;
    std::string line;
    while (std::getline(input, line)) {
        auto split = line.find('\t');
        if (split != std::string::npos) {
            if (line.substr(0, split) == "@tab")
                tabs.push_back(line.substr(split + 1));
            else
                rows.emplace_back(line.substr(0, split), line.substr(split + 1));
        }
    }
    auto* context = Rml::CreateContext("layout", {1920, 1080});
    int failures = 0;
    int checks = 0;
    for (float scale : {1.5f, 369.f / 160.f, 369.f / 160.f * 1.25f, 369.f / 160.f * 1.5f}) {
        context->SetDensityIndependentPixelRatio(scale);
        std::string source = R"(<rml><head>
<link type="text/rcss" href="res/rml/theme.rcss"/>
<link type="text/rcss" href="res/rml/controls.rcss"/>
<link type="text/rcss" href="res/rml/tabbing.rcss"/>
<link type="text/rcss" href="res/rml/window.rcss"/>
</head><body><window open><tab-bar id="tabs" closable><close/>)";
        for (size_t i = 0; i < tabs.size(); ++i) {
            source += "<tab id=\"tab" + std::to_string(i) + "\">" + tabs[i] + "</tab>";
        }
        source += "<tab-end-spacer/></tab-bar><content><pane id=\"left\">";
        for (size_t i = 0; i < rows.size(); ++i) {
            bool editing = rows[i].second.starts_with("@input:");
            source += "<select-button id=\"row" + std::to_string(i) + "\"" +
                      (editing ? " class=\"editing\"" : "") + "><key>" + rows[i].first +
                      "</key><icon/>";
            if (editing)
                source += "<value style=\"visibility: hidden\">100%</value><input type=\"number\" "
                          "value=\"100\"/>";
            else
                source += "<value>" + rows[i].second + "</value>";
            source += "</select-button>";
        }
        source += "</pane><pane><div>Texte d'aide et description du "
                  "réglage.</div></pane></content></window></body></rml>";
        auto* doc = context->LoadDocumentFromMemory(source);
        auto styles = Rml::Factory::InstanceStyleSheetFile("res/rml/global.rcss");
        styles = styles->CombineStyleSheetContainer(*doc->GetStyleSheetContainer());
        if (!before)
            styles = styles->CombineStyleSheetContainer(
                *Rml::Factory::InstanceStyleSheetFile("res/rml/android.rcss"));
        doc->SetStyleSheetContainer(std::move(styles));
        doc->Show();
        context->Update();
        auto* bar = doc->GetElementById("tabs");
        const auto barPosition = bar->GetAbsoluteOffset(Rml::BoxArea::Content);
        const auto barSize = bar->GetBox().GetSize(Rml::BoxArea::Content);
        for (size_t i = 0; i < tabs.size(); ++i) {
            auto* tab = doc->GetElementById("tab" + std::to_string(i));
            const auto position = tab->GetAbsoluteOffset(Rml::BoxArea::Border);
            const auto size = tab->GetBox().GetSize(Rml::BoxArea::Border);
            if (position.x < barPosition.x - 2 ||
                position.x + size.x > barPosition.x + barSize.x + 2 ||
                position.y < barPosition.y - 2 ||
                position.y + size.y > barPosition.y + barSize.y + 2)
            {
                std::cout << "FAIL tab scale=" << scale << " label=" << tabs[i] << '\n';
                ++failures;
            }
            ++checks;
        }
        for (size_t i = 0; i < rows.size(); ++i) {
            auto* row = doc->GetElementById("row" + std::to_string(i));
            row->ScrollIntoView();
            context->Update();
            context->Render();
            for (const char* tag : {"key", "value"}) {
                if (rows[i].second.starts_with("@input:") && std::string(tag) == "value") {
                    auto* input = row->QuerySelector("input");
                    const auto position = input->GetAbsoluteOffset(Rml::BoxArea::Border);
                    const auto size = input->GetBox().GetSize(Rml::BoxArea::Border);
                    const auto rowPosition = row->GetAbsoluteOffset(Rml::BoxArea::Border);
                    const auto rowSize = row->GetBox().GetSize(Rml::BoxArea::Border);
                    if (size.x < Rml::ElementUtilities::GetStringWidth(input, "100") ||
                        position.x + size.x > rowPosition.x + rowSize.x + 2 ||
                        position.y + size.y > rowPosition.y + rowSize.y + 2)
                    {
                        std::cout << "FAIL numeric input scale=" << scale << '\n';
                        ++failures;
                    }
                    ++checks;
                    continue;
                }
                auto* label = row->QuerySelector(tag);
                auto* text = dynamic_cast<Rml::ElementText*>(label->GetFirstChild());
                if (!text) {
                    ++failures;
                    continue;
                }
                std::string actual;
                float maxWidth = 0;
                for (const auto& renderedLine : text->GetLines()) {
                    actual += renderedLine.text;
                    // Geometry widths may already include ellipsis truncation. Measure the
                    // complete line so clipped text cannot make this check pass.
                    maxWidth = std::max(maxWidth,
                        static_cast<float>(
                            Rml::ElementUtilities::GetStringWidth(text, renderedLine.text)));
                }
                float width = label->GetClientWidth();
                bool fits = maxWidth <= width + 2 &&
                            normalize(actual) == normalize(text->GetText()) && maxWidth > 0;
                if (!fits) {
                    std::cout << "FAIL scale=" << scale << " " << tag << " width=" << width
                              << " text=" << maxWidth << " label=" << text->GetText()
                              << " rendered=" << actual << '\n';
                    ++failures;
                }
                ++checks;
            }
        }
        doc->Close();
        context->Update();
    }
    Rml::RemoveContext("layout");
    Rml::Shutdown();
    std::cout << checks << " label checks, " << failures << " failures\n";
    return failures || system.badStyles ? 1 : 0;
}
