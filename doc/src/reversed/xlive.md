
## XLive (GFWL)

- [Overview](#overview)
- [Call Order](#call-order)
- [Threads](#threads)
- [Signature Check](#signature-check)
- [Modification Check](#modification-check)
- [Hook Protection](#hook-protection)
- [Imported Functions (IAT)](#imported-functions-iat)
- [Files](#files)
- [Authentication](#authentication)
- [Client](#client)
- [Catalog](#catalog)
  - [FindGameOffers](#findgameoffers)
  - [FindGames](#findgames)

### Overview

```admonish warning
Some observations are hard to interpret as XLive <i>looooves to crash a lot</i> when attempting to tamper it.
For example: XLive itself has to be modified in order to make it debuggable which might yield incorrect results.
```

XLive uses several anti-tampering tricks:

|Trick|Bypass|
|---|---|
|[Signature Check](#signature-check)||
|[Modification Check](#modification-check)|Byte patch (the irony lol)|
|[IAT Obfuscation](#imported-functions-iat)|Hook by ordinal|
|[Hook Protection](#hook-protection)|Substitution|
|Hardware/Software-Breakpoint Checks|[Thread Suspension](#threads)|
|Function Obfuscation and Encryption||

### Call Order

#### At launch

- XNetGetTitleXnAddr
- XLivePreTranslateMessage
- XUserGetSigninState
- XLiveInitializeEx
  - On failure: engine warns of debugger usage and shuts down.
- XNetStartup
  - If `XLiveInitializeEx` succeeded.
- XOnlineStartup
  - If `XNetStartup` succeeded.
- XSocketWSAGetLastError
  - If `XNetStartup` errored.
- XHVCreateEngine
- XUserGetSigninInfo
- XUserGetSigninStat
- XNotifyCreateListener
- XNotifyPositionUI
- XPresenceInitialize
- XLiveSetDebugLevel
  - If `XNotifyCreateListener` succeeded.
- XSocketNTOHS
  - If `XNotifyCreateListener` succeeded.
- XNetSetSystemLinkPort
  - If `XNotifyCreateListener` succeeded.
- XLiveInput
  - Called frequently. Game stalls if call errored.
- XLiveRender
  - Called frequently.
- XUserGetName
- XFriendsCreateEnumerator
- XUserCreateAchievementEnumerator
- XUserSetContext
- XNotifyGetNext
  - Called frequently.

#### When pressing start

- XShowSigninUI

#### When pressing Live

- XShowGuideUI

#### When starting a game

- XUserGetName
- XUserGetName
- XFriendsCreateEnumerator
- XUserSetContext

#### When triggering an achievement

- XUserWriteAchievements

#### Game exit

- XLiveUninitialize
- XOnlineCleanup
- XLiveOnDestroyDevice
- XLiveUninitialize
- XNetCleanup

### Threads

The game seems to be in a playable state even when XLive's "Main" thread gets suspended.
Obviously this will make it freeze if you want to interact with XLive
but at least this will bypass the anti-debugging checks.

|Index|Start Address|Description|
|---|---|---|
|0|0x146ab8|UI|
|1|0x146ab8|Main|
|2|0x146ab8|Anti-Debugger|
|3|0x146ab8|Anti-Debugger|
|4|0x146ab8|Anti-Debugger|
|5|0x146ab8|Anti-Debugger|
|6|0x146ab8|Anti-Debugger|
|7|0x146ab8|Anti-Debugger|
|8|0x4f1077|Connection|
|9|0xda3f3|Notifications|
|10|0xda3f3|Notifications|

It's probably possible to patch a few places to get rid of the debugger checks but I suspect that it won't be that easy.
Let's say XLive works like this and we write a naive patch:

```c++
void xlive()
{
  // Try jumping after all the checks
  Debugger_check();

  // This will be a problem if this sets important values
  Operation_with_side_effects();

  Another_debugger_check();
}
```

There is another problem which I suspect is how XLive actually works, since there are so many threads of "Main":

```c++
void xlive()
{
  // Try jumping after all the checks
  Debugger_check();

  // Wake up other thread
  Signal_next_thread_and_wait();

  // Other thread will continue somewhere with its own debugger checks
  Another_debugger_check();
}
```

This is too painful to try and verify. All of this is a lot more complicated with 8 "Main" threads and a ton of
different debugger checks. It's actually hilarious if you see the amount of debugger confusion for yourself.

### Signature Check

XLive will not initialize properly without first validating `GridGame.exe` with `GridGame.exe.cat` for modifications.
The `.cat` file is a security catalogue file which is signed by Microsoft Windows Games for Live:

|Detail|Description|
|---|---|
|Signer|Microsoft LIVE PCA|
|Signing Time|Saturday, 30 October 2010 01:55:25|

```admonish todo
Isn't it a bit too late to check if the file has been modified if it already launched?
```

```admonish todo
Figure out if Microsoft messed up and we can simply bypass this too.
```

### Modification Check

From current observation the game seems to always check for modifications when loading the [save file][] by calling
`XLiveUnprotectData`. There is a simple `two-byte-patch` method that was originally found by several people in 2009
and before which eliminates this check.

[save file]: ./reversed/savefile.md

### Hook Protection

Some functions are protected by obfuscation and anti-tampering tricks!
XLive checks if the correct return address from the caller's module (the base module)
is on the stack. A traditional function hook by calling the original function would not work.
However XLive does not check if the call is a substitution :^).

```cpp
__declspec(naked) void hook()
{
  // save registers
  // do whatever...
  // restore registers

  // return address should be on the correct stack location 
  __asm jmp to_original_function;
}
```

The reason why hooking through a trampoline function does not work is because of a direct call to a decrypt function.
This function will use the return address to decrypt the rest of the function body.
What's interesting is that they modify the stack using [Return Oriented Programming][] (ROP) in order to jump to
the next decrypt function or to another ROP gadget.
In several other function locations some new code will be written and executed.
Although not all calls to sub-functions are encrypted.

[Return Oriented Programming]: https://en.wikipedia.org/wiki/Return-oriented_programming

```cpp
__declspec(naked) void xlive_5034()
{
  // Stack:
  //     0x20ff0000 -> return address of caller
  //                   higher bits for the module base might be checked

  __asm {
      call decrypt_xlive_5034;
  };

  // Stack:
  //     0x20ff0004 -> "return address" for decryption function
  //                   which will be used to execute the body

  __asm {
      encoded_metadata_bytes;
      // Rest of the encrypted body
  };
}

```

The values below the calling function are metadata which are used to determine the size and the address of the body.
Thread synchronization primitives (omitted in the code-snippet below) are being used at the beginning
which seem to be the main reason why stepping through this function with a debugger would cause a crash.
The function mostly consists of operations which decrypt the function body.
There are several reads to global variables which hold additional information.

```cpp
// Partially reversed decrypt function
void decrypt_xlive_5034()
{
  auto ra = uintptr_t(_ReturnAddress());
  auto encoded = *(int32_t*)ra;

  auto body_size = (encoded >> 4) + 0x4fff - (((encoded >> 4) + 0x4fff) % 0x5000);
  auto address_of_body = ra + (encoded & 3) + 4;

  auto offset = encoded >> 4;
  auto offset_into_body = address_of_body + offset;
  auto relative_address = address_of_body - module_base;

  auto var1 = *(int32_t*)offset_into_body;
  auto var2 = *(int32_t*)(offset_into_body + 4);

  auto length = var2 & 0xffffffff;
  auto address = module_base + (var1 & 0xffffffff);

  // ...
}
```

In very special cases the return address of the caller (the process module) will be used.
This will fail if we simply call an XLive function from any other module.

```admonish todo
Figure out if it actually is the main module.
```

One odd thing seems to happen with `XLiveCloseProtectedDataContext` though which might be a bug in XLive.
This function will crash the process if a call to `XLiveUnprotectData` fails to read parts of a corrupted save file.
However the call to `XLiveCloseProtectedDataContext` will succeed if it gets called from a different module.

### Imported Functions (IAT)

```admonish warning
XLive has more exported functions than the game has imported.
```

|Ordinal|Name|Notes|
|---|---|---|
|3|XSocketCreate||
|4|XSocketClose||
|6|XSocketIOCTLSocket||
|7|XSocketSetSockOpt||
|8|XSocketGetSockOpt||
|9|XSocketGetSockName||
|11|XSocketBind||
|12|XSocketConnect||
|13|XSocketListen||
|14|XSocketAccept||
|15|XSocketSelect||
|18|XSocketRecv|called frequently|
|20|XSocketRecvFrom||
|22|XSocketSend|called frequently|
|24|XSocketSendTo||
|27|XSocketWSAGetLastError||
|38|XSocketNTOHS||
|51|XNetStartup||
|52|XNetCleanup||
|53|XNetRandom||
|55|XNetRegisterKey||
|56|XNetUnregisterKey||
|57|XNetXnAddrToInAddr||
|58|XNetServerToInAddr||
|63|XNetUnregisterInAddr||
|67|XNetDnsLookup||
|68|XNetDnsRelease||
|69|XNetQosListen||
|70|XNetQosLookup||
|71|XNetQosServiceLookup||
|72|XNetQosRelease||
|73|XNetGetTitleXnAddr||
|75|XNetGetEthernetLinkStatus||
|77|XNetQosGetListenStats||
|84|XNetSetSystemLinkPort||
|651|XNotifyGetNext|called frequently|
|652|XNotifyPositionUI||
|1082|XGetOverlappedExtendedError||
|1083|XGetOverlappedResult||
|5001|XLiveInput|called frequently|
|5002|XLiveRender|called frequently|
|5003|XLiveUninitialize||
|5006|XLiveOnDestroyDevice||
|5007|XLiveOnResetDevice||
|5008|XHVCreateEngine||
|5016|XLivePBufferAllocate||
|5017|XLivePBufferFree||
|5030|XLivePreTranslateMessage|called frequently|
|5031|XLiveSetDebugLevel||
|5034|XLiveProtectData||
|5035|XLiveUnprotectData||
|5036|XLiveCreateProtectedDataContext||
|5038|XLiveCloseProtectedDataContext||
|5206|XShowMessagesUI||
|5208|XShowGameInviteUI||
|5209|XShowMessageComposeUI||
|5210|XShowFriendRequestUI||
|5212|XShowCustomPlayerListUI|called frequently|
|5214|XShowPlayerReviewUI||
|5215|XShowGuideUI||
|5216|XShowKeyboardUI||
|5250|XShowAchievementsUI||
|5251|XCloseHandle||
|5252|XShowGamerCardUI||
|5256|XEnumerate||
|5258|XLiveSignout||
|5259|XLiveSignin||
|5260|XShowSigninUI||
|5262|XUserGetSigninState|called frequently|
|5263|XUserGetName||
|5264|XUserAreUsersFriends||
|5265|XUserCheckPrivilege||
|5267|XUserGetSigninInfo||
|5270|XNotifyCreateListener||
|5271|XShowPlayersUI||
|5274|XUserAwardGamerPicture||
|5275|XShowFriendsUI||
|5277|XUserSetContext||
|5278|XUserWriteAchievements||
|5279|XUserReadAchievementPicture||
|5280|XUserCreateAchievementEnumerator||
|5281|XUserReadStats||
|5284|XUserCreateStatsEnumeratorByRank||
|5286|XUserCreateStatsEnumeratorByXuid||
|5292|XUserSetContextEx||
|5293|XUserSetPropertyEx||
|5294|XLivePBufferGetByteArray||
|5297|XLiveInitializeEx||
|5300|XSessionCreate||
|5303|XStringVerify||
|5305|XStorageUploadFromMemory||
|5306|XStorageEnumerate||
|5310|XOnlineStartup||
|5311|XOnlineCleanup||
|5318|XSessionStart||
|5312|XFriendsCreateEnumerator||
|5313|XPresenceInitialize||
|5314|XUserMuteListQuery||
|5315|XInviteGetAcceptedInfo||
|5316|XInviteSend||
|5317|XSessionWriteStats||
|5320|XSessionSearchByID||
|5321|XSessionSearch||
|5322|XSessionModify||
|5324|XOnlineGetNatType||
|5326|XSessionJoinRemote||
|5327|XSessionJoinLocal||
|5328|XSessionGetDetails||
|5329|XSessionFlushStats||
|5330|XSessionDelete||
|5331|XUserReadProfileSettings||
|5332|XSessionEnd||
|5333|XSessionArbitrationRegister||
|5335|XTitleServerCreateEnumerator||
|5336|XSessionLeaveRemote||
|5337|XUserWriteProfileSettings||
|5338|XPresenceSubscribe||
|5340|XPresenceCreateEnumerator||
|5342|XSessionModifySkill||
|5343|XSessionCalculateSkill||
|5344|XStorageBuildServerPath||
|5345|XStorageDownloadToMemory||
|5350|XLiveContentCreateAccessHandle||
|5355|XLiveContentGetPath||
|5356|XLiveContentGetDisplayName||
|5360|XLiveContentCreateEnumerator||
|5365|XShowMarketplaceUI||
|5367|XContentGetMarketplaceCounts||
|5372|XMarketplaceCreateOfferEnumerator||

### Files

Located in `AppData\Local\Microsoft\Xlive`.

Content:
  - \<XUID\>\\FFFE07D1\\00010000
      - \<XUID\>_MountPt
        - \<TITLE_ID>\.gpd
        - Account
        - FFFE07D1.gpd
        - tile_32.png
        - tile_64.png
      - \<XUID\>

Titles:
- \<TITLE_ID\> (425607F3 for Tron: Evolution)
  - config.bin
  - Token.bin

```admonish todo
What is `FFFE07D1` and `00010000`?
```

```admonish todo
Figure out `FBDX` format.
```

```admonish todo
Figure out 0x20 + `NOC` format.
```

### Authentication

Using [Kerberos Protocol][].

Kerberos: `40.64.89.190`

Xbox: `tgs.prod.xboxlive.com`, `65.55.42.217`

[Kerberos Protocol]: https://en.wikipedia.org/wiki/Kerberos_(protocol)

### Client

Command: `GFWLClient.exe /NoAutoSignIn /NoInterface`

### Catalog

Tron: Evolution Media ID: `66acd000-77fe-1000-9115-d804425607f3`

Host: `catalog.xboxlive.com`

API: GET `/Catalog/Catalog.asmx/Query?methodName=`

#### FindGameOffers

|Names|Values|
|---|---|
|Locale|en-US|
|LegalLocale|en-US|
|Store|3|
|PageSize|10|
|PageNum|1|
|DetailView|3|
|OfferFilterLevel|1|
|MediaIds|66acd000-77fe-1000-9115-d804425607f3|
|UserTypes|3|
|MediaTypes|1|
|MediaTypes|5|
|MediaTypes|18|
|MediaTypes|19|
|MediaTypes|20|
|MediaTypes|21|
|MediaTypes|22|
|MediaTypes|23|
|MediaTypes|30|
|MediaTypes|34|
|MediaTypes|37|

#### FindGames

|Names|Values|
|---|---|
|Locale|en-US|
|LegalLocale|en-US|
|Store|3|
|PageSize|10|
|PageNum|1|
|DetailView|5|
|Relations|2|
|UserTypes|2|
|UserTypes|3|
|MediaIds|66acd000-77fe-1000-9115-d804425607f3|
|MediaTypes|1|
|MediaTypes|5|
|MediaTypes|18|
|MediaTypes|19|
|MediaTypes|20|
|MediaTypes|21|
|MediaTypes|22|
|MediaTypes|23|
|MediaTypes|30|
|MediaTypes|34|
|MediaTypes|37|
|ImageFormats|5|
|ImageSizes|15|
