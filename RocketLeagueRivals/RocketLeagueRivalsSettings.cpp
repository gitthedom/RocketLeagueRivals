#include "pch.h"
#include "RocketLeagueRivals.h"

void RocketLeagueRivals::RenderSettings() {
    ImGui::TextUnformatted("Rocket League Rivals Settings");

    // Helper function to render checkboxes
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

    renderCheckbox("Align display to the right", "align_display_right", "Align display to the right (Defaults to left)");
    renderCheckbox("Hides your teams stats", "hide_my_team_enabled", "Hides stats from your own team. You would only see Rival team if it is enabled.");
    renderCheckbox("Hides rival teams stats", "hide_rival_team_enabled", "Hides stats from Rival team. You would only see your team if it is enabled.");
    renderCheckbox("Show both wins/loses with/against", "show_all_winsloses_stats_enabled", "Shows both wins and loses with and against no matter what team they are on.");
    renderCheckbox("Show demo stats", "show_demo_stats_enabled", "Shows stats on demos received and given to and from another player.");
    renderCheckbox("Enables logging in data/rivals folder", "folder_logging_enabled", "Enables logging in data/rivals folder.");
}

