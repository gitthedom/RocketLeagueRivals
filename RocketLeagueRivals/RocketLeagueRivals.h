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
#include <filesystem>

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
    int demosGiven;
    int demosReceived;

    // Convert PlayerInfo to JSON
    json to_json() const {
        return json{
            {"name", name},
            {"winsWith", winsWith},
            {"lossesWith", lossesWith},
            {"winsAgainst", winsAgainst},
            {"lossesAgainst", lossesAgainst},
            {"team", team},
            {"timestamp", timestamp},
            {"demosGiven", demosGiven},
            {"demosReceived", demosReceived}
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
            j.at("timestamp").get<std::string>(),
            j.at("demosGiven").get<int>(),
            j.at("demosReceived").get<int>()
        };
    }
};

struct TriggerGoalScoreParams {
    int TeamScoredOn;
    uintptr_t Scorer;
};

struct StatTickerParams {
    uintptr_t Receiver;
    uintptr_t Victim;
    uintptr_t StatEvent;
};

class RocketLeagueRivals : public BakkesMod::Plugin::BakkesModPlugin,
    public SettingsWindowBase
{
private:
    std::map<std::string, PlayerInfo> playerData;
    std::unordered_map<std::string, PlayerInfo> activePlayers;

    int myTeam;
    bool matchStarted = false;

    bool rightAlignEnabled = false;
    bool loggingEnabled = false;
    bool hideMyTeamEnabled = false;
    bool hideRivalTeamEnabled = false;
    bool showAllStatsEnabled = false;

    void OnMatchStarted(ServerWrapper server);
    void OnMatchEnded(ServerWrapper server);
    void Render(CanvasWrapper canvas);
    void ReadJSON();
    void WriteJSON();
    bool DidWin(ServerWrapper server);
    void KeepScore(ServerWrapper server, void* params);
    void OnStatTickerMessage(ServerWrapper caller, void* params, std::string eventname);
    void RecordDemo(const std::string& receiverName, const std::string& victimName);

    struct TeamScores {
        int team0Score = 0;
        int team1Score = 0;
    };

    TeamScores scores;

    // Helper functions for rendering
    void DrawHeader(CanvasWrapper& canvas, const std::string& text, int xOffset, int& yOffset, int width, int headerHeight, int padding, float fontSize, float fontScale);
    void DrawPlayerInfo(CanvasWrapper& canvas, const PlayerInfo& player, int xOffset, int& yOffset, int width, int playerHeight, int lineHeight, int padding, float playerFontSize, float playerFontScale, float statFontSize, float statFontScale, bool isMyTeam);
    void RenderTeam(CanvasWrapper& canvas, const std::string& header, const std::vector<PlayerInfo>& players, int xOffset, int& yOffset, int width, int headerHeight, int playerHeight, int lineHeight, int padding, float headerFontSize, float headerFontScale, float playerFontSize, float playerFontScale, float statFontSize, float statFontScale, bool isMyTeam);

    // Function to sanitize file names
    std::string SanitizeFileName(const std::string& filename);

public:
    virtual void onLoad() override;
    virtual void onUnload() override;
    void RenderSettings() override;
};

#endif // ROCKETLEAGUERIVALS_H
