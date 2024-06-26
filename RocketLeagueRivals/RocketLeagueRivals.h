#pragma once
#ifndef ROCKETLEAGUERIVALS_H
#define ROCKETLEAGUERIVALS_H

#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "bakkesmod/wrappers/GameWrapper.h"
#include "bakkesmod/wrappers/includes.h"
#include <map>
#include <fstream>
#include <sstream>
#include "third_party/json.hpp"
#include <unordered_map>

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

using json = nlohmann::json;

struct PlayerInfo {
    std::string name;
    int winsWith;
    int lossesWith;
    int winsAgainst;
    int lossesAgainst;
    int team;
    std::string timestamp;

    // Convert PlayerInfo to JSON
    json to_json() const {
        return json{
            {"name", name},
            {"winsWith", winsWith},
            {"lossesWith", lossesWith},
            {"winsAgainst", winsAgainst},
            {"lossesAgainst", lossesAgainst},
            {"team", team},
            {"timestamp", timestamp}
        };
    }

    // Create PlayerInfo from JSON
    static PlayerInfo from_json(const json& j) {
        return PlayerInfo{
            j.at("name").get<std::string>(),
            j.at("winsWith").get<int>(),
            j.at("lossesWith").get<int>(),
            j.at("winsAgainst").get<int>(),
            j.at("lossesAgainst").get<int>(),
            j.at("team").get<int>(),
            j.at("timestamp").get<std::string>()
        };
    }
};

struct TriggerGoalScoreParams {
    int TeamScoredOn;
    uintptr_t Scorer;
};

class RocketLeagueRivals : public BakkesMod::Plugin::BakkesModPlugin,
    public SettingsWindowBase
{
private:
    std::map<std::string, PlayerInfo> playerData;
    std::unordered_map<std::string, PlayerInfo> activePlayers;

    int myTeam;
    bool matchStarted = false;

    void OnMatchStarted(ServerWrapper server);
    void OnMatchEnded(ServerWrapper server);
    void Render(CanvasWrapper canvas);
    void ReadJSON();
    void WriteJSON();
    bool DidWin(ServerWrapper server);
    struct TeamScores {
        int team0Score = 0;
        int team1Score = 0;
    };

    TeamScores scores;
    void KeepScore(ServerWrapper server, void* params);

    // Helper functions for rendering
    void DrawHeader(CanvasWrapper& canvas, const std::string& text, int xOffset, int& yOffset, int width, int headerHeight, int padding, float fontSize, float fontScale);
    void DrawPlayerInfo(CanvasWrapper& canvas, const PlayerInfo& player, int xOffset, int& yOffset, int width, int playerHeight, int lineHeight, int padding, float playerFontSize, float playerFontScale, float statFontSize, float statFontScale, bool isMyTeam);
    void RenderTeam(CanvasWrapper& canvas, const std::string& header, const std::vector<PlayerInfo>& players, int xOffset, int& yOffset, int width, int headerHeight, int playerHeight, int lineHeight, int padding, float headerFontSize, float headerFontScale, float playerFontSize, float playerFontScale, float statFontSize, float statFontScale, bool isMyTeam);

public:
    virtual void onLoad() override;
    virtual void onUnload() override;
    void RenderSettings() override;
};

#endif // ROCKETLEAGUERIVALS_H
