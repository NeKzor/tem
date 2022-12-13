
## XLive (GFWL)

XLive uses several anti-reversing tricks:

|Trick|Bypass|
|---|---|
|[Modification Check](#modification-check)|Byte patch (the irony lol)|
|[IAT Obfuscation](#imported-functions-iat)|Hook by ordinal|
|[Hook Protection](#hook-protection)|Substitution|
|Hardware/Software-Breakpoint Checks|[Thread Suspension](#threads)|
|Function Obfuscation and Encryption||

### Threads

The game seems like in a playable state even when XLive's "Main" thread gets suspended.
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
|8|0x4f1077||
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

This is too painful to try and verify. All of this is a lot more complicated with 8 "Main" threads and a ton of different debugger checks.
It's actually hilarious if you see the amount of debugger confusion for yourself.

### Modification Check

From current observation the game seems to always check for modifications when loading the [save file][] by calling `XLiveUnprotectData`. There is a simple `two-byte-patch` method which was originally found by several people in 2009 and before.

[save file]: ./reversed/savefile.md

### Hook Protection

Some functions are protected by obfuscation and anti-hooking tricks!
XLive checks if the correct return address from the importer module (the game executable)
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

### Imported Functions (IAT)

NOTE: XLive has more exported functions than the game has imported.

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
|5212|XShowCustomPlayerListUI|called frequently|
|5215|XShowGuideUI||
|5216|XShowKeyboardUI||
|5251|XCloseHandle||
|5252|XShowGamerCardUI||
|5256|XEnumerate||
|5258|XLiveSignout||
|5262|XUserGetSigninState|called frequently|
|5263|XUserGetName||
|5265|XUserCheckPrivilege||
|5267|XUserGetSigninInfo||
|5270|XNotifyCreateListener||
|5271|XShowPlayersUI||
|5274|XUserAwardGamerPicture||
|5275|XShowFriendsUI||
|5277|XUserSetContext||
|5279|XUserReadAchievementPicture||
|5280|XUserCreateAchievementEnumerator||
|5281|XUserReadStats||
|5284|XUserCreateStatsEnumeratorByRank||
|5286|XUserCreateStatsEnumeratorByXuid||
|5292|XUserSetContextEx||
|5293|XUserSetPropertyEx||
|5294|XLivePBufferGetByteArray||
|5303|XStringVerify||
|5305|XStorageUploadFromMemory||
|5306|XStorageEnumerate||
|5310|XOnlineStartup||
|5311|XOnlineCleanup||
|5312|XFriendsCreateEnumerator||
|5313|XPresenceInitialize||
|5314|XUserMuteListQuery||
|5315|XInviteGetAcceptedInfo||
|5316|XInviteSend||
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
