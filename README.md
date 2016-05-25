MeryLauncher
==============
A modification of [MinimaLauncher by iGlitch](https://github.com/iGlitch/MinimaLauncher) to act as an ad-hoc launcher for individual mods on an SDHC card. Put the full path to gameconfig.txt and *.gct file in arguments 1 and 2 to the homebrew app to get a direct launcher for your mod. (Note: Ignores the disc ID in gameconfig.txt.) Currently hardcoded to use only OSSleepThread as hook type.

```<app version="1.2">
        <name>Brawl-</name>
        <!-- ... -->
        <arguments>
        <arg>sd:/apps/brawl-/gameconfig.txt</arg>
        <arg>sd:/apps/brawl-/RSBE01.gct</arg>
        </arguments>
    </app>
```

If you thought MinimaLauncher had too much bothersome UI, this is the launcher for you!

MinimaLauncher Description
--------------
A disc booter with no UI for Wii/vWii.
Supports Ocarina Codes.
It can boot Wii and Gamecube games from the Disc slot. (GC not for WiiU)
This app serves as an alternative to GeckoOS.
