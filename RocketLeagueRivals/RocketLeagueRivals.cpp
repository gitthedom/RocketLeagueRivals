#include "pch.h"
#include "RocketLeagueRivals.h"
#include "bakkesmod/wrappers/MMRWrapper.h"
#include "bakkesmod/wrappers/UniqueIDWrapper.h"
#include "bakkesmod/wrappers/GameObject/PriWrapper.h"
#include "bakkesmod/wrappers/GameObject/CarWrapper.h"
#include "bakkesmod/wrappers/Engine/EngineTAWrapper.h"
#include "bakkesmod/wrappers/GameObject/Stats/StatEventWrapper.h"
#include <ctime>

BAKKESMOD_PLUGIN(RocketLeagueRivals, "Track player interactions in Rocket League", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;
std::string dataFolder;
std::string localPlayerName;

bool rightAlignEnabled = false;
bool loggingEnabled = false;
bool hideMyTeamEnabled = false;
bool hideRivalTeamEnabled = false;
bool showAllWinsLosesStatsEnabled = false;
bool showDemoStatsEnabled = false;

void LogToFile(const std::string& message) {
    if (!loggingEnabled) return;
    if (!_globalCvarManager) return; // Ensure _globalCvarManager is valid
    _globalCvarManager->log(message);
    std::string logPath = dataFolder + "/rivals/rocketleaguerivals.log";
    std::ofstream logFile(logPath, std::ios_base::app);
    logFile << message << std::endl;
    logFile.close();
}

void RocketLeagueRivals::onLoad() {
    _globalCvarManager = cvarManager;
    dataFolder = gameWrapper->GetDataFolder().string();

    cvarManager->registerCvar("align_display_right", "0", "Enable Right Side Render", true, true, 0, true, 1)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
        rightAlignEnabled = cvar.getBoolValue();
            });

    cvarManager->registerCvar("logging_enabled", "0", "Enable Logging", true, true, 0, true, 1)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
        loggingEnabled = cvar.getBoolValue();
            });

    cvarManager->registerCvar("hide_my_team_enabled", "0", "Enable Hiding My Team", true, true, 0, true, 1)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
        hideMyTeamEnabled = cvar.getBoolValue();
            });

    cvarManager->registerCvar("hide_rival_team_enabled", "0", "Enable Hiding Rival Team", true, true, 0, true, 1)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
        hideRivalTeamEnabled = cvar.getBoolValue();
            });

    cvarManager->registerCvar("show_all_winsloses_stats_enabled", "0", "Enable Show All Wins/Loses Stats", true, true, 0, true, 1)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
        showAllWinsLosesStatsEnabled = cvar.getBoolValue();
            });

    cvarManager->registerCvar("show_demo_stats_enabled", "0", "Enable Demo Stats", true, true, 0, true, 1)
        .addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
        showDemoStatsEnabled = cvar.getBoolValue();
            });

    LogToFile("RocketLeagueRivals plugin loaded");

    ReadJSON();
    LogToFile("Attempting to hook into match start event");
    gameWrapper->HookEventWithCaller<ServerWrapper>("Function GameEvent_Soccar_TA.Active.StartRound", std::bind(&RocketLeagueRivals::OnMatchStarted, this, std::placeholders::_1));
    LogToFile("Attempting to hook into match end event");
    gameWrapper->HookEventWithCaller<ServerWrapper>("Function TAGame.AchievementSystem_TA.CheckWonMatch", std::bind(&RocketLeagueRivals::OnMatchEnded, this, std::placeholders::_1));
    gameWrapper->HookEventWithCaller<ServerWrapper>("Function TAGame.GFxShell_TA.LeaveMatch", std::bind(&RocketLeagueRivals::OnMatchEnded, this, std::placeholders::_1));
    gameWrapper->HookEventWithCaller<ServerWrapper>("Function TAGame.GameEvent_Soccar_TA.TriggerGoalScoreEvent", std::bind(&RocketLeagueRivals::KeepScore, this, std::placeholders::_1, std::placeholders::_2));
    gameWrapper->HookEventWithCaller<ServerWrapper>("Function TAGame.GFxHUD_TA.HandleStatTickerMessage", std::bind(&RocketLeagueRivals::OnStatTickerMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    gameWrapper->RegisterDrawable(std::bind(&RocketLeagueRivals::Render, this, std::placeholders::_1));
}

void RocketLeagueRivals::onUnload() {
    LogToFile("RocketLeagueRivals plugin unloaded");
    WriteJSON();
    gameWrapper->UnhookEvent("Function GameEvent_Soccar_TA.Active.StartRound");
    gameWrapper->UnhookEvent("Function TAGame.AchievementSystem_TA.CheckWonMatch");
    gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.TriggerGoalScoreEvent");
    gameWrapper->UnhookEvent("Function TAGame.GFxShell_TA.LeaveMatch");
    gameWrapper->UnhookEvent("Function TAGame.GFxHUD_TA.HandleStatTickerMessage");
}

void RocketLeagueRivals::ReadJSON() {
    LogToFile("Reading JSON");
    std::string jsonPath = dataFolder + "/rivals/player_data.json";
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        LogToFile("player_data.json not found, creating new file");
        std::ofstream newFile(jsonPath);
        newFile.close();
        return;
    }

    json j;
    file >> j;

    for (const auto& item : j) {
        PlayerInfo player = PlayerInfo::from_json(item);
        playerData[player.name] = player;
    }

    file.close();
    LogToFile("Finished reading JSON");
}

void RocketLeagueRivals::WriteJSON() {
    LogToFile("Writing to JSON");

    std::string jsonPath = dataFolder + "/rivals/player_data.json";

    // Load existing JSON data into playerData
    ReadJSON();

    // Update playerData with activePlayers data
    for (const auto& [name, player] : activePlayers) {
        playerData[name] = player;
    }

    // Convert playerData to JSON array
    json updatedData = json::array();
    for (const auto& [name, player] : playerData) {
        updatedData.push_back(player.to_json());
    }

    // Write the updated JSON array to the file
    std::ofstream outputFile(jsonPath);
    if (!outputFile.is_open()) {
        LogToFile("Failed to open player_data.json for writing");
        return;
    }

    outputFile << updatedData.dump(4); // Pretty print with 4-space indent
    outputFile.close();
    LogToFile("Finished writing to JSON");
}

void RocketLeagueRivals::OnMatchStarted(ServerWrapper server) {
    matchStarted = true;

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
        if (playerName == localPlayerName) {
            myTeam = pri.GetTeamNum();
            continue; // Skip the local player
        }

        auto it = playerData.find(playerName);
        if (it != playerData.end()) {
            // Player exists, update team and timestamp in activePlayers
            activePlayers[playerName] = it->second;
            activePlayers[playerName].team = pri.GetTeamNum();
            activePlayers[playerName].timestamp = timestamp;
        }
        else {
            // Player does not exist, add new entry to activePlayers
            activePlayers[playerName] = PlayerInfo{ playerName, 0, 0, 0, 0, pri.GetTeamNum(), timestamp };
        }
    }
}

void RocketLeagueRivals::OnMatchEnded(ServerWrapper server) {
    matchStarted = false;

    LogToFile("Match ended");
    bool won = DidWin(server);

    for (auto& [name, player] : activePlayers) {
        if (name == localPlayerName) {
            continue; // Skip the local player
        }

        if (player.team == myTeam) {
            LogToFile("Before: " + player.name + " winsWith: " + std::to_string(player.winsWith) + ", lossesWith: " + std::to_string(player.lossesWith));
            if (won) {
                player.winsWith++;
            }
            else {
                player.lossesWith++;
            }
            LogToFile("After: " + player.name + " winsWith: " + std::to_string(player.winsWith) + ", lossesWith: " + std::to_string(player.lossesWith));
        }
        else {
            LogToFile("Before: " + player.name + " winsAgainst: " + std::to_string(player.winsAgainst) + ", lossesAgainst: " + std::to_string(player.lossesAgainst));
            if (won) {
                player.winsAgainst++;
            }
            else {
                player.lossesAgainst++;
            }
            LogToFile("After: " + player.name + " winsAgainst: " + std::to_string(player.winsAgainst) + ", lossesAgainst: " + std::to_string(player.lossesAgainst));
        }
    }

    WriteJSON();
    ReadJSON();

    scores = { 0, 0 }; // Reset the scores at the end of each match
    activePlayers.clear(); // Clear active players at the end of each match
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

bool RocketLeagueRivals::DidWin(ServerWrapper server) {
    if (server.IsNull()) {
        LogToFile("Server is null");
        return false;
    }

    int localTeam = myTeam; // This should be set correctly in your OnMatchStarted function

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

    // Get canvas size
    Vector2 canvasSize = canvas.GetSize();
    int canvasHeight = canvasSize.Y;

    // Set the x position for the left placement
    int xOffset = 20; // Adjust this value to fit your text properly
    if (rightAlignEnabled) {
        xOffset = canvasSize.X - 220; // Adjust for right alignment
    }
    int yOffset = canvasHeight / 2 - ((players.Count() * 60) / 2); // Centering vertically

    // Separate players into teams
    std::vector<PlayerInfo> myTeamPlayers;
    std::vector<PlayerInfo> rivalTeamPlayers;

    for (int i = 0; i < players.Count(); ++i) {
        auto player = players.Get(i);
        if (player.IsNull()) continue;

        PriWrapper pri = player.GetPRI();
        if (pri.IsNull()) continue;

        std::string playerName = pri.GetPlayerName().ToString();
        if (playerName == localPlayerName) {
            continue; // Skip the local player
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

    if (!hideMyTeamEnabled) {
        RenderTeam(canvas, "My Team", myTeamPlayers, xOffset, yOffset, width, headerHeight, playerHeight, lineHeight, padding, headerFontSize, headerFontScale, playerFontSize, playerFontScale, statFontSize, statFontScale, true);
    }
    if (!hideRivalTeamEnabled) {
        RenderTeam(canvas, "Rival Team", rivalTeamPlayers, xOffset, yOffset, width, headerHeight, playerHeight, lineHeight, padding, headerFontSize, headerFontScale, playerFontSize, playerFontScale, statFontSize, statFontScale, false);
    }
}


void RocketLeagueRivals::DrawHeader(CanvasWrapper& canvas, 
    const std::string& text, 
    int xOffset, 
    int& yOffset, 
    int width, 
    int headerHeight, 
    int padding, 
    float fontSize, 
    float fontScale) {
    canvas.SetColor(0, 0, 0, 128); // Transparent black background
    canvas.DrawRect(Vector2(xOffset - padding, yOffset - padding), Vector2(xOffset + width + padding, yOffset + headerHeight + padding));
    canvas.SetColor(255, 255, 255, 255); // White color for header
    canvas.SetPosition(Vector2(xOffset + 5, yOffset));
    canvas.DrawString(text, fontSize, fontScale);
    yOffset += headerHeight + padding * 2;
}

void RocketLeagueRivals::DrawPlayerInfo(CanvasWrapper& canvas,
    const PlayerInfo& player,
    int xOffset,
    int& yOffset,
    int width,
    int playerHeight,
    int lineHeight,
    int padding,
    float playerFontSize,
    float playerFontScale,
    float statFontSize,
    float statFontScale,
    bool isMyTeam) {

    // Determine background height based on the number of lines
    int adjustedPlayerHeight = playerHeight;
    if (showAllWinsLosesStatsEnabled) {
        adjustedPlayerHeight += lineHeight + padding; // Extra line for additional stats
    }

    if (showDemoStatsEnabled) {
        adjustedPlayerHeight += lineHeight; // Extra line for additional stats
    }

    // Draw background rectangle
    canvas.SetColor(0, 0, 0, 128); // Transparent black background
    canvas.DrawRect(Vector2(xOffset - padding, yOffset - padding), Vector2(xOffset + width + padding, yOffset + adjustedPlayerHeight));

    // Draw player name in white
    canvas.SetColor(255, 255, 255, 255); // White
    std::string displayName = player.name.length() > 18 ? player.name.substr(0, 15) + "..." : player.name;
    std::stringstream ss;
    ss << displayName;
    canvas.SetPosition(Vector2(xOffset + 5, yOffset));
    canvas.DrawString(ss.str(), playerFontSize, playerFontScale);
    yOffset += lineHeight;

    // Draw demos given (DG) and demos received (DR) in white
    if (showDemoStatsEnabled) {
        std::stringstream ssDemos;
        ssDemos << "DG: " << player.demosGiven << "   DR: " << player.demosReceived;
        canvas.SetColor(255, 255, 255, 255); // White
        canvas.SetPosition(Vector2(xOffset + 10, yOffset));
        canvas.DrawString(ssDemos.str(), statFontSize, statFontScale);
        yOffset += lineHeight;
    }

    // Determine color and draw other stats
    if (showAllWinsLosesStatsEnabled) {
        float totalWithGames = static_cast<float>(player.winsWith + player.lossesWith);
        float winWithPercentage = totalWithGames == 0 ? 0 : (player.winsWith / totalWithGames) * 100.0f;
        std::stringstream ssWinWithPercentage;
        ssWinWithPercentage << "Won with" << ": " << std::fixed << std::setprecision(1) << winWithPercentage << "%";

        // Draw "wins with/losses with" stats
        if (player.winsWith == 0 && player.lossesWith == 0) {
            canvas.SetColor(255, 255, 255, 255); // White
        }
        else if (player.winsWith > player.lossesWith) {
            canvas.SetColor(56, 142, 235, 255); // Blue
        }
        else {
            canvas.SetColor(255, 165, 0, 255); // Orange
        }
        canvas.SetPosition(Vector2(xOffset + 10, yOffset));
        canvas.DrawString(ssWinWithPercentage.str(), statFontSize, statFontScale);
        yOffset += lineHeight;

        float totalAgainstGames = static_cast<float>(player.winsAgainst + player.lossesAgainst);
        float winAgainstPercentage = totalAgainstGames == 0 ? 0 : (player.winsAgainst / totalAgainstGames) * 100.0f;
        std::stringstream ssWinAgainstPercentage;
        ssWinAgainstPercentage << "Won against" << ": " << std::fixed << std::setprecision(1) << winAgainstPercentage << "%";

        // Draw "wins against/losses against" stats
        if (player.winsAgainst == 0 && player.lossesAgainst == 0) {
            canvas.SetColor(255, 255, 255, 255); // White
        }
        else if (player.winsAgainst > player.lossesAgainst) {
            canvas.SetColor(0, 255, 0, 255); // Green
        }
        else {
            canvas.SetColor(255, 0, 0, 255); // Red
        }
        canvas.SetPosition(Vector2(xOffset + 10, yOffset));
        canvas.DrawString(ssWinAgainstPercentage.str(), statFontSize, statFontScale);
    }
    else {
        if (isMyTeam) {
            float totalWithGames = static_cast<float>(player.winsWith + player.lossesWith);
            float winWithPercentage = totalWithGames == 0 ? 0 : (player.winsWith / totalWithGames) * 100.0f;
            std::stringstream ssWinWithPercentage;
            ssWinWithPercentage << "Won with" << ": " << std::fixed << std::setprecision(1) << winWithPercentage << "%";

            // Draw "wins with/losses with" stats
            if (player.winsWith == 0 && player.lossesWith == 0) {
                canvas.SetColor(255, 255, 255, 255); // White
            }
            else if (player.winsWith > player.lossesWith) {
                canvas.SetColor(56, 142, 235, 255); // Blue
            }
            else {
                canvas.SetColor(255, 165, 0, 255); // Orange
            }
            canvas.SetPosition(Vector2(xOffset + 10, yOffset));
            canvas.DrawString(ssWinWithPercentage.str(), statFontSize, statFontScale);
        }
        else {
            float totalAgainstGames = static_cast<float>(player.winsAgainst + player.lossesAgainst);
            float winAgainstPercentage = totalAgainstGames == 0 ? 0 : (player.winsAgainst / totalAgainstGames) * 100.0f;
            std::stringstream ssWinAgainstPercentage;
            ssWinAgainstPercentage << "Won against" << ": " << std::fixed << std::setprecision(1) << winAgainstPercentage << "%";

            // Draw "wins against/losses against" stats
            if (player.winsAgainst == 0 && player.lossesAgainst == 0) {
                canvas.SetColor(255, 255, 255, 255); // White
            }
            else if (player.winsAgainst > player.lossesAgainst) {
                canvas.SetColor(0, 255, 0, 255); // Green
            }
            else {
                canvas.SetColor(255, 0, 0, 255); // Red
            }
            canvas.SetPosition(Vector2(xOffset + 10, yOffset));
            canvas.DrawString(ssWinAgainstPercentage.str(), statFontSize, statFontScale);
        }
    }
    yOffset += lineHeight + padding;
}

void RocketLeagueRivals::RenderTeam(CanvasWrapper& canvas, 
    const std::string& header, 
    const std::vector<PlayerInfo>& players, 
    int xOffset, 
    int& yOffset, 
    int width, 
    int headerHeight, 
    int playerHeight, 
    int lineHeight, 
    int padding, 
    float headerFontSize, 
    float headerFontScale, 
    float playerFontSize, 
    float playerFontScale, 
    float statFontSize, 
    float statFontScale, 
    bool isMyTeam) {
    if (!players.empty()) {
        DrawHeader(canvas, header, xOffset, yOffset, width, headerHeight, padding, headerFontSize, headerFontScale);
        for (const auto& player : players) {
            DrawPlayerInfo(canvas, player, xOffset, yOffset, width, playerHeight, lineHeight, padding, playerFontSize, playerFontScale, statFontSize, statFontScale, isMyTeam);
        }
        yOffset += headerHeight; // Extra space between teams
    }
}