#pragma once

#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "bakkesmod/wrappers/GameWrapper.h"
#include "bakkesmod/wrappers/includes.h"
#include <map>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
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

    json to_json() const;
    static PlayerInfo from_json(const json& j);
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

class RocketLeagueRivals : public BakkesMod::Plugin::BakkesModPlugin, public SettingsWindowBase {
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

    const std::tuple<int, int, int, int> neutralColor = { 255, 255, 255, 255 };
    const std::tuple<int, int, int, int> lightBlueColor = { 56, 142, 235, 255 };
    const std::tuple<int, int, int, int> redColor = { 255, 0, 0, 255 };
    const std::tuple<int, int, int, int> orangeColor = { 255, 165, 0, 255 };

    void OnRoundStart(ServerWrapper server);
    void OnMatchEnded();
    void Render(CanvasWrapper canvas);
    void ReadJSON();
    void WriteJSON();
    bool DidWin();
    void KeepScore(ServerWrapper server, void* params);
    void OnStatTickerMessage(ServerWrapper caller, void* params, std::string eventname);
    void RecordDemo(const std::string& receiverName, const std::string& victimName);

    struct TeamScores {
        int team0Score = 0;
        int team1Score = 0;
    };

    TeamScores scores;

    void DrawHeader(CanvasWrapper& canvas, const std::string& text, int xOffset, int& yOffset);
    void DrawPlayerInfo(CanvasWrapper& canvas, const PlayerInfo& player, int xOffset, int& yOffset, bool isMyTeam);
    void RenderTeam(CanvasWrapper& canvas, const std::string& header, const std::vector<PlayerInfo>& players, int xOffset, int& yOffset, bool isMyTeam);

    std::string SanitizeFileName(const std::string& filename);

    struct RenderConfig {
        float headerFontSize = 2.0f;
        float headerFontScale = 2.0f;
        float playerFontSize = 1.5f;
        float playerFontScale = 1.5f;
        float statFontSize = 1.2f;
        float statFontScale = 1.2f;
        int padding = 0;
        int lineHeight = 20;
        int headerHeight = 30;
        int playerHeight = lineHeight * 2 + padding;
        int width = 200;
    };

    RenderConfig renderConfig;

public:
    virtual void onLoad() override;
    virtual void onUnload() override;
    void RenderSettings() override;
    float CalculateRivalryScore(const PlayerInfo& player);
};
