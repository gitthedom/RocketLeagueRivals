# Rocket League Rivals

Rocket League Rivals is a plugin for [BakkesMod](https://bakkesmod.com/), designed to track and display player interactions and statistics during Rocket League matches. This plugin helps you analyze your performance against teammates and rivals by providing detailed stats.

## Features

- **Player Stats:** Track wins, losses, demos given and received for each player you encounter.
- **Team Stats:** Analyze how well you perform with and against specific players.
- **Rivalry Score:** Calculate a rivalry score based on various factors to identify your key rivals.
- **Customizable Display:** Configure which stats to show and how they are displayed on the screen.

## Installation

1. Download and install [BakkesMod](https://bakkesmod.com/).
2. Copy the plugin files into the BakkesMod plugins folder: `BakkesMod\plugins`
3. Start BakkesMod and Rocket League.
4. Load the plugin by pressing F2 -> Plugins -> Open PluginManager and check the plugin box for RocketLeagueRivals


## Usage

The Rocket League Rivals plugin displays various stats about the players in your matches. These stats are shown in a customizable on-screen overlay.

### Displayed Stats

- **Player Name:** The name of the player.
- **Wins With:** Percentage of matches won with this player on your team.
- **Wins Against:** Percentage of matches won against this player.
- **Demos Given / Received:** Number of times you demolished this player and that this player has demolished you.
- **Rivalry Score:** A calculated score indicating the intensity of your rivalry with this player.

### Customization Options

- **Align Display Right:** Align the stats display to the right side of the screen.
- **Enable Logging:** Enable or disable logging of player stats to a file.
- **Hide My Team:** Hide stats for players on your team.
- **Hide Rival Team:** Hide stats for players on the opposing team.
- **Show All Wins/Losses Stats:** Show detailed win/loss stats for all players.
- **Show Team Stats:** Show team-based stats.
- **Show Demo Stats:** Show demolition stats.
- **Show Rival Stats:** Show rivalry scores for players.

### Commands

You change settings in the plugins window or you can use the following console commands to customize the plugin settings:

- `align_display_right <0/1>`: Enable or disable right-side display alignment.
- `folder_logging_enabled <0/1>`: Enable or disable logging.
- `hide_my_team_enabled <0/1>`: Enable or disable hiding your team's stats.
- `hide_rival_team_enabled <0/1>`: Enable or disable hiding the rival team's stats.
- `show_all_winsloses_stats_enabled <0/1>`: Enable or disable showing all win/loss stats.
- `show_team_stats_enabled <0/1>`: Enable or disable showing team stats.
- `show_demo_stats_enabled <0/1>`: Enable or disable showing demo stats.
- `show_rival_stats_enabled <0/1>`: Enable or disable showing rival stats.
- `set_rival_number <number>`: Set the threshold for rivalry scores.

## Development

If you wish to contribute or modify the plugin, here are the instructions to set up the development environment:

1. Clone the repository:
```sh
git clone https://github.com/gitthedom/rocketleaguerivals.git
```
2. Open the project in your preferred IDE.
3. Make sure to link the BakkesMod SDK to your project.

## Contributing
Feel free to open issues or submit pull requests. We welcome contributions to improve this plugin.

## License
This project is licensed under the MIT License. See the LICENSE file for details.

Happy playing!
