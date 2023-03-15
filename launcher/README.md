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
