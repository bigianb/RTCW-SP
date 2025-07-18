# Return to Castle Wolfenstein single player GPL source release

This is a very simplified port of the Return to Castle Wolfenstein code.
The aim is to have something easy to understand. In particular the dynamic library
loading and Quake C interpreter / recompiler have been removed.
The primary development environment is a mac although other platforms should also work fine.

## GENERAL NOTES

### Game data and patching:

This source release does not contain any game data,
the game data is still covered by the original EULA and must be obeyed as usual.

Note that RTCW is available from the Steam store at
http://store.steampowered.com/app/9010/
and also the GOG store at https://www.gog.com/en/game/return_to_castle_wolfenstein

### Mac Installing

For mac, I use the GOG version.
Run the downloaded offline installer using
[wine](https://gitlab.winehq.org/wine/wine/-/wikis/Download).
I assume the steam version will work too but I have not tried.

By defult this will install to `~/.wine/drive_c/GOG Games/Return to Castle Wolfenstein`

You can use shift+command+period to toggle showing hidden files in finder (.wine is a hidden directory)


