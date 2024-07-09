#include "pch.h"
#include "RocketLeagueRivals.h"
#include <algorithm>
#include "bakkesmod/wrappers/MMRWrapper.h"
#include "bakkesmod/wrappers/UniqueIDWrapper.h"
#include "bakkesmod/wrappers/GameObject/PriWrapper.h"
#include "bakkesmod/wrappers/GameObject/CarWrapper.h"
#include "bakkesmod/wrappers/Engine/EngineTAWrapper.h"
#include "bakkesmod/wrappers/GameObject/Stats/StatEventWrapper.h"
#include <ctime>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

BAKKESMOD_PLUGIN(RocketLeagueRivals, "Track player interactions in Rocket League", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;
std::string dataFolder;
std::string playerDataFolder;
std::string localPlayerName;
std::string logPath;

bool rightAlignEnabled = false;
bool loggingEnabled = false;
bool hideMyTeamEnabled = false;
bool hideRivalTeamEnabled = false;
bool showAllWinsLosesStatsEnabled = false;
bool showTeamStatsEnabled = false;
bool showDemoStatsEnabled = false;
bool showRivalStatsEnabled = true;
std::string rivalNumberString = "3";

void LogToFile(const std::string& message) {
    if (!_globalCvarManager) return;
    _globalCvarManager->log(message);
    if (!loggingEnabled) return;

    std::ofstream logFile(logPath, std::ios_base::app);
    logFile << message << std::endl;
    logFile.close();
}

json PlayerInfo::to_json() const {
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

PlayerInfo PlayerInfo::from_json(const json& j) {
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

void RocketLeagueRivals::onLoad() {
    _globalCvarManager = cvarManager;
    dataFolder = gameWrapper->GetDataFolder().string();
    playerDataFolder = dataFolder + "/rivals/players";
    logPath = dataFolder + "/rivals/rocketleaguerivals.log";

    if (fs::create_directories(playerDataFolder)) {
        LogToFile("Created player data directory: " + playerDataFolder);
    }
    else {
        LogToFile("Player data directory already exists or failed to create: " + playerDataFolder);
    }

    auto registerAndBindBoolCvar = [&](const char* name, const char* defaultValue, const char* description, bool& variable) {
        cvarManager->registerCvar(name, defaultValue, description, true, true, 0, true, 1)
            .addOnValueChanged([&variable](std::string oldValue, CVarWrapper cvar) {
            variable = cvar.getBoolValue();
                });
        };

    auto registerAndBindStringCvar = [&](const char* name, const char* defaultValue, const char* description, std::string& variable) {
        cvarManager->registerCvar(name, defaultValue, description, true, true, 0, true, 1)
            .addOnValueChanged([&variable](std::string oldValue, CVarWrapper cvar) {
            variable = cvar.getStringValue();
                });
        };

    registerAndBindBoolCvar("align_display_right", "0", "Enable Right Side Render", rightAlignEnabled);
    registerAndBindBoolCvar("folder_logging_enabled", "0", "Enable Logging", loggingEnabled);
    registerAndBindBoolCvar("hide_my_team_enabled", "0", "Enable Hiding My Team", hideMyTeamEnabled);
    registerAndBindBoolCvar("hide_rival_team_enabled", "0", "Enable Hiding Rival Team", hideRivalTeamEnabled);
    registerAndBindBoolCvar("show_all_winsloses_stats_enabled", "0", "Enable Show All Wins/Loses Stats", showAllWinsLosesStatsEnabled);
    registerAndBindBoolCvar("show_team_stats_enabled", "0", "Enable Show Team Stats", showTeamStatsEnabled);
    registerAndBindBoolCvar("show_demo_stats_enabled", "0", "Enable Demo Stats", showDemoStatsEnabled);
    registerAndBindBoolCvar("show_rival_stats_enabled", "1", "Enable Rival Stats", showRivalStatsEnabled);
    registerAndBindStringCvar("set_rival_number", "3", "Enable Demo Stats", rivalNumberString);

    LogToFile("RocketLeagueRivals plugin loaded");

    LogToFile("Attempting to hook into match start event");
    gameWrapper->HookEventWithCaller<ServerWrapper>("Function GameEvent_Soccar_TA.Active.StartRound", std::bind(&RocketLeagueRivals::OnRoundStart, this, std::placeholders::_1));
    LogToFile("Attempting to hook into match end event");
    gameWrapper->HookEventWithCaller<ServerWrapper>("Function TAGame.AchievementSystem_TA.CheckWonMatch", std::bind(&RocketLeagueRivals::OnMatchEnded, this));
    gameWrapper->HookEventWithCaller<ServerWrapper>("Function TAGame.GFxShell_TA.LeaveMatch", std::bind(&RocketLeagueRivals::OnMatchEnded, this));
    gameWrapper->HookEventWithCaller<ServerWrapper>("Function TAGame.GameEvent_Soccar_TA.TriggerGoalScoreEvent", std::bind(&RocketLeagueRivals::KeepScore, this, std::placeholders::_1, std::placeholders::_2));
    gameWrapper->HookEventWithCaller<ServerWrapper>("Function TAGame.GFxHUD_TA.HandleStatTickerMessage", std::bind(&RocketLeagueRivals::OnStatTickerMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    gameWrapper->RegisterDrawable(std::bind(&RocketLeagueRivals::Render, this, std::placeholders::_1));
}

void RocketLeagueRivals::onUnload() {
    LogToFile("RocketLeagueRivals plugin unloaded");
    WriteJSON();
    gameWrapper->UnhookEvent("Function GameEvent_Soccar_TA.Active.StartRound");
    gameWrapper->UnhookEvent("Function TAGame.AchievementSystem_TA.CheckWonMatch");
    gameWrapper->UnhookEvent("Function TAGame.GFxShell_TA.LeaveMatch");
    gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.TriggerGoalScoreEvent");
    gameWrapper->UnhookEvent("Function TAGame.GFxHUD_TA.HandleStatTickerMessage");
}

void RocketLeagueRivals::ReadJSON() {
    LogToFile("Reading JSON");

    for (const auto& entry : fs::directory_iterator(playerDataFolder)) {
        std::ifstream file(entry.path());
        if (!file.is_open()) {
            LogToFile("Failed to open " + entry.path().string());
            continue;
        }

        json j;
        file >> j;
        PlayerInfo player = PlayerInfo::from_json(j);
        playerData[player.name] = player;

        file.close();
    }

    LogToFile("Finished reading JSON");
}

void RocketLeagueRivals::WriteJSON() {
    LogToFile("Writing to JSON");

    for (const auto& [name, player] : activePlayers) {
        std::string sanitizedPlayerName = SanitizeFileName(name);
        std::string filePath = playerDataFolder + "/" + sanitizedPlayerName + ".json";
        std::ofstream outputFile(filePath);
        if (!outputFile.is_open()) {
            LogToFile("Failed to open " + filePath + " for writing");
            continue;
        }
        LogToFile(filePath);
        json j = player.to_json();
        outputFile << j.dump(4);
        outputFile.close();
    }

    LogToFile("Finished writing to JSON");
}

std::string RocketLeagueRivals::SanitizeFileName(const std::string& filename) {
    static const std::string invalidChars = "<>:\"/\\|?*";
    std::string sanitized = filename;
    for (char c : invalidChars) {
        std::replace(sanitized.begin(), sanitized.end(), c, '_');
    }
    return sanitized;
}

void RocketLeagueRivals::OnRoundStart(ServerWrapper server) {
    matchStarted = true;

    int timeRemaing = server.GetSecondsRemaining();
    
    if (timeRemaing == 300 && activePlayers.size() > 0) {
        LogToFile("Clearing last games active players");
        OnMatchEnded();
    }

    LogToFile("Match started");
    if (server.IsNull()) {
        LogToFile("Server is null");
        return;
    }

    auto localPlayer = gameWrapper->GetLocalCar();
    if (localPlayer.IsNull()) {
        return;
    }

    auto localPri = localPlayer.GetPRI();
    if (localPri.IsNull()) {
        return;
    }

    localPlayerName = localPri.GetPlayerName().ToString();
    auto players = server.GetCars();
    if (players.IsNull()) {
        LogToFile("Players are null");
        return;
    }

    std::time_t now = std::time(nullptr);
    std::string timestamp = std::ctime(&now);

    for (int i = 0; i < players.Count(); ++i) {
        auto player = players.Get(i);
        if (player.IsNull()) continue;

        PriWrapper pri = player.GetPRI();
        if (pri.IsNull()) continue;

        std::string playerName = pri.GetPlayerName().ToString();
        std::string sanitizedPlayerName = SanitizeFileName(playerName);

        if (playerName == localPlayerName) {
            myTeam = pri.GetTeamNum();
        }

        if (playerName == localPlayerName) continue;

        if (activePlayers.find(playerName) == activePlayers.end()) {
            std::string filePath = playerDataFolder + "/" + sanitizedPlayerName + ".json";
            if (fs::exists(filePath)) {
                std::ifstream file(filePath);
                if (file.is_open()) {
                    json j;
                    file >> j;
                    PlayerInfo player = PlayerInfo::from_json(j);
                    player.team = pri.GetTeamNum();
                    player.timestamp = timestamp;
                    activePlayers[playerName] = player;
                    file.close();
                }
            }
            else {
                activePlayers[playerName] = PlayerInfo{ playerName, 0, 0, 0, 0, pri.GetTeamNum(), timestamp, 0, 0 };
            }
        }
    }
}

void RocketLeagueRivals::OnMatchEnded() {
    matchStarted = false;

    LogToFile("Match ended");
    bool won = DidWin();

    for (auto& [name, player] : activePlayers) {
        if (name == localPlayerName) {
            continue;
        }

        if (player.team == myTeam) {
            if (won) {
                player.winsWith++;
            }
            else {
                player.lossesWith++;
            }
        }
        else {
            if (won) {
                player.winsAgainst++;
            }
            else {
                player.lossesAgainst++;
            }
        }
    }

    WriteJSON();
    activePlayers.clear();
    scores = { 0, 0 };
}

void RocketLeagueRivals::OnStatTickerMessage(ServerWrapper caller, void* params, std::string eventname) {
    StatTickerParams* pStruct = reinterpret_cast<StatTickerParams*>(params);
    PriWrapper receiver = PriWrapper(pStruct->Receiver);
    PriWrapper victim = PriWrapper(pStruct->Victim);
    StatEventWrapper statEvent = StatEventWrapper(pStruct->StatEvent);

    if (statEvent.GetEventName() == "Demolish") {
        std::string receiverName = receiver.GetPlayerName().ToString();
        std::string victimName = victim.GetPlayerName().ToString();

        RecordDemo(receiverName, victimName);
    }
}

void RocketLeagueRivals::RecordDemo(const std::string& receiverName, const std::string& victimName) {
    if (receiverName == localPlayerName) {
        if (activePlayers.find(victimName) != activePlayers.end()) {
            activePlayers[victimName].demosGiven++;
            LogToFile(victimName + " demos given: " + std::to_string(activePlayers[victimName].demosGiven));
        }
    }
    else if (victimName == localPlayerName) {
        if (activePlayers.find(receiverName) != activePlayers.end()) {
            activePlayers[receiverName].demosReceived++;
            LogToFile(receiverName + " demos received: " + std::to_string(activePlayers[receiverName].demosReceived));
        }
    }
}

void RocketLeagueRivals::KeepScore(ServerWrapper caller, void* params) {
    TriggerGoalScoreParams* goalParams = reinterpret_cast<TriggerGoalScoreParams*>(params);
    if (goalParams->TeamScoredOn == 0) {
        scores.team1Score++;
    }
    else if (goalParams->TeamScoredOn == 1) {
        scores.team0Score++;
    }

    LogToFile("Goal scored! Team 0: " + std::to_string(scores.team0Score) + " Team 1: " + std::to_string(scores.team1Score));
}

bool RocketLeagueRivals::DidWin() {
    int localTeam = myTeam;

    bool won = (localTeam == 0 && scores.team0Score > scores.team1Score) || (localTeam == 1 && scores.team1Score > scores.team0Score);

    LogToFile("Match ended with scores - Team 0: " + std::to_string(scores.team0Score) + ", Team 1: " + std::to_string(scores.team1Score));
    LogToFile(won ? "Local player won the match" : "Local player lost the match");

    return won;
}

void RocketLeagueRivals::Render(CanvasWrapper canvas) {
    ServerWrapper server = gameWrapper->GetCurrentGameState();
    if (!server) {
        return;
    }

    auto localPlayer = gameWrapper->GetLocalCar();
    if (localPlayer.IsNull()) {
        return;
    }

    auto localPri = localPlayer.GetPRI();
    if (localPri.IsNull()) {
        return;
    }

    std::string localPlayerName = localPri.GetPlayerName().ToString();

    auto players = server.GetCars();
    if (players.IsNull()) {
        return;
    }

    Vector2 canvasSize = canvas.GetSize();
    int canvasHeight = canvasSize.Y;

    int xOffset = 20;
    if (rightAlignEnabled) {
        xOffset = canvasSize.X - 220;
    }
    int yOffset = canvasHeight / 2 - ((players.Count() * 60) / 2);

    std::vector<PlayerInfo> myTeamPlayers;
    std::vector<PlayerInfo> rivalTeamPlayers;

    for (int i = 0; i < players.Count(); ++i) {
        auto player = players.Get(i);
        if (player.IsNull()) continue;

        PriWrapper pri = player.GetPRI();
        if (pri.IsNull()) continue;

        std::string playerName = pri.GetPlayerName().ToString();
        if (playerName == localPlayerName) {
            continue;
        }

        if (activePlayers.find(playerName) != activePlayers.end()) {
            const PlayerInfo& player = activePlayers[playerName];
            if (player.team == myTeam) {
                myTeamPlayers.push_back(player);
            }
            else {
                rivalTeamPlayers.push_back(player);
            }
        }
    }

    if (!hideMyTeamEnabled) {
        RenderTeam(canvas, "My Team", myTeamPlayers, xOffset, yOffset, true);
    }
    if (!hideRivalTeamEnabled) {
        RenderTeam(canvas, "Rival Team", rivalTeamPlayers, xOffset, yOffset, false);
    }
}

void DrawText(CanvasWrapper& canvas, const std::string& text, int x, int y, float fontSize, float fontScale, const std::tuple<int, int, int, int>& color) {
    canvas.SetColor(std::get<0>(color), std::get<1>(color), std::get<2>(color), std::get<3>(color));
    canvas.SetPosition(Vector2(x, y));
    canvas.DrawString(text, fontSize, fontScale);
}

void RocketLeagueRivals::DrawHeader(CanvasWrapper& canvas, const std::string& text, int xOffset, int& yOffset) {
    const RenderConfig& config = renderConfig;
    canvas.SetColor(0, 0, 0, 128);
    canvas.DrawRect(Vector2(xOffset - config.padding, yOffset + 2 - config.padding), Vector2(xOffset + config.width + config.padding, yOffset + 2 + config.headerHeight + config.padding));
    DrawText(canvas, text, xOffset + 5, yOffset, config.headerFontSize, config.headerFontScale, neutralColor);
    yOffset += config.headerHeight + config.padding * 2;
}

void DrawStat(CanvasWrapper& canvas, const std::string& label, float statValue, int x, int& y, float fontSize, float fontScale, const std::tuple<int, int, int, int>& color, int lineHeight) {
    std::stringstream ss;
    ss << label << ": " << std::fixed << std::setprecision(1) << statValue << "%";
    DrawText(canvas, ss.str(), x, y, fontSize, fontScale, color);
}

std::tuple<int, int, int, int> GetStatColor(int wins, int losses, const std::tuple<int, int, int, int>& defaultColor, const std::tuple<int, int, int, int>& winColor, const std::tuple<int, int, int, int>& lossColor) {
    if (wins == 0 && losses == 0) {
        return defaultColor;
    }
    return wins > losses ? winColor : lossColor;
}

void RocketLeagueRivals::DrawPlayerInfo(CanvasWrapper& canvas, const PlayerInfo& player, int xOffset, int& yOffset, bool isMyTeam) {
    const RenderConfig& config = renderConfig;

    int adjustedPlayerHeight = config.playerHeight / 2;
    if (showAllWinsLosesStatsEnabled) {
        adjustedPlayerHeight += config.lineHeight + config.padding;
        if (!showTeamStatsEnabled) {
            adjustedPlayerHeight += config.lineHeight;
        }
    }
    if (showDemoStatsEnabled) {
        adjustedPlayerHeight += config.lineHeight;
    }
    if (showTeamStatsEnabled) {
        adjustedPlayerHeight += config.lineHeight;
    }
    if (showRivalStatsEnabled) {
        adjustedPlayerHeight += config.lineHeight;
    }

    float rivalryScore = CalculateRivalryScore(player);
    std::stringstream geek;
    geek << rivalNumberString;
    int rivalNumber;
    geek >> rivalNumber;
    if (!(rivalNumber > 0)) {
        rivalNumber = 5;
    }
    bool isRival = rivalryScore > rivalNumber;

    std::ostringstream scoreStream;
    scoreStream << std::fixed << std::setprecision(1) << rivalryScore;
    std::string rivalryScoreStr = scoreStream.str();

    canvas.SetColor(0, 0, 0, 128);
    canvas.DrawRect(Vector2(xOffset - config.padding, yOffset + 2 - config.padding), Vector2(xOffset + config.width + config.padding, yOffset + 2 + adjustedPlayerHeight));

    std::string displayName = player.name.length() > 18 ? player.name.substr(0, 15) + "..." : player.name;
    if (!showRivalStatsEnabled) {
        if (isRival) {
            DrawText(canvas, displayName, xOffset + 5, yOffset, config.playerFontSize, config.playerFontScale, redColor);
        }
        else {
            DrawText(canvas, displayName, xOffset + 5, yOffset, config.playerFontSize, config.playerFontScale, lightBlueColor);
        }
    }
    else {
        DrawText(canvas, displayName, xOffset + 5, yOffset, config.playerFontSize, config.playerFontScale, neutralColor);
    }

    if (showRivalStatsEnabled) {
        yOffset += config.lineHeight;
        std::string rivalText;
        if (isRival) {
            rivalText = "Rivalry Score: " + rivalryScoreStr;
            DrawText(canvas, rivalText, xOffset + 10, yOffset, config.statFontSize, config.statFontScale, redColor);
        }
        else {
            std::tuple<int, int, int, int> colorDetermination = rivalryScore == 0 ? neutralColor : lightBlueColor;
            rivalText = "Rivalry Score: " + rivalryScoreStr;
            DrawText(canvas, rivalText, xOffset + 10, yOffset, config.statFontSize, config.statFontScale, colorDetermination);
        }
    }

    if (showDemoStatsEnabled) {
        yOffset += config.lineHeight;
        std::stringstream ssDemos;
        ssDemos << "Demos to/from: " << player.demosGiven << " : " << player.demosReceived;
        DrawText(canvas, ssDemos.str(), xOffset + 10, yOffset, config.statFontSize, config.statFontScale, neutralColor);
    }

    if (showAllWinsLosesStatsEnabled || (isMyTeam && showTeamStatsEnabled)) {
        yOffset += config.lineHeight;
        float totalWithGames = static_cast<float>(player.winsWith + player.lossesWith);
        float winWithPercentage = totalWithGames == 0 ? 0 : (player.winsWith / totalWithGames) * 100.0f;
        auto color = GetStatColor(player.winsWith, player.lossesWith, neutralColor, lightBlueColor, orangeColor);
        DrawStat(canvas, "Won with", winWithPercentage, xOffset + 10, yOffset, config.statFontSize, config.statFontScale, color, config.lineHeight);
    }

    if (showAllWinsLosesStatsEnabled || (!isMyTeam && showTeamStatsEnabled)) {
        yOffset += config.lineHeight;
        float totalAgainstGames = static_cast<float>(player.winsAgainst + player.lossesAgainst);
        float winAgainstPercentage = totalAgainstGames == 0 ? 0 : (player.winsAgainst / totalAgainstGames) * 100.0f;
        auto color = GetStatColor(player.winsAgainst, player.lossesAgainst, neutralColor, { 0, 255, 0, 255 }, redColor);
        DrawStat(canvas, "Won against", winAgainstPercentage, xOffset + 10, yOffset, config.statFontSize, config.statFontScale, color, config.lineHeight);
    }

    yOffset += config.lineHeight + config.padding;
}

void RocketLeagueRivals::RenderTeam(CanvasWrapper& canvas, const std::string& header, const std::vector<PlayerInfo>& players, int xOffset, int& yOffset, bool isMyTeam) {
    if (!players.empty()) {
        DrawHeader(canvas, header, xOffset, yOffset);
        for (const auto& player : players) {
            DrawPlayerInfo(canvas, player, xOffset, yOffset, isMyTeam);
        }
        yOffset += renderConfig.headerHeight;
    }
}

int GetWinAgainstPoints(float winPercentageAgainst) {
    if (winPercentageAgainst <= 10) return 5;
    else if (winPercentageAgainst <= 20) return 4;
    else if (winPercentageAgainst <= 30) return 3;
    else if (winPercentageAgainst <= 40) return 2;
    else if (winPercentageAgainst <= 50) return 1;
    else return 0;
}

int GetWinWithPoints(float winPercentageWith) {
    if (winPercentageWith <= 10) return 5;
    else if (winPercentageWith <= 20) return 4;
    else if (winPercentageWith <= 30) return 3;
    else if (winPercentageWith <= 40) return 2;
    else if (winPercentageWith <= 50) return 1;
    else if (winPercentageWith <= 60) return -1;
    else if (winPercentageWith <= 70) return -2;
    else if (winPercentageWith <= 80) return -3;
    else if (winPercentageWith <= 90) return -4;
    else return -5;
}

int GetDemosPoints(int demos) {
    if (demos <= 2) return 1;
    else if (demos <= 5) return 2;
    else return 5;
}

int GetFrequencyPoints(int encounters) {
    if (encounters <= 3) return 1;
    else if (encounters <= 6) return 2;
    else return 3;
}

float RocketLeagueRivals::CalculateRivalryScore(const PlayerInfo& player) {
    float totalGamesWith = static_cast<float>(player.winsWith + player.lossesWith);
    float winPercentageWith = totalGamesWith == 0 ? 0 : (player.winsWith / totalGamesWith) * 100.0f;

    float totalGamesAgainst = static_cast<float>(player.winsAgainst + player.lossesAgainst);
    float winPercentageAgainst = totalGamesAgainst == 0 ? 0 : (player.winsAgainst / totalGamesAgainst) * 100.0f;

    int winAgainstPoints = totalGamesAgainst == 0 ? 0 : GetWinAgainstPoints(winPercentageAgainst);
    int winWithPoints = totalGamesWith == 0 ? 0 : GetWinWithPoints(winPercentageWith);
    int demosPoints = player.demosReceived == 0 ? 0 : GetDemosPoints(player.demosReceived);
    int totalEncounters = player.winsWith + player.lossesWith + player.winsAgainst + player.lossesAgainst;
    int frequencyPoints = GetFrequencyPoints(totalEncounters);

    float rivalryScore = (winAgainstPoints + winWithPoints) * 0.4 + demosPoints * 0.3 + frequencyPoints * 0.3;
    return totalGamesAgainst + totalGamesWith == 0 ? demosPoints * 0.3 : rivalryScore < 0 ? demosPoints * 0.3 : rivalryScore;
}
