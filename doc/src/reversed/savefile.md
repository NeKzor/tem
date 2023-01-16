
## Save File

Located in `%USERPROFILE%\Documents\Disney Interactive Studios\Tron Evolution\SaveData\<XUID>\Save\CHECKPT`.

XUID Folder is formatted as a 16 zero-padded hex-string of type of type `unsigned __int64`.
Example: `0xE00000FDBD7D2113`.

```admonish todo
 What is `XUID`?
```

```admonish warning
Always make a backup of the file! Apparently a game crash during saving can corrupt it lol.
```

### Structure

|File Offset|Type|Name|Description|
|---|---|---|---|
|0x00000|int32|Version|0x0001 = CRC32|
|0x00004|int32|Body Size|Size of body|
|0x00008|int32|Body CRC|CRC32 of body|
|0x003FC|int32|Header CRC|CRC value from offset 0-1020|
|0x00400|byte[]|[Body](#save-data)|Map save data, statistics, specific options|
|0x1A800|byte[360]|[Protected Data](#protected-data)|XLive encrypted data|

### Save Data Checks

The game does roughly the following:

- Read Header (size = 1024 bytes)
  - Verify header CRC from offset 0-1024
  - Verify body CRC32
- Read Body (size = from header)
- Read Encrypted Data (size = 360)
  - Decrypted with `XLiveUnprotectData` function
    - First call is used to determine size of data (always 8 bytes) and to get a handle
    - Second call is used to get the original data with the values of the first call
      - Original data (size = 8 bytes) contains CRC values
        - First 4 bytes is the CRC value of the header (header offset 0x03FC)
        - Next 4 bytes is the CRC value of the body (header offset 0x0008)
  - Final call to `XLiveCloseProtectedDataContext` is used to close the handle

### Protected Data

The game does this when saving:

- Call to `XUserGetSigninInfo` to get some information for the next call
- Call to `XLiveCreateProtectedDataContext`
  - Requires context information such as
    - How much to allocate (always 8 bytes)
    - Flags which will be set to 0x0001 if the user is signed in online
  - Provides a handle
- Two calls to `XLiveProtectData`
  - First call to determine the size of data (always 8 bytes)
  - Second call to protect the data
    - First 4 bytes is the CRC value of the header (header offset 0x03FC)
    - Next 4 bytes is the CRC value of the body (header offset 0x0008)
- Final call to `XLiveCloseProtectedDataContext` with the handle

### Save Data

|File Offset|Key|
|---|---|
|0586|Total Kills|
|059f|Total Deaths|
|05b8|Highest Fall|
|05d0|Deaths Disc|
|05e9|Deaths Melee|
|0608|Deaths Light Cycle|
|0620|Deaths Tank|
|0638|KILL STREAK|
|0663|Distance Walked|
|067f|Distance Driven|
|06ad|Longest Time Alive|
|06c4|Kills Disc|
|06eb|Kills Melee|
|0716|Kills Bomb Disc|
|0747|Kills Corruption Disc|
|0774|Kills Stasis Disc|
|07a0|Kills Heavy Disc|
|07cd|Kills Light Cycle|
|0802|Kills Tank|
|0832|Power Nodes Captured|
|085b|Total Assists|
|0875|Complete 1_01|
|088f|Complete 1_03|
|08b8|Complete 1_04|
|08e1|Complete 2_01|
|08fb|Complete 2_02|
|0924|Complete 2_03|
|093e|Complete 2_04|
|0967|Complete 2_05|
|0981|Complete 2_06|
|099b|Complete 2_07|
|09b5|Complete 2_08|
|09de|Complete 2_09|
|09f8|Complete 2_10|
|0a21|Complete 3_01|
|0a3b|Complete 3_02|
|0a55|Complete 3_03|
|0a7d|Player Level|
|0aee|MP Matches|
|0b08|Highest Combo|
|0b50|Tutorial_Parry|
|0b78|Tutorial_Dodge|
|0ba0|Tutorial_Combo|
|0bcb|Tutorial_Mobility|
|0bee|TF_SYSMON|
|0c05|TF_SCTSENT|
|0c19|TF_TRON|
|0c2f|TF_SPZUSE|
|0c49|TF_LIGHTCYCLE|
|0c5f|TF_SENTRY|
|0c76|TF_SOLSAIL|
|0c89|TF_QUO|
|0c9c|TF_ARJ|
|0cb3|TF_ARJIANS|
|0ccb|TF_LIGHTANK|
|0cdd|TF_GG|
|0cf2|TF_TCITY|
|0d07|TF_RECOG|
|0d1a|TF_CLU|
|0d31|TF_BOSTCOL|
|0d45|TF_GIBS|
|0d5e|TF_BOSTRUMIT|
|0d75|TF_HVYSENT|
|0d8b|TF_BASICS|
|0da3|TF_INFECTED|
|0db8|TF_FLYNN|
|0dce|TF_PLAYER|
|0de3|TF_RADIA|
|0df7|TF_DISC|
|0e0e|TF_OUTLAND|
|0e23|TF_ABRAX|
|0e35|TF_BG|
|0e4a|TF_REGUL|
|0e61|ABRXSHARD1|
|0e78|ABRXSHARD2|
|0e8f|ABRXSHARD3|
|0ea6|ABRXSHARD4|
|0ebd|ABRXSHARD5|
|0ed4|ABRXSHARD6|
|0ef2|LOWEST_DIFFICULTY|
|0f11|IRON_MAN_AVAILABLE|
|0f2e|CompleteTutorial|
|0f58|ItemsPurchased|
|0f81|ModsPurchased|
|0faf|EnhancersPurchased|
|0fdc|UpgradesPurchased|
|1005|LEVEL_UP_GRID|
|102f|PLAY_GAME_GRID|
|105a|WIN_DEATH_MATCH|
|108a|WIN_TEAM_DEATH_MATCH|
|10b6|WIN_POWER_MONGER|
|10e2|CompleteGameHard|
|1101|CompleteGameInsane|
|112c|CurentHitStreak|
|117d|SKIN_QUORRA|
|1191|SKIN_GIBSON|
|11a2|SKIN_CLU|
|11c0|VEHICLE_DEFAULT_CYCLE|
|11d8|SKIN_BLACKGUARD|
|11ed|SKIN_DEFAULT|
|120d|Default_LightcycleTrail|
|1223|SKIN_SAMFLYNN|
|123c|CORE_HEAVY_DISC1|
|1254|UPGRADE_ENERGY1|
|126c|UPGRADE_HEALTH1|
|1284|ENHANC_SOLOHLT1|
|129c|UPGRADE_ABSORB1|
|12b4|MOD_HEAVY_DISC2|
|12ce|MISC_MP_LOADOUT_2|
|12e6|UPGRADE_ENERGY2|
|1300|MOD_HEAVY_PASSIVE|
|1318|ENHANC_SOLOENG1|
|1330|UPGRADE_HEALTH2|
|1348|CORE_BOMB_DISC1|
|1360|ENHANC_TEAMHLT1|
|1377|ENHANC_DMGRES1|
|138f|UPGRADE_ABSORB2|
|13a7|ENHANC_TEAMENG1|
|13be|MOD_BOMB_DISC2|
|13d6|UPGRADE_HEALTH3|
|13f0|CORE_STASIS_DISC1|
|140a|MISC_MP_LOADOUT_3|
|1422|UPGRADE_ABSORB3|
|143b|MOD_STASIS_DISC2|
|1454|MOD_BOMB_PASSIVE|
|1471|CORE_CORRUPTION_DISC|
|1488|ENHANC_DMGRET1|
|14a0|UPGRADE_ENERGY3|
|14bd|MOD_CORRUPTION_DISC2|
|14db|ENHANC_LINEDMGSCALES1|
|14f3|UPGRADE_ABSORB4|
|1511|ENHANC_LINEDMGSCALET1|
|152c|MOD_STASIS_PASSIVE|
|1544|UPGRADE_HEALTH4|
|1560|VEHICLE_MOVIE_CYCLE|
|157f|MOD_CORRUPTION_PASSIVE|
|1594|ENHANC_DMGS1|
|15a9|ENHANC_DMGT1|
|15c1|ENHANC_BESERKS1|
|15d9|ENHANC_BESERKT1|
|15f1|ENHANC_SOLOHLT2|
|160d|VEHICLE_FLYNN_CYCLE|
|1625|ENHANC_SOLOENG2|
|163c|ENHANC_DMGRES2|
|1654|ENHANC_TEAMHLT2|
|166c|ENHANC_TEAMENG2|
|168a|ENHANC_LINEDMGSCALES2|
|16a8|ENHANC_LINEDMGSCALET2|
|16bf|ENHANC_DMGRET2|
|16d4|ENHANC_DMGS2|
|16e9|ENHANC_DMGT2|
|1701|ENHANC_BESERKS2|
|1719|ENHANC_BESERKT2|
|1730|ENHANC_ALLINES|
|1734|Audio Subtitles|
|1750|Volume SFX|
|1754|Volume Music|
|1758|Volume Dialogue|
|042d or 046e|Total XP|
|043d|Difficulty|
|0418|HUD|
|0419|Disc Station Grids|
|1738|Inverted Look|
|0435|Camera Sensitivity|
