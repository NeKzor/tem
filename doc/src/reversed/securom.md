## SecuROM

### Context

[Wikipedia Article][].

- CD/DVD copy protection
- Digital rights management (DRM)
- Part of [Sony DADC][]
- Lots of controversies including Tron: Evolution :(

TL;DR: SecuROM was supposed to be a copy-protection for software. However the idea and the implementation was flawed as it made it harder or even impossible (!!) to use the protected software, resulting in users to download a protection-free version of their game through torrents.

[Wikipedia Article]: https://en.wikipedia.org/wiki/SecuROM
[Sony DADC]: https://en.wikipedia.org/wiki/Sony_Digital_Audio_Disc_Corporation

### Launcher

From game process:
- Checks if it was launched from the right executable
- Checks if it could map a shared file

Creates 20 files in `AppData\Local\Temp\GridGameLauncher_Data_DFE`.

Example filename: `data_dfe_b93b5ee4f6834d365af674dbed527de5`.

TODO: Does DFE have a special meaning?

### GetClassNameA

Joined together with four different parts.

Example: `GridGameLauncher.exe_7910_4E63862_FBC446`.

|Part|Description|
|---|---|
|GridGameLauncher.exe|Launcher name|
|7910||
|4E63862|GetTickCount|
|FBC446||

### Strings

Lots of useless string obfuscation for calling kernel32 functions.

Often encrypted with Rot13.

### Compression

Uses [zlib deflate][].

[zlib deflate]: https://github.com/madler/zlib/blob/master/deflate.c

### Production Servers

- `pa01.sonyvfactory.com:443/SecuROM_pa_web/activation`
- `pa02.sonyvfactory.com:443/SecuROM_pa_web/activation`
- `pa02.sonyvfactory.com:80/SecuROM_pa_web/activation`
- `pa01.sonyvfactory.com:443/SecuROM_pa_web/activation`
- `pa02.sonyvfactory.com:443/SecuROM_pa_web/activation`
- `pa03.sonyvfactory.com:443/SecuROM_pa_web/activation`

### UI

Module `paul.dll` is used for activation.

It's made with MFC and ATL.

### Activation Error Codes

NOTE: Somebody at Sony could not spell `occurred` lol.

|Code|Description|
|---|---|
|0|an internal error occured|
|2|API command not implemented|
|3|API command is invalid |
|4|out of memory|;
|5|a buffer is too small|
|6|a buffer is too large|
|7|unlock code has invalid format|
|8|unlock code has invalid CPA|
|9|unlock code is invalid|
|10|unlock code is not available|
|11|unlock code is expired|
|12|unlock code was revoked|
|13|API call is not allowed|
|14|userdata request failed|
|15|wrong version, API update needed|
|16|timestamp expired|
|17|activation failed|
|18|a SSL error occured|
|19|a connection error occured|
|20|unlock convert error|
|21|a XML error occured|
|22|error during sending HTTP request|
|23|error during receiving HTTP response|
|24|USER has canceled transaction|
|25|evaluation of the unlock code failed|
|26|verification of unlock code failed|
|27|userdata commit failed|
|28|unlock code is invalid but within grace period|
|29|unlock code is invalid and grace period ended|
|30|invalid parameter passed to api|
|31|unlock code is expired but within grace period|
|32|unlock code is blacklisted (already used)|
|33|unlock code is empty (not set)|
|34|server error - global verify error|
|35|server error - purchase error|
|36|server error - error during verification|
|37|server error - activation check error|
|38|server error - error during creating activation|
|39|server error - can not update statistics|
|40|server error - registration required|
|41|server error - license expired|
|42|server error - project (CPA) not active|
|43|server error - purchase not found|
|44|server error - too many activations within timeframe|
|45|server error - too many total activations|
|46|server error - wrong/invalid serial|
|47|server error - application not found|
|48|grace period is undefined|
|49|expiry of unlock code is undefined|
|50|logfile does not exist, log something first|
|51|unlock code is valid|
|52|system time is corrupt|
|53|server error - internal error|
|54|server error - start date not reached|
|55|server error - too many activations on same PC|
|56|server error - too many activation on different PC's|
|57|server error - unknown server error|
|58|evaluation of unlockcode failed (hw changed?)|
|59|server error - serial revoked too often|
|60|server error - serial revoked too often within timeframe|
|61|server error - license end date reached|
|62|server error - invalid geographical region|
|other|unknown error|

### XLive (GFWL)

Call to `XLiveSetSponsorToken` (`xlive_5026`) with:
- Token = product code (most likely)
- ID = 0x425607F3 | 1112934387
