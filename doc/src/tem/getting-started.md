# Getting started

## Requirements

1.\) Game is installed.

2.\) Game is activated. See section [Unlocker](#unlocker) if you need help with activation. 

## Installation

1.\) Download the latest `tem.zip` file from [GitHub releases][].

2.\) Extract the files to the same folder as `GridGame.exe`. The default location is:

```
C:\Program Files (x86)\Disney Interactive Studios\Tron Evolution\Binaries\Win32Live
```

3.\) Launch the game with the replaced `GridGameLauncher.exe`

[GitHub releases]: https://github.com/NeKzor/tem/releases

## Unlocker

TEM comes with an unlocker tool that allows anyone to generate their unlock code for product activation on their local machine. This only works if you also installed SecuROM's [offline installer package][].

<details>
<summary>Click to preview</summary>

![unlock-code-sub-10-fast-game-complete.gif](/images/unlock-code-sub-10-fast-game-complete.gif)

</details>

[offline installer package]: https://support.securom.com/pop_tron.html

## XDead (optional)

TEM works in combination with [XDead][], which replaces GFWL aka XLive. This is the quickest and easiest way to get past GFWL installation and authentication. This means that no Microsoft account is needed.

[XDead]: https://github.com/NeKzor/xdead

## Linux

Because of XDead, we can easily get the game to run under Linux with [Wine][].

- Install `dinput8` and `physx` with [Winetricks].
- Use `wine regedit` to set the required registry values in `HKEY_LOCAL_MACHINE\Software\Disney Interactive Studios\tr2npc`:
  - `InstallPath REG_SZ "C:\Program Files (x86)\Disney Interactive Studios\Tron Evolution\Binaries\Win32Live"`
  - `Language REG_SZ "EN"` (other supported languages are `DE`, `FR`, `ES`, `IT`, `JP`, `RU`, `PL`, `NL`, `CZ`)
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
