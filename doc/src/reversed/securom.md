## SecuROM

- [Context](#context)
- [Launcher](#launcher)
- [Data File Encryption](#data-file-encryption-dfe)
- [Spot Check](#spot-check)
- [Strings](#strings)
- [Compression](#compression)
- [Production Servers](#production-servers)
- [UI](#ui)
- [Activation Error Codes](#activation-error-codes)
- [XLive (GFWL)](#xlive-gfwl)
- [Hardware ID (HWID)](#hardware-id-hwid)
  - [Layout](#layout)
  - [OS Version](#os-operation)
  - [CPU Info](#cpu-info)
  - [GPU Info](#gpu-info)
  - [Network Info](#network-info)
  - [Volume Info](#volume-info)
  - [XOR Operation](#xor-operation)
- [DES Encryption](#des-encryption)
- [Unlock Code](#unlock-code)

### Context

[Wikipedia Article][].

- CD/DVD copy protection
- Digital rights management (DRM)
- Part of [Sony DADC][]
- Lots of controversies including Tron: Evolution :(

TL;DR: SecuROM was supposed to be a copy-protection for software. However the idea and the implementation was flawed
as it made it harder or even impossible (!!) to use the protected software, resulting in users to download a
protection-free version of their game through torrents.

[Wikipedia Article]: https://en.wikipedia.org/wiki/SecuROM
[Sony DADC]: https://en.wikipedia.org/wiki/Sony_Digital_Audio_Disc_Corporation

### Launcher

Before `GridGameLauncher.exe` starts `GridGame.exe` in a sub-process it will check for the game's activation.
Since this check depends on your [hardware id](#hardware-id-hwid) a single change in your hardware components will
affect your activation.
As it turns out you can plug in any external storage device such as an HDD and
<b>the game will not be able to launch after activation!!</b>
This has to be one of the most brain-dead ideas in history of software engineering.
Thanks again for nothing Sony DADC!

When the game process starts it does the following:

- Copies 1732 bytes from the memory mapped file `-=[SMS_GridGame.exe_SMS]=-` into a buffer
  - Bytes at offset `4-7` represent the launcher handle which is used to trigger "spot checks" through `SendMessageW`
- Checks if the game was launched from `GridGameLauncher.exe`
- Does a CRC check of `GridGameLauncher.exe`
  - Access registry path `HKEY_LOCAL_MACHINE\\Software\\Disney Interactive Studios\\tr2npc`
	- Gets registry key `InstallPath`
	- Gets registry key `Language`
  - Checks if the file `EN/patch.dat` does not exist
    - Skips the rest if it does exist lol
  - Opens `GridGameLauncher.exe`
    - Reads 0x6D1558 bytes (the whole file) into a buffer
	- XORs 4 bytes at a time with `0xAE19EDA3` from `(0x6D1558 - 40) / 4` to `0x6D1558`
	- Checks if hash matches with `0x6A85B570` for `EN` language
		- `RU` = `0xD91791D4`
		- `CZ` and `PL` = `0xBCD55594`
	- Triggers a "spot check" if the hash does not match

This basically means we cannot simply modify the game binary as it does a CRC self-check unless you create a
`patch.dat` file which seems to be a backdoor implemented by the Disney devs. However even if we want to modify the
file statically we will not be able to progress without GFWL's signature check. The game is not meant to be playable
without GFWL since the engine would just shutdown and crash because of a null pointer dereference somewhere deep
inside the online subsystem code which requires GFWL to be initialized.

The launcher can simply be replaced by modifying multiple code locations to get it working without GFWL
or by simply writing your own one:

```cpp
// NOTE: Make sure that the launcher process has the same name
//       as the original one "GridGameLauncher.exe"

#define TR2NPC_PATH L"Software\\Disney Interactive Studios\\tr2npc"
#define GAME_EXE L"GridGame.exe"
#define BUFFER_SIZE 1723

auto main() -> int
{
    println("Creating file mapping...");

	// I wonder what SMS means. SecuROM's Mapping Signature?
    auto file = std::format(L"-=[SMS_{}_SMS]=-", GAME_EXE);

    auto handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, BUFFER_SIZE, file.c_str());
    if (!handle) {
        println("Could not create file mapping object {}", GetLastError());
        return 1;
    }

    auto buffer = MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, BUFFER_SIZE);
    if (!buffer) {
        println("Could not map view of file {}", GetLastError());
        CloseHandle(handle);
        return 1;
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    WCHAR install_path[MAX_PATH] = {};
    auto install_path_size = DWORD(sizeof(install_path));

    auto result = RegGetValueW(
        HKEY_LOCAL_MACHINE,
        TR2NPC_PATH,
        L"InstallPath",
        RRF_RT_ANY,
        nullptr,
        &install_path,
        &install_path_size);

    if (result != ERROR_SUCCESS) {
        println("Failed to get registry key value for install path");
        return 1;
    }

    auto cmd = std::format(L"{}\\{}", install_path, GAME_EXE);
    CreateProcessW(NULL, (LPWSTR)cmd.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

    println("launched");

    WaitForSingleObject(pi.hProcess, INFINITE);

    println("exited");

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    UnmapViewOfFile((LPCVOID)buffer);
    CloseHandle(handle);

    return 0;
}
```

```admonish todo
Figure out how messages are handled.
```

Layout of copied buffer at launch:

|Offset|Length in bytes|Description|
|---|---|---|
|0x0004|4|Launcher handle for [spot checks](#spot-check).|
|0x0044|2|Spot check message buffer.|

### Data File Encryption (DFE)

The launcher creates 20 encrypted files in `AppData\Local\Temp\GridGameLauncher_Data_DFE`.

Example filename: `data_dfe_b93b5ee4f6834d365af674dbed527de5`.

One file is located at install path called `dfe`.

```admonish todo
There are several other files in the install path folder which might also be related to this.
```

### Spot Check

Used to trigger checks by communicating with the SecuROM launcher by sending signals through `SendMessageW`.
Each check does some calculations which might call some kernel32 functions.
This only seems to be done once for each type if the check succeeded.
A global map keeps track of the type check by key.
The game might behave differently if one check fails or gets skipped
because the results will be checked in different functions.

```cpp
// This is a simplified version.
// All names are obviously made up.
// Also worth to notice is that the checks are implemented by the game devs.
// SecuROM most likely only provides the obfuscated part which verifies the end result.

enum class SpotChecks {
	OnlineSystem = 0,
	Bindings = 1,
	NoXp = 2,
	// ...
}

enum class SpotCheck {
	Ok = 0,
	Invalid = 1 ,
}

struct SpotCheckResult {
	SpotCheck check;
}

static TMap<SpotChecks, SpotCheckResult> global_map;

auto result = SpotCheckResult();
auto index = global_map.find_by_key(SpotChecks::NoXp, &result);

if (index == INVALID_INDEX) {
	// Trigger a check.
	// Might also be called multiple times,
	// passing down the result of the former call.
	SendMessageW(securom_hwnd, securom_buffer, 2u, 0);

	// Do some calculations.
	// Calls to kernel32 functions are actually obfuscated.
	auto data = GetClassNameA();
	auto tick_count_result = data.split('_').at(3);

	// Verify result.
	auto new_result = SpotCheckResult();

	if (is_valid_tick_count(tick_count_result)) {
		new_result.check = SpotCheck::Ok;
	} else {
		new_result.check = SpotCheck::Invalid;
	}

	global_map.insert(SpotChecks::NoXp, new_result);
}

// Somewhere else...

auto result = SpotCheckResult();
auto index = global_map.find_by_key(SpotChecks::NoXp, &result);

if (index != INVALID_INDEX && result.check == SpotCheck::Invalid) {
	// Execute code to troll the player. Thanks :>
}
```

|Key|Spot Location|Troll Code|
|---|---|---|
|0|Online system function|Weird version change from `1.01` to `1.01.1`|
|1|Deadzone binding function|Buffers and delays all inputs|
|2|`UPgOnline::Init` / checked in lobby|Removes weapon from player|
|3|Triggered when spawning|???|
|4|Save load manager|Messes with the save file by zero-ing the save data buffer which causes a crash when loading the save<sup>1</sup>|
|5|Validates save game manager<br>Checks result in  `UPgOnlineGameManager::SetNextMap`|Cannot go past 3rd map|
|6|Pause Menu<br>Verifies game signature hash|Disables unpause when the game pauses|
|7|???|XP counter will not go up|
|8|Main Menu|???|
|9|Result of manual check if CRC check fails at launch|Game will not launch|

<sup>1</sup> The crash is super weird. Not sure if intended. TODO: Might want to investigate.

Kernel32 function `GetClassNameA` has been observed multiple times in spot checks.
It provides a string with four different parts.

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

```admonish bug
Somebody at Sony DADC could not spell `occurred` lol.
```

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

### Hardware ID (HWID)

Consists of five parts which will be hashed in each step with MD5 and [XOR operations](#xor-operation).

|Part|API|
|---|---|
|[OS Version][]|[GetVersionEx][]|
|[CPU Info][]|[GetVersionEx][]|
|[GPU Info][]|[Direct3DCreate9][], [GetAdapterIdentifier][]|
|[Network Info][]|[GetAdaptersInfo][]|
|[Volume Info][]|[GetDriveTypeA][], [GetVolumeInformationA][]|

[OS Version]: #os-version
[GetVersionEx]: https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getversionexa
[CPU Info]: #cpu-info
[GetVersionEx]: https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getversionexa
[GPU Info]: #gpu-info
[Direct3DCreate9]: https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-direct3dcreate9
[GetAdapterIdentifier]: https://learn.microsoft.com/en-us/windows/win32/api/d3d9/nf-d3d9-idirect3d9-getadapteridentifier
[Network Info]: #network-info
[GetAdaptersInfo]: https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-getadaptersinfo
[Volume Info]: #volume-info
[GetDriveTypeA]: https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getdrivetypea
[GetVolumeInformationA]: https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getvolumeinformationa

### Layout

```cpp
struct hwid_t {
	byte unk0;
	byte version_hash;
	WORD cpu_hash;
	byte gpu_hash;
	byte unk1;
	byte network_hash;
	WORD unk2;
	byte unk3;
	WORD disk_hash;
	WORD unk4;
	byte terminator;
};

// As string representation
auto hwid_to_string(hwid_t hwid) -> std::string {
	return std::format("{:02X}", hwid.unk0)
		+ std::format("{:02X}", hwid.version_hash)
		+ std::format("{:04X}", _byteswap_ushort(hwid.cpu_hash))
		+ std::format("{:02X}", hwid.gpu_hash)
		+ std::format("{:02X}", hwid.unk1)
		+ std::format("{:02X}", hwid.network_hash)
		+ std::format("{:04X}", hwid.unk2)
		+ std::format("{:02X}", hwid.unk3)
		+ std::format("{:04X}", _byteswap_ushort(hwid.disk_hash))
		+ std::format("{:04X}", hwid.unk4);
}
```

#### OS Version

Struct: [OSVERSIONINFO](https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-osversioninfoa)

|MD5(Nl)|MD5(Nh)|MD5(num)|MD5(data[0])|MD5(data[1])|MD5(data[2])|MD5(data[3])|
|---|---|---|---|---|---|---|
|128|0|16|dwProcessorType|dwMajorVersion|dwBuildNumber|dwPlatformId|

#### CPU Info

Struct: [SYSTEM_INFO](https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/ns-sysinfoapi-system_info)

|MD5(Nl)|MD5(Nh)|MD5(num)|MD5(data[0])|MD5(data[1])|MD5(data[2])|MD5(data[3])|
|---|---|---|---|---|---|---|
|128|0|16|dwProcessorType|dwAllocationGranularity|wProcessorLevel|wProcessorRevision|

#### GPU Info

Struct: [D3DADAPTER_IDENTIFIER9](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dadapter-identifier9)

|MD5(Nl)|MD5(Nh)|MD5(num)|MD5(data[0])|MD5(data[1])|MD5(data[2])|MD5(data[3])|
|---|---|---|---|---|---|---|
|128|0|16|VendorId|DeviceId|SubSysId|Revision|

#### Network Info

Struct: [PIP_ADAPTER_INFO](https://learn.microsoft.com/en-us/windows/win32/api/iptypes/ns-iptypes-ip_adapter_info)

```admonish todo
Verify if this is correct: Skip hashing when adapter type is ethernet (`MIB_IF_TYPE_ETHERNET`).
```

|MD5(Nl)|MD5(Nh)|MD5(num)|MD5(data[0])|
|---|---|---|---|
|48|0|6|Address|

#### Volume Info

|MD5(Nl)|MD5(Nh)|MD5(num)|MD5(data[0])|
|---|---|---|---|
|128|0|4|_byteswap_ulong(lpVolumeSerialNumber)|

#### XOR Operation

```cpp
auto xor_op(byte* dest, byte* src, DWORD size) -> void {
	for (auto i = 1ul; i <= sizeof(MD5_LONG) * 4ul; ++i) {
		for (auto j = 0ul; j < size; ++j) {
			*(dest + j) ^= *(src++);
		}
	}
}

// Each part will xor its calculated MD5 hash
xor_op(&hwid.version_hash, (byte*)&data[0], sizeof(hwid_t::version_hash));
```

### DES Encryption

SecuROM uses DES [Cipher Feedback Encryption][] for the product activation process.
The OpenSSL version that is used for this is [from 2005][].

<img src="/images/1920px-CFB_encryption.svg.png" alt="cfb_image" style="background: white;">

```cpp
auto encrypt_with_des(
	const_DES_cblock* cblock,
	unsigned char* input,
	unsigned char* output,
	size_t length) -> void
{
	DES_key_schedule key_schedule = {};
	DES_cblock result =  {};

	DES_set_key_unchecked(cblock, &key_schedule);

	DES_cfb_encrypt(
		input,
		output,
		8,
		length,
		&key_schedule,
		&result,
		DES_ENCRYPT
	);
}
```

[Cipher Feedback Encryption]: https://en.wikipedia.org/wiki/Block_cipher_mode_of_operation#Cipher_feedback_(CFB)
[from 2005]: https://github.com/openssl/openssl/blob/OpenSSL_0_9_8/crypto/des/cfb_enc.c#L71-L73

### Unlock Code

After a fresh game installation, the product has to be activated once through a activation process which roughly works
like this:

- SecuROM generates unlock request code which contains the users's HWID and serial and sends this to the activation
  servers
- Server sends back the generated unlock code
- SecuROM unpacks the binary and saves the result in the registry

SecuROM provides a manual activation in case the automatic one fails.
This means that the user has to enter the unlock request code + serial manually on SecuROM's web page to request
the unlock code. When SecuROM decides to shutdown their service, like in the case for Tron: Evolution, users
would not be able to progress further. The product simply cannot be activated, or can it?

Fortunately, the unlock code can be generated locally without the need of SecuROM's activation servers in the
following manner:

```admonish note
The complete code can be found in the [unlocker](https://github.com/NeKzor/tem/tree/master/unlocker) folder
as this is only a summary.
```

Generate the user's hardware ID, see chapter [HWID](#hardware-id-hwid).

```cpp
// Example: "010BE04D5A009B00000037210000"
auto hwid = generate_user_hwid();
```

Calculate the RSA plaintext:

\\( c = m^e \mod n \\)

|Variable|Description|
|---|---|
|c|Plaintext|
|m|HWID|
|e|0xB4109B85B0CAFBD73EDDAB05A9881|
|n|0x1CF9DFF37F133D15D21CC4F5ADE91F|

```cpp
// NOTE: All Numbers are "huge" integers. A magic library should handle the maths :)
auto m = huge_integer_from_hex(hwid);
auto e = huge_integer_from_hex("B4109B85B0CAFBD73EDDAB05A9881");
auto n = huge_integer_from_hex("1CF9DFF37F133D15D21CC4F5ADE91F");
auto c = mod_exp(m, e, n);
```

Calculate plaintext length. The total length should be 30 but the number can be lower than that.
Simply fill the rest with 'f' characters.
Then append the original length at the end of the buffer.

```cpp
auto plaintext = huge_integer_to_hex_string(c);

const auto max_plaintext_length = 30;

auto plaintext_length = plaintext.length();
auto offset = max_plaintext_length - plaintext_length;

if ((offset & 0x80000000) != 0) {
	println("[-] invalid plaintext length :(");
	return false;
}

if (max_plaintext_length != plaintext_length) {
	do {
		plaintext += "f";
		--offset;
	} while (offset);
}

plaintext += std::format("{:02x}", plaintext_length);
```

Convert hex string back to a byte buffer and XOR everything with the game's appid signature.

```cpp
// Game signature aka appid (48 bytes)
// Found in spot check 6
unsigned char appid[] = {
	// 1st part (16 bytes)
	0xF9, 0x83, 0x7A, 0x1D, 0x22, 0x2F, 0x64, 0x74,
	0x28, 0xCB, 0x13, 0x30, 0x32, 0xD0, 0xD0, 0x0C,
	// 2nd part (32 bytes)
	0xE8, 0x96, 0xD4, 0xE1, 0xBD, 0xFC, 0x0E, 0x37,
	0x8C, 0x8D, 0x17, 0x74, 0x27, 0x27, 0xBC, 0xE0,
	0xE8, 0x96, 0xD4, 0xE1, 0xBD, 0xFC, 0x0E, 0x37,
	0xE8, 0x96, 0xD4, 0xE1, 0xBD, 0xFC, 0x0E, 0x37,
};

BYTE data_buffer[56] = { 0x00, 0x01, 0xC7, 0x22 };
auto data_buffer_ptr = data_buffer + 4;

convert_hex_to_bytes(data_buffer_ptr + 5, (BYTE*)plaintext.c_str(), 32);

for (auto i = 0; i < 16; ++i) {
	*(data_buffer_ptr + 5 + i) ^= *(appid + i);
}
```

Calculate MD5 of game's appid and XOR it with the buffer.

```cpp
MD5_CTX ctx = {};
MD5_Init(&ctx);
MD5_Update(&ctx, appid, sizeof(appid));
BYTE data[32] = {};
MD5_Final(data, &ctx);

xor_data(data, 16, data_buffer_ptr, 2u);
```

Encrypt the game's signature with lots of XOR operations and DES CBF.

```cpp
BYTE cblock[19] = {};
for (auto i = 0; i < 16; ++i) {
	*(cblock + i) = *(appid + i) ^ *(appid + i + (16 * 1)) ^ *(appid + i + (16 * 2));
}

memcpy(cblock + 16, (BYTE*)plaintext.c_str(), 3);

BYTE des_buffer[21] = {};
memcpy(des_buffer, data_buffer_ptr + 2, sizeof(des_buffer));

*(cblock + 0) ^= *(cblock + 8);
*(cblock + 1) ^= *(cblock + 9);
*(cblock + 2) ^= *(cblock + 10);
*(cblock + 3) ^= *(cblock + 11);
*(cblock + 4) ^= *(cblock + 12);
*(cblock + 5) ^= *(cblock + 13);
*(cblock + 6) ^= *(cblock + 14);
*(cblock + 7) ^= *(cblock + 15);
*(cblock + 8) ^= *(cblock + 16);

encrypt_with_des(cblock, data_buffer_ptr + 2, des_buffer, sizeof(des_buffer) - 2);
```

Now comes the longest process: S-box DES encryption times 2<sup>21</sup>.
This will take a few seconds depending on the CPU's speed lol.

```cpp
auto rounds = 0x20000;

BYTE output_buffer[64] = { *(data_buffer_ptr + 0), *(data_buffer_ptr + 1) };
memcpy(output_buffer + 2, des_buffer, 19);

do {
	for (auto i = 0; i < 8; ++i) {
		des_encrypt_with_sbox(output_buffer, data_buffer_ptr, 21);
		++des_calls;
		des_encrypt_with_sbox(data_buffer_ptr, output_buffer, 21);
		++des_calls;
	}
	--rounds;
} while (rounds);
```

Calculate CRC and XOR it with the buffer.

```cpp
auto crc = get_crc(data_buffer + 1, 24);
xor_data((BYTE*)&crc, 4, data_buffer, 1);
```

Final DES CFB encryption with random seed.

```cpp
BYTE cblock_seed[17] = {};
get_random_hash(cblock_seed, sizeof(cblock_seed) - 1);

memcpy(cblock_seed + 16, cblock, 1);

*(cblock_seed + 0) ^= *(cblock_seed + 8);
*(cblock_seed + 1) ^= *(cblock_seed + 9);
*(cblock_seed + 2) ^= *(cblock_seed + 10);
*(cblock_seed + 3) ^= *(cblock_seed + 11);
*(cblock_seed + 4) ^= *(cblock_seed + 12);
*(cblock_seed + 5) ^= *(cblock_seed + 13);
*(cblock_seed + 6) ^= *(cblock_seed + 14);
*(cblock_seed + 7) ^= *(cblock_seed + 15);
*(cblock_seed + 8) ^= *(cblock_seed + 16);

BYTE unlock_code_des_buffer[32] = {};
encrypt_with_des(cblock_seed, data_buffer, unlock_code_des_buffer, 25);

BYTE unlock_code_hex_buffer[40] = {};
decode_des_buffer(unlock_code_des_buffer, 25, unlock_code_hex_buffer, sizeof(unlock_code_hex_buffer));
```

Encode rest of the buffer to ASCII and insert hyphens etc.

```cpp
decode_to_ascii(unlock_code_hex_buffer, output_buffer, sizeof(unlock_code_hex_buffer));

char unlock_code_buffer[64] = {};
insert_hyphens((char*)output_buffer, unlock_code_buffer, 40, 5);
```

Credits to 80_PA.
