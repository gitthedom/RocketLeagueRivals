#include "pch.h"
#include "RocketLeagueRivals.h"

bool CharFilterCallback(ImGuiInputTextCallbackData* data) {
    if (data->EventChar < 256 && !isdigit(data->EventChar)) {
        return true;
    }

    // Limit to two characters
    if (data->BufTextLen >= 2 && data->EventChar != 0) {
        return true;
    }

    return false;
}

void RocketLeagueRivals::RenderSettings() {
    ImGui::TextUnformatted("Rocket League Rivals Settings");

    auto renderCheckbox = [&](const char* label, const char* cvarName, const char* tooltip) {
        CVarWrapper cvar = cvarManager->getCvar(cvarName);
        if (!cvar) { return; }
        bool value = cvar.getBoolValue();
        if (ImGui::Checkbox(label, &value)) {
            cvar.setValue(value);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(tooltip);
        }
    };

    auto renderInputStringBox = [&](const char* label, const char* cvarName, const char* tooltip, const int textBoxSize) {
        CVarWrapper cvar = cvarManager->getCvar(cvarName);
        if (!cvar) { return; }
        std::string value = cvar.getStringValue();
        char inputBuffer[3]; // Buffer for up to 2 characters + null terminator
        strncpy(inputBuffer, value.c_str(), sizeof(inputBuffer));
        inputBuffer[sizeof(inputBuffer) - 1] = '\0'; // Ensure null-termination

        ImGui::SetNextItemWidth(textBoxSize);
        if (ImGui::InputText(label, inputBuffer, sizeof(inputBuffer), ImGuiInputTextFlags_CallbackCharFilter, [](ImGuiInputTextCallbackData* data) -> int {
            return CharFilterCallback(data) ? 1 : 0;
            })) {
            cvar.setValue(std::string(inputBuffer));
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(tooltip);
        }
    };

    renderCheckbox("Align display to the right", "align_display_right", "Align display to the right (Defaults to left)");
    renderCheckbox("Shows percentage instead of ratio", "show_percentage_enabled", "Shows your win percentage instead of your win:loss ratio");
    renderCheckbox("Hides your teams stats", "hide_my_team_enabled", "Hides stats from your own team. You would only see Rival team if it is enabled.");
    renderCheckbox("Hides rival teams stats", "hide_rival_team_enabled", "Hides stats from Rival team. You would only see your team if it is enabled.");
    renderCheckbox("Show team stats", "show_team_stats_enabled", "Shows only the current team stats. Only win percentage of games where they were on the corresponding team.");
    renderCheckbox("Show both wins/loses with/against", "show_all_winsloses_stats_enabled", "Shows both wins and loses with and against no matter what team they are on.");
    renderCheckbox("Show demo stats", "show_demo_stats_enabled", "Shows stats on demos received and given to and from another player.");
    renderCheckbox("Show rival stats", "show_rival_stats_enabled", "Shows rival stats on another player another player.");
    renderInputStringBox("Set Rival Threshold Score 1-99", "set_rival_number", "Sets the points value that would quantify someone as a rival.", 25);
    CVarWrapper rivalNumberInput = cvarManager->getCvar("set_rival_number");
    std::string rivalNumberString = rivalNumberInput.getStringValue();
    std::stringstream geek;
    geek << rivalNumberString;
    int rivalNumberCheck;
    geek >> rivalNumberCheck;
    if (!(rivalNumberCheck > 0)) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "This setting must be a number above zero. Defaulting to 5.");
    }

    ImGui::TextUnformatted("");
    renderCheckbox("Enables logging in data/rivals folder", "folder_logging_enabled", "Enables logging in data/rivals folder.");
}