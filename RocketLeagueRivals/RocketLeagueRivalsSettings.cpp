#include "pch.h"
#include "RocketLeagueRivals.h"

void RocketLeagueRivals::RenderSettings() {
    ImGui::TextUnformatted("Rocket League Rivals Settings");

    CVarWrapper enableRightAlignCvar = cvarManager->getCvar("align_display_right");
    if (!enableRightAlignCvar) { return; }
    bool enabledRightAlign = enableRightAlignCvar.getBoolValue();
    if (ImGui::Checkbox("Align display to the right", &enabledRightAlign)) {
        enableRightAlignCvar.setValue(enabledRightAlign);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Align display to the right (Defaults to left)");
    }

    CVarWrapper enableHideMyTeamCvar = cvarManager->getCvar("hide_my_team_enabled");
    if (!enableHideMyTeamCvar) { return; }
    bool enabledHideMyTeam = enableHideMyTeamCvar.getBoolValue();
    if (ImGui::Checkbox("Hides your teams stats", &enabledHideMyTeam)) {
        enableHideMyTeamCvar.setValue(enabledHideMyTeam);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Hides stats from your own team. You would only see Rival team if it is enabled.");
    }

    CVarWrapper enableHideRivalTeamCvar = cvarManager->getCvar("hide_rival_team_enabled");
    if (!enableHideRivalTeamCvar) { return; }
    bool enabledHideRivalTeam = enableHideRivalTeamCvar.getBoolValue();
    if (ImGui::Checkbox("Hides rival teams stats", &enabledHideRivalTeam)) {
        enableHideRivalTeamCvar.setValue(enabledHideRivalTeam);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Hides stats from Rival team. You would only see your team if it is enabled.");
    }

    CVarWrapper enableShowAllStatsCvar = cvarManager->getCvar("show_all_winsloses_stats_enabled");
    if (!enableShowAllStatsCvar) { return; }
    bool enabledAllStats = enableShowAllStatsCvar.getBoolValue();
    if (ImGui::Checkbox("Show both wins/loses with/against", &enabledAllStats)) {
        enableShowAllStatsCvar.setValue(enabledAllStats);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Shows both wins and loses with and against no matter what team they are on.");
    }

    CVarWrapper enableShowDemoStatsCvar = cvarManager->getCvar("show_demo_stats_enabled");
    if (!enableShowDemoStatsCvar) { return; }
    bool enabledDemoStats = enableShowDemoStatsCvar.getBoolValue();
    if (ImGui::Checkbox("Show demo stats", &enabledDemoStats)) {
        enableShowDemoStatsCvar.setValue(enabledDemoStats);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Shows stats on demos received and given to and from another player.");
    }

    ImGui::TextUnformatted("Debug Settings");

    CVarWrapper enableLoggingCvar = cvarManager->getCvar("logging_enabled");
    if (!enableLoggingCvar) { return; }
    bool enabledLogging = enableLoggingCvar.getBoolValue();
    if (ImGui::Checkbox("Enables logging in data/rivals folder", &enabledLogging)) {
        enableLoggingCvar.setValue(enabledLogging);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Enables logging in data/rivals folder");
    }
}