# Launcher

This replaces the SecuROM launcher after product activation.

The game developers implemented two flaws which allows us to remove the original launcher without patching any of the
original files. This means **we can safely change any of our hardware components** and we save 7 MB on our disk for more
useful things, heck yeah!! Who wants to keep Sony DADC's garbage anyway?

## Flaw #1 - Launcher Filename

The game checks if the name of the parent process matches with `GridGameLauncher.exe`.
However, this only checks the filename and not the full pathname.
This means we can start the launcher from anywhere we want.

## Flaw #2 - CRC Bypass

The game calculates the CRC value of the `GridGameLauncher.exe` file and matches it against the expected value,
depending which language is used (`RU`, `CZ/PL` or international). This calculation can be bypassed by creating an empty
`patch.dat` file in the same directory of the game. Maybe the game developers used this internally and
forgot to remove it like they forgot to remove the developer console? Or maybe this was supposed to be a DLC update
hack but they could not update the launcher? Regardless, I will call this a backdoor :^\)

![crc-backdoor.gif](/doc/src/images/crc-backdoor.gif)

## Caveats

The game still requires the two registry entries `InstallPath` and `Language` to be set in
`HKEY_LOCAL_MACHINE\Software\Disney Interactive Studios\tr2npc`, which are set automatically after installation.
The new launcher checks for `InstallPath`, otherwise the game would fail to read `patch.dat`, `GridGameLauncher.exe`
and other settings.

Optional: Verify correct installation in PowerShell:

```ps1
reg query "HKLM\Software\Disney Interactive Studios\tr2npc" /reg:32
```

## Wine

For running the game on Linux you'll need to install [Wine][].

- Install `dinput8` and `physx` with [Winetricks].
- Use `wine regedit` to set the required registry values, see section [Caveats](#caveats) above.
- By default Wine loads its own `dinput8.dll` version. Simply use `WINEDLLOVERRIDES="dinput8=n"` to use TEM's proxy.
- Make sure to use 32-bit version of Wine with `WINEARCH=win32` (only required for the first time).

```bash
# Install requirements
winetricks -q physx dinput8

# Manually add registry values
wine regedit

# Launch
WINEDLLOVERRIDES="dinput8=n" WINEARCH=win32 wine GridGameLauncher.exe
```

[Wine]: https://www.winehq.org
[Winetricks]: https://github.com/Winetricks/winetricks

## Steam + Proton

Yea, no.
