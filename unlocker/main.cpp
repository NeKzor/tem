/*
 * Copyright (c) 2022-2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 *
 *
 * This generates the unlock code for SecuROM's manual activation.
 * Code is based on on 80_PA.
 *
 * Read for more information: https://tem.nekz.me/reversed/securom.html#unlock-code
 *
 * Example output:
 *	UB9NU-ZU3LF-6R5ZQ-LPYE5-8C3UH-RGR3P-HX5NP-P6GHQ
 *	PKZA9-GUVLN-9ATMR-YGL9G-EVTG7-FC2PH-UDD43-K9G69
 */

#pragma warning(disable : 4002)
#pragma warning(disable : 4996)

// clang-format off

#include <ctime>
#include <d3d9.h>
#include <format>
#include <immintrin.h>
#include <iostream>
#include <iptypes.h>
#include <openssl/md5.h>
#include <openssl/opensslv.h>
#include <openssl/des.h>
#include "Unlocker.hpp"
#include "lib/BigDigits/bigd.h"

// clang-format on

#ifdef LOBYTE
    #undef LOBYTE
#endif

#define BYTEn(x, n) (*((char*)&(x) + n))
#define BYTE2(x) BYTEn(x, 2)
#define BYTE3(x) BYTEn(x, 3)
#define LOW_IND(x, part_type) 0
#define LOBYTE(x) BYTEn(x, LOW_IND(x, BYTE))

hwid_t::hwid_t()
{
    this->get_version_hash();
    this->get_cpu_hash();
    this->get_gpu_hash();
    this->get_network_hash();
    this->get_disk_hash();
}

auto hwid_t::hash_field(BYTE* dest, BYTE* src, DWORD size) -> void
{
    for (auto i = 1ul; i <= sizeof(MD5_LONG) * 4ul; ++i) {
        for (auto j = 0ul; j < size; ++j) {
            *(dest + j) ^= *(src++);
        }
    }
}

auto hwid_t::get_version_hash() -> void
{
    OSVERSIONINFO osvi = {};
    ZeroMemory(&osvi, 0, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);

    MD5_CTX ctx = {};
    MD5_Init(&ctx);

    ctx.Nl = 128;
    ctx.Nh = 0;
    ctx.num = MD5_DIGEST_LENGTH;
    ctx.data[0] = osvi.dwMajorVersion;
    ctx.data[1] = osvi.dwMinorVersion;
    ctx.data[2] = osvi.dwBuildNumber;
    ctx.data[3] = osvi.dwPlatformId;

    unsigned long data[32] = {};
    MD5_Final((unsigned char*)&data[0], &ctx);

    this->hash_field(&this->version_hash, (byte*)&data[0], sizeof(hwid_t::version_hash));
}

auto hwid_t::get_cpu_hash() -> void
{
    SYSTEM_INFO info = {};
    GetSystemInfo(&info);

    MD5_CTX ctx = {};
    MD5_Init(&ctx);

    ctx.Nl = 128;
    ctx.Nh = 0;
    ctx.num = MD5_DIGEST_LENGTH;
    ctx.data[0] = info.dwProcessorType;
    ctx.data[1] = info.dwAllocationGranularity;
    ctx.data[2] = info.wProcessorLevel;
    ctx.data[3] = info.wProcessorRevision;

    unsigned long data[32] = {};
    MD5_Final((unsigned char*)&data[0], &ctx);

    this->hash_field((byte*)&this->cpu_hash, (byte*)&data[0], sizeof(hwid_t::cpu_hash));
}

auto hwid_t::get_gpu_hash() -> void
{
    auto d3d9Module = LoadLibrary(L"d3d9.dll");
    if (d3d9Module != NULL) {
        D3DADAPTER_IDENTIFIER9 gpu = {};

        typedef PDIRECT3D9(__stdcall * _Direct3DCreate9)(UINT SDKVersion);
        auto Direct3DCreate9 = (_Direct3DCreate9)GetProcAddress((HMODULE)d3d9Module, "Direct3DCreate9");
        auto d3d9 = Direct3DCreate9(D3D_SDK_VERSION);

        d3d9->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &gpu);

        FreeLibrary(d3d9Module);

        MD5_CTX ctx = {};
        MD5_Init(&ctx);

        ctx.Nl = 128;
        ctx.Nh = 0;
        ctx.num = MD5_DIGEST_LENGTH;
        ctx.data[0] = gpu.VendorId;
        ctx.data[1] = gpu.DeviceId;
        ctx.data[2] = gpu.SubSysId;
        ctx.data[3] = gpu.Revision;

        unsigned long data[32] = {};
        MD5_Final((unsigned char*)&data[0], &ctx);

        this->hash_field((byte*)&this->gpu_hash, (byte*)&data[0], sizeof(hwid_t::gpu_hash));
    }
}

auto hwid_t::get_network_hash() -> void
{
    auto iphlpapiModule = LoadLibrary(L"IPHLPAPI.dll");
    if (iphlpapiModule != NULL) {
        PIP_ADAPTER_INFO pAdapterInfo = {};
        ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO) * 8;

        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO) * 8);
        if (!pAdapterInfo) {
            std::cout << "[-] oom :(" << std::endl;
            return;
        }

        typedef ULONG(__stdcall * _IPHLPAPI_GetAdaptersInfo)(PIP_ADAPTER_INFO AdapterInfo, PULONG SizePointer);
        auto GetAdaptersInfo = (_IPHLPAPI_GetAdaptersInfo)GetProcAddress((HMODULE)iphlpapiModule, "GetAdaptersInfo");

        GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
        FreeLibrary(iphlpapiModule);

        auto checkForEthernet = true;

        if (pAdapterInfo->Type == MIB_IF_TYPE_ETHERNET && checkForEthernet) {
            MD5_CTX ctx = {};
            MD5_Init(&ctx);

            ctx.Nl = 48;
            ctx.Nh = 0;
            ctx.num = MD5_DIGEST_LENGTH - 10;
            memcpy(&ctx.data[0], pAdapterInfo->Address, sizeof(pAdapterInfo->Address));

            unsigned long data[32] = {};
            MD5_Final((unsigned char*)&data[0], &ctx);

            this->hash_field(&this->network_hash, (byte*)&data[0], sizeof(hwid_t::network_hash));
        }

        free((void*)pAdapterInfo);
    }
}

auto hwid_t::get_disk_hash() -> void
{
    char volumeNameBuffer[MAX_PATH] = {};
    char fileSystemNameBuffer[MAX_PATH] = {};
    DWORD volumeSerialNumber = 0;
    DWORD maximumComponentLength = 0;
    DWORD fileSystemFlags = 0;

    char volume_name[4] = "c:\\";

    for (; volume_name[0] <= *"z"; ++volume_name[0]) {
        auto dtype = GetDriveTypeA((LPCSTR)&volume_name[0]);
        //std::cout << std::format("[+] volume_name: {} (dtype = {})", volume_name, dtype) << std::endl;

        if (dtype != DRIVE_FIXED) {
            continue;
        }

        GetVolumeInformationA((LPCSTR)&volume_name, (LPSTR)volumeNameBuffer, sizeof(volumeNameBuffer),
            &volumeSerialNumber, &maximumComponentLength, &fileSystemFlags, (LPSTR)fileSystemNameBuffer,
            sizeof(fileSystemNameBuffer));

        std::cout << std::format("[+] volume {} with serial number: {}", volume_name, volumeSerialNumber) << std::endl;

        break;
    }

    MD5_CTX ctx = {};
    MD5_Init(&ctx);

    ctx.Nl = 32;
    ctx.Nh = 0;
    ctx.num = 4;
    ctx.data[0] = _byteswap_ulong(volumeSerialNumber);

    unsigned long data[32] = {};
    MD5_Final((unsigned char*)&data[0], &ctx);

    this->hash_field((byte*)&this->disk_hash, (byte*)&data[0], sizeof(hwid_t::disk_hash));
}

auto hwid_t::to_string() -> std::string
{
    return std::format("{:02X}", this->unk0) + std::format("{:02X}", this->version_hash)
        + std::format("{:04X}", _byteswap_ushort(this->cpu_hash)) + std::format("{:02X}", this->gpu_hash)
        + std::format("{:02X}", this->unk1) + std::format("{:02X}", this->network_hash)
        + std::format("{:04X}", this->unk2) + std::format("{:02X}", this->unk3)
        + std::format("{:04X}", _byteswap_ushort(this->disk_hash)) + std::format("{:04X}", this->unk4);
}

auto println(const char* txt) -> void { std::cout << txt << std::endl; }

auto print_buffer(const char* prefix, char* buf, size_t size) -> void
{
    auto dataPtr = buf;
    auto dataPtrIdx = 0;
    std::cout << prefix;
    while (dataPtrIdx < size) {
        std::cout << std::format("{:02X}", dataPtr[dataPtrIdx++]);
    }
    std::cout << std::endl;
}

auto print_buffer_ascii(const char* prefix, char* buf, size_t size) -> void
{
    auto dataPtr = buf;
    auto dataPtrIdx = 0;
    std::cout << prefix;
    while (dataPtrIdx < size) {
        std::cout << dataPtr[dataPtrIdx++];
    }
    std::cout << std::endl;
}

auto string_to_hex(const std::string& input) -> std::string
{
    static const char hex_digits[] = "0123456789ABCDEF";

    std::string output;
    output.reserve(input.length() * 2);
    for (unsigned char c : input) {
        output.push_back(hex_digits[c >> 4]);
        output.push_back(hex_digits[c & 15]);
    }
    return output;
}

auto convert_int_to_hex(unsigned int count, char* buf) -> int
{
    char* buf_1; // eax@1
    unsigned int count_1; // ebp@1
    unsigned int v4; // edi@2
    unsigned int v5; // ecx@2
    char* v6; // edx@6
    char* v7; // ebp@6
    int result; // eax@7

    buf_1 = buf;
    count_1 = count;

    do {
        v4 = count_1 & 0xF;
        count_1 >>= 4;
        v5 = v4 << 24;

        if (v4 <= 9) {
            *buf++ = (v5 + 0x30000000) >> 24;
        } else {
            *buf++ = (v5 + 0x57000000) >> 24;
        }
    } while (count_1);

    *buf = 0;
    v6 = buf - 1;
    v7 = buf_1;

    do {
        result = *v6;
        *v6 = *v7;
        *v7 = result;
        --v6;
        ++v7;
    } while (v7 < v6);

    return result;
}

/*
 * Convert hex input buffer to byte buffer.
 */
auto convert_hex_to_bytes(BYTE* output, BYTE* input, size_t length) -> BYTE*
{
    auto next_input = input + 1;
    auto bytes_length = length >> 1;

    if (length >> 1) {
        do {
            *output++ = ((16 * ascii_table[*input]) | ascii_table[*next_input]);
            next_input += 2;
            input += 2;
            --bytes_length;
        } while (bytes_length);
    }

    return output;
}

/*
 * TODO
 */
auto figure_out_what_this_does_4(BYTE* input, size_t input_length, BYTE* output, size_t output_length) -> signed int
{
    if (input && output) {
        if (output_length > 0) {
            auto v4 = 31;
            auto v5 = 0;
            auto v6 = 1;
            auto v7 = 0;
            auto v8 = 0;

            do {
                auto v9 = v6 >= input_length ? 0 : input[v6];
                v7 = input[v6 - 1] | ((v9 << 8) | v7 & 0xFFFF00FF) & 0xFFFFFF00;
                output[v8] = (v7 & v4) >> v5;
                v5 += 5;
                ++v8;
                v4 *= 32;

                if (v5 >= 8) {
                    ++v6;
                    v4 >>= 8;
                    v5 -= 8;
                }
            } while (v8 < output_length);
        }

        return 1;
    }

    return 0;
}

/*
 * Decode input buffer to ASCII.
 */
auto decode_to_ascii(BYTE* input, BYTE* output, size_t length) -> signed int
{
    if (!length) {
        return 1;
    }

    auto index = 0;
    while (true) {
        auto c = *input;

        if (c <= 7) { // 2-9
            *output = ((c << 24) + 0x32000000) >> 24;
            goto next;
        }
        if (c >= 8 && c <= 0xF) { // A-H
            *output = ((c << 24) + 0x39000000) >> 24;
            goto next;
        }
        if (c < 0x10) {
            if (c < 0x15) {
                return 0;
            }
            goto decode;
        }
        if (c <= 0x14) { // D-N
            *output = ((c << 24) + 0x3A000000) >> 24;
            goto next;
        }
        if (c < 0x15 || c > 0x1F) {
            return 0;
        }
    decode:
        *output = ((c << 24) + 0x3B000000) >> 24;
    next:
        ++output;
        ++input;
        auto total_length = index++;
        if (total_length >= length) {
            return 1;
        }
    }
}

/*
 * Insert '-' into input buffer every "group length" amount of characters.
 */
auto insert_hyphens(char* input, char* output, size_t length, size_t group_length) -> int
{
    auto input_1 = input;
    auto length_1 = length;
    auto output_1 = output;
    auto group_length_1 = group_length;

    do {
        auto remaining = group_length_1--;
        if (remaining) {
            *output_1++ = *input_1++;
            --length_1;
        } else {
            *output_1++ = '-';
            group_length_1 = group_length;
        }
    } while (length_1);

    *input_1 = 0;

    return 1;
}

/*
 * XOR input buffer and store result into output buffer.
 */
auto xor_data(BYTE* input, size_t input_length, BYTE* output, size_t output_length) -> void
{
    memset(output, 0, output_length);
    auto length = output_length;
    auto buf_end = -(signed int)output;
    auto total = 0;
    auto result = 0;

    do {
        auto ptr = output;
        total = result;
        auto buf_end_1 = buf_end;
        auto index = length;
        auto offset = (size_t)output + buf_end;

        do {
            auto data_index = offset++ % input_length;
            unsigned char temp = *(input + data_index);
            *ptr ^= temp;
            ++ptr;
            --index;
        } while (index);

        length = output_length;
        result += output_length;
        buf_end = output_length + buf_end_1;
    } while ((output_length + total) < input_length);
}

/*
 * Encrypt input buffer with DES CFB.
 */
auto encrypt_with_des(BYTE* cblock, BYTE* data, BYTE* buf, size_t length) -> void
{
    *(cblock + 0) ^= *(cblock + 8);
    *(cblock + 1) ^= *(cblock + 9);
    *(cblock + 2) ^= *(cblock + 10);
    *(cblock + 3) ^= *(cblock + 11);
    *(cblock + 4) ^= *(cblock + 12);
    *(cblock + 5) ^= *(cblock + 13);
    *(cblock + 6) ^= *(cblock + 14);
    *(cblock + 7) ^= *(cblock + 15);
    *(cblock + 8) ^= *(cblock + 16);

    DES_key_schedule sch_key = {};
    DES_set_key_unchecked((const_DES_cblock*)cblock, &sch_key);

    DES_cblock result = {};
    DES_cfb_encrypt(data, buf, 8, length, &sch_key, &result, DES_ENCRYPT);
}

auto des_encrypt_with_sbox(BYTE* input, BYTE* output, int length) -> signed int
{
    signed int index; // ebx@1
    unsigned int number; // ecx@3
    BYTE* current_byte; // edx@3
    BYTE* byte_mask; // esi@3
    signed int bits; // eax@3
    BYTE v8; // bl@6
    unsigned int v9; // edx@6
    unsigned int v10; // edx@6
    unsigned int v11; // ecx@6
    unsigned int v12; // edx@6
    unsigned int v13; // esi@6
    unsigned int v14; // edi@6
    unsigned int v15; // edx@6
    unsigned int v16; // ecx@6
    unsigned int v17; // esi@6
    unsigned int v18; // edx@6
    unsigned int v19; // ecx@6
    unsigned int v20; // esi@6
    unsigned int v21; // edx@6
    unsigned int v22; // ecx@6
    unsigned int v23; // esi@6
    unsigned int v24; // edi@6
    unsigned int v25; // ecx@6
    unsigned int v26; // esi@6
    unsigned int v27; // edi@6
    unsigned int v28; // ecx@6
    unsigned int v29; // edi@6
    unsigned int v30; // esi@6
    unsigned int v31; // edi@6
    unsigned int v32; // ecx@6
    unsigned int v33; // esi@6
    unsigned int v34; // edx@6
    unsigned int v35; // ecx@6
    unsigned int v36; // edi@6
    char v37; // bl@6
    BYTE* current_buf_ptr; // ecx@6
    signed int index_1; // esi@6
    unsigned int v40; // ebx@7
    BYTE* byte_mask_1; // edi@7
    signed int bits_1; // eax@7
    BYTE* v43; // edx@7
    BYTE* data_2; // edx@10
    BYTE result_buffer[20] = {}; // [sp+0h] [bp-44h]@1
    BYTE buffer[24] = {}; // [sp+14h] [bp-30h]@2
    int offset; // [sp+2Ch] [bp-18h]@1
    int length_1; // [sp+34h] [bp-10h]@1
    BYTE* data_1; // [sp+38h] [bp-Ch]@1
    BYTE* buf_ptr; // [sp+54h] [bp+10h]@2

    index = 0;
    *(unsigned int*)&result_buffer[12] = 0;
    *(unsigned int*)&result_buffer[16] = 0;
    data_1 = input;
    length_1 = length;
    do {
        *(unsigned int*)buffer = *(unsigned int*)&result_buffer[12];
        *(unsigned int*)&buffer[4] = *(unsigned int*)&result_buffer[16];
        *(unsigned int*)&buffer[8] = 0;
        *(unsigned int*)&buffer[12] = 0;
        buf_ptr = buffer;
        do {
            number = (unsigned __int8)*buf_ptr;
            current_byte = &buffer[8];
            byte_mask = 8 * (index + (number >> 4)) + unk_des_mask;
            bits = 8;
            do {
                *current_byte |= *byte_mask | *(BYTE*)(8 * ((number & 0xF) - (number >> 4)) + 128 + byte_mask);
                ++current_byte;
                ++byte_mask;
                --bits;
            } while (bits);
            index += 32;
            buf_ptr = (buf_ptr + 1);
        } while (index < 256);
        *(unsigned int*)&buffer[16] = *(unsigned int*)&buffer[8];
        v8 = buffer[17];
        buffer[19] = buffer[8];
        buffer[16] = buffer[11];
        buffer[17] = buffer[10];
        buffer[23] = buffer[12];
        buffer[20] = buffer[15];
        buffer[22] = buffer[13];
        buffer[21] = buffer[14];
        v9 = (*(unsigned int*)&buffer[20] >> 1) | (*(unsigned int*)&buffer[20] << 31);
        v10 = des_sbox_2[((v9 ^ 0x68000) >> 14) & 0x3F] | des_sbox_7[((v9 >> 6) ^ 0xFFFFFFF2) & 0x3F]
            | des_sbox_4[((v9 >> 10) ^ 0xFFFFFFF1) & 0x3F] | des_sbox_1[((v9 >> 26) ^ 0xFFFFFFF1) & 0x3F]
            | des_sbox_6[((v9 >> 18) ^ 0xFFFFFFED) & 0x3F] | des_sbox_5[((v9 ^ 0x38) >> 2) & 0x3F]
            | des_sbox_3[((v9 ^ 0x1800000) >> 22) & 0x3F]
            | des_sbox_8[((2 * *(unsigned int*)&buffer[20] | (*(unsigned int*)&buffer[20] >> 31)) ^ 0xFFFFFFF7) & 0x3F];
        buffer[18] = v8;
        v11 = v10 ^ *(unsigned int*)&buffer[16];
        v12 = ((v10 ^ *(unsigned int*)&buffer[16]) >> 1) | ((v10 ^ *(unsigned int*)&buffer[16]) << 31);
        v13 = (des_sbox_1[(v12 ^ 0x24000000) >> 26] | des_sbox_2[((v12 >> 14) ^ 0xFFFFFFF6) & 0x3F]
                  | des_sbox_4[((v12 >> 10) ^ 0xFFFFFFED) & 0x3F] | des_sbox_3[((v12 >> 22) ^ 0xFFFFFFE4) & 0x3F]
                  | des_sbox_5[((v12 ^ 0x34) >> 2) & 0x3F] | des_sbox_6[((v12 ^ 0x3C0000) >> 18) & 0x3F]
                  | des_sbox_7[((v12 ^ 0x780) >> 6) & 0x3F] | des_sbox_8[((2 * v11 | (v11 >> 31)) ^ 0x13) & 0x3F])
            ^ *(unsigned int*)&buffer[20];
        v14 = (v13 >> 1) | (v13 << 31);
        v15 = des_sbox_4[(((v14 ^ 0x7400) >> 8) >> 2) & 0x3F] | des_sbox_7[((v14 >> 6) ^ 0xFFFFFFFA) & 0x3F]
            | des_sbox_6[((v14 ^ 0x100000) >> 18) & 0x3F] | des_sbox_1[((v14 >> 26) ^ 0xFFFFFFEF) & 0x3F]
            | des_sbox_3[((v14 >> 22) ^ 0xFFFFFFE5) & 0x3F] | des_sbox_5[((v14 ^ 0x64) >> 2) & 0x3F]
            | des_sbox_2[((v14 ^ 0x14000) >> 14) & 0x3F];
        *(unsigned int*)&buffer[20] = v13;
        v16 = (v15 | des_sbox_8[((2 * v13 | (v13 >> 31)) ^ 6) & 0x3F]) ^ v11;
        v17 = (des_sbox_1[(((v16 >> 1) | (v16 << 31)) ^ 0x8000000) >> 26]
                  | des_sbox_4[~(((v16 >> 1) | (v16 << 31)) >> 10) & 0x3F]
                  | des_sbox_2[((((v16 >> 1) | (v16 << 31)) >> 14) ^ 0xFFFFFFF9) & 0x3F]
                  | des_sbox_3[((((v16 >> 1) | (v16 << 31)) >> 22) ^ 0xFFFFFFF0) & 0x3F]
                  | des_sbox_6[((((v16 >> 1) | (v16 << 31)) ^ 0x340000) >> 18) & 0x3F]
                  | des_sbox_5[((((v16 >> 1) | (v16 << 31)) ^ 0x5C) >> 2) & 0x3F]
                  | des_sbox_7[((((v16 >> 1) | (v16 << 31)) ^ 0x280) >> 6) & 0x3F]
                  | des_sbox_8[((2 * v16 | (v16 >> 31)) ^ 0xE) & 0x3F])
            ^ v13;
        v18 = des_sbox_1[((((v17 >> 1) | (v17 << 31)) >> 26) ^ 0xFFFFFFE3) & 0x3F]
            | des_sbox_5[((((v17 >> 1) | (v17 << 31)) ^ 0x6C) >> 2) & 0x3F]
            | des_sbox_7[((((v17 >> 1) | (v17 << 31)) ^ 0x3C0) >> 6) & 0x3F]
            | des_sbox_3[((((v17 >> 1) | (v17 << 31)) ^ 0x5400000) >> 22) & 0x3F]
            | des_sbox_4[((((v17 >> 1) | (v17 << 31)) ^ 0x2C00) >> 10) & 0x3F];
        *(unsigned int*)&buffer[20] = v17;
        v19 = (des_sbox_2[((((v17 >> 1) | (v17 << 31)) >> 14) ^ 0xFFFFFFFD) & 0x3F]
                  | des_sbox_6[((((v17 >> 1) | (v17 << 31)) ^ 0x80000) >> 18) & 0x3F] | v18
                  | des_sbox_8[((2 * v17 | (v17 >> 31)) ^ 0xF) & 0x3F])
            ^ v16;
        v20 = (des_sbox_4[((((v19 >> 1) | (v19 << 31)) ^ 0x7C00) >> 10) & 0x3F]
                  | des_sbox_2[((((v19 >> 1) | (v19 << 31)) ^ 0x20000) >> 14) & 0x3F]
                  | des_sbox_7[((((v19 >> 1) | (v19 << 31)) >> 6) ^ 0xFFFFFFEF) & 0x3F]
                  | des_sbox_6[((((v19 >> 1) | (v19 << 31)) >> 18) ^ 0xFFFFFFEF) & 0x3F]
                  | des_sbox_3[((((v19 >> 1) | (v19 << 31)) >> 22) ^ 0xFFFFFFF0) & 0x3F]
                  | des_sbox_1[((((v19 >> 1) | (v19 << 31)) >> 26) ^ 0xFFFFFFE5) & 0x3F]
                  | des_sbox_5[((((v19 >> 1) | (v19 << 31)) ^ 0x4C) >> 2) & 0x3F]
                  | des_sbox_8[((2 * v19 | (v19 >> 31)) ^ 0xFFFFFFE3) & 0x3F])
            ^ v17;
        v21 = des_sbox_3[((((v20 >> 1) | (v20 << 31)) >> 22) ^ 0xFFFFFFE7) & 0x3F]
            | des_sbox_2[((((v20 >> 1) | (v20 << 31)) >> 14) ^ 0xFFFFFFE1) & 0x3F];
        *(unsigned int*)&buffer[20] = v20;
        v22 = (des_sbox_1[(((v20 >> 1) | (v20 << 31)) ^ 0x18000000) >> 26]
                  | des_sbox_5[((((v20 >> 1) | (v20 << 31)) >> 2) ^ 0xFFFFFFF5) & 0x3F]
                  | des_sbox_7[((((v20 >> 1) | (v20 << 31)) >> 6) ^ 0xFFFFFFEC) & 0x3F]
                  | des_sbox_4[((((v20 >> 1) | (v20 << 31)) >> 10) ^ 0xFFFFFFEB) & 0x3F]
                  | des_sbox_6[((((v20 >> 1) | (v20 << 31)) >> 18) ^ 0xFFFFFFEA) & 0x3F] | v21
                  | des_sbox_8[((2 * v20 | (v20 >> 31)) ^ 0xFFFFFFEB) & 0x3F])
            ^ v19;
        v23 = (des_sbox_5[((((v22 >> 1) | (v22 << 31)) >> 2) ^ 0xFFFFFFFD) & 0x3F]
                  | des_sbox_2[((((v22 >> 1) | (v22 << 31)) ^ 0x30000) >> 14) & 0x3F]
                  | des_sbox_7[((((v22 >> 1) | (v22 << 31)) >> 6) ^ 0xFFFFFFEF) & 0x3F]
                  | des_sbox_6[((((v22 >> 1) | (v22 << 31)) >> 18) ^ 0xFFFFFFF5) & 0x3F]
                  | des_sbox_4[((((v22 >> 1) | (v22 << 31)) >> 10) ^ 0xFFFFFFE3) & 0x3F]
                  | des_sbox_1[((((v22 >> 1) | (v22 << 31)) >> 26) ^ 0xFFFFFFE6) & 0x3F]
                  | des_sbox_3[((((v22 >> 1) | (v22 << 31)) ^ 0x4C00000) >> 22) & 0x3F]
                  | des_sbox_8[((2 * v22 | (v22 >> 31)) ^ 0x12) & 0x3F])
            ^ v20;
        *(unsigned int*)&buffer[20] = v23;
        v24 = (v23 >> 1) | (v23 << 31);
        v25 = (des_sbox_1[v24 >> 26] | des_sbox_2[((v24 ^ 0x78000) >> 14) & 0x3F]
                  | des_sbox_4[((v24 >> 10) ^ 0xFFFFFFF3) & 0x3F] | des_sbox_6[((v24 >> 18) ^ 0xFFFFFFF2) & 0x3F]
                  | des_sbox_5[((v24 >> 2) ^ 0xFFFFFFE5) & 0x3F] | des_sbox_3[((v24 >> 22) ^ 0xFFFFFFF2) & 0x3F]
                  | des_sbox_7[((v24 ^ 0x440) >> 6) & 0x3F] | des_sbox_8[((2 * v23 | (v23 >> 31)) ^ 0xFFFFFFFB) & 0x3F])
            ^ v22;
        v26 = (des_sbox_1[(((v25 >> 1) | (v25 << 31)) ^ 0x7C000000) >> 26]
                  | des_sbox_7[((((v25 >> 1) | (v25 << 31)) >> 6) ^ 0xFFFFFFFD) & 0x3F]
                  | des_sbox_2[((((v25 >> 1) | (v25 << 31)) >> 14) ^ 0xFFFFFFF4) & 0x3F]
                  | des_sbox_4[((((v25 >> 1) | (v25 << 31)) >> 10) ^ 0xFFFFFFF1) & 0x3F]
                  | des_sbox_5[((((v25 >> 1) | (v25 << 31)) >> 2) ^ 0xFFFFFFE9) & 0x3F]
                  | des_sbox_6[((0x480000 ^ ((v25 >> 1) | (v25 << 31))) >> 18) & 0x3F]
                  | des_sbox_3[((((v25 >> 1) | (v25 << 31)) ^ 0x800000) >> 22) & 0x3F]
                  | des_sbox_8[((2 * v25 | (v25 >> 31)) ^ 0xFFFFFFFC) & 0x3F])
            ^ v23;
        v27 = (v26 >> 1) | (v26 << 31);
        v28 = (des_sbox_5[~(v27 >> 2) & 0x3F] | des_sbox_1[((v27 >> 26) ^ 0xFFFFFFF5) & 0x3F]
                  | des_sbox_6[((v27 >> 18) ^ 0xFFFFFFF0) & 0x3F] | des_sbox_2[((v27 >> 14) ^ 0xFFFFFFE8) & 0x3F]
                  | des_sbox_3[((v27 >> 22) ^ 0xFFFFFFE8) & 0x3F] | des_sbox_7[((v27 ^ 0x640) >> 6) & 0x3F]
                  | des_sbox_4[((((v26 >> 1) ^ 0x5000) >> 8) >> 2) & 0x3F]
                  | des_sbox_8[((2 * v26 | (v26 >> 31)) ^ 0xFFFFFFFC) & 0x3F])
            ^ v25;
        v29 = des_sbox_7[((((v28 >> 1) | (v28 << 31)) ^ 0x640) >> 6) & 0x3F]
            | des_sbox_8[((2 * v28 | (v28 >> 31)) ^ 0xFFFFFFFD) & 0x3F];
        *(unsigned int*)&buffer[20] = v26;
        v30 = (des_sbox_5[((((v28 >> 1) | (v28 << 31)) >> 2) ^ 0xFFFFFFF2) & 0x3F]
                  | des_sbox_2[((((v28 >> 1) | (v28 << 31)) >> 14) ^ 0xFFFFFFF6) & 0x3F]
                  | des_sbox_1[((((v28 >> 1) | (v28 << 31)) >> 26) ^ 0xFFFFFFF2) & 0x3F]
                  | des_sbox_4[((((v28 >> 1) | (v28 << 31)) >> 10) ^ 0xFFFFFFE6) & 0x3F]
                  | des_sbox_3[((((v28 >> 1) | (v28 << 31)) >> 22) ^ 0xFFFFFFEA) & 0x3F]
                  | des_sbox_6[((((v28 >> 1) | (v28 << 31)) ^ 0x200000) >> 18) & 0x3F] | v29)
            ^ v26;
        v31 = (v30 >> 1) | (v30 << 31);
        v32 = (des_sbox_4[(((v31 ^ 0x4800) >> 8) >> 2) & 0x3F] | des_sbox_5[((v31 >> 2) ^ 0xFFFFFFFA) & 0x3F]
                  | des_sbox_7[((v31 >> 6) ^ 0xFFFFFFF7) & 0x3F] | des_sbox_2[((v31 ^ 0x38000) >> 14) & 0x3F]
                  | des_sbox_6[((v31 >> 18) ^ 0xFFFFFFF8) & 0x3F] | des_sbox_1[((v31 >> 26) ^ 0xFFFFFFEB) & 0x3F]
                  | des_sbox_3[((v31 ^ 0x2400000) >> 22) & 0x3F]
                  | des_sbox_8[((2 * v30 | (v30 >> 31)) ^ 0xFFFFFFF5) & 0x3F])
            ^ v28;
        *(unsigned int*)&buffer[20] = v30;
        v33 = (des_sbox_1[(((v32 >> 1) | (v32 << 31)) ^ 0x60000000) >> 26]
                  | des_sbox_4[((((v32 >> 1) | (v32 << 31)) ^ 0x3800) >> 10) & 0x3F]
                  | des_sbox_7[((((v32 >> 1) | (v32 << 31)) >> 6) ^ 0xFFFFFFF6) & 0x3F]
                  | des_sbox_5[((((v32 >> 1) | (v32 << 31)) >> 2) ^ 0xFFFFFFE6) & 0x3F]
                  | des_sbox_2[((((v32 >> 1) | (v32 << 31)) >> 14) ^ 0xFFFFFFEA) & 0x3F]
                  | des_sbox_3[((((v32 >> 1) | (v32 << 31)) >> 22) ^ 0xFFFFFFE3) & 0x3F]
                  | des_sbox_6[((((v32 >> 1) | (v32 << 31)) ^ 0x240000) >> 18) & 0x3F]
                  | des_sbox_8[((2 * v32 | (v32 >> 31)) ^ 0xFFFFFFFD) & 0x3F])
            ^ v30;
        v34 = des_sbox_2[((((v33 >> 1) | (v33 << 31)) >> 14) ^ 0xFFFFFFF0) & 0x3F]
            | des_sbox_5[((((v33 >> 1) | (v33 << 31)) >> 2) ^ 0xFFFFFFE6) & 0x3F]
            | des_sbox_6[((((v33 >> 1) | (v33 << 31)) >> 18) ^ 0xFFFFFFF1) & 0x3F]
            | des_sbox_4[((((v33 >> 1) | (v33 << 31)) >> 10) ^ 0xFFFFFFE8) & 0x3F]
            | des_sbox_1[((((v33 >> 1) | (v33 << 31)) >> 26) ^ 0xFFFFFFEA) & 0x3F]
            | des_sbox_3[((((v33 >> 1) | (v33 << 31)) ^ 0x3400000) >> 22) & 0x3F];
        *(unsigned int*)&buffer[20] = v33;
        v35 = (des_sbox_7[((((v33 >> 1) | (v33 << 31)) >> 6) ^ 0xFFFFFFF5) & 0x3F] | v34
                  | des_sbox_8[((2 * v33 | (v33 >> 31)) ^ 0x17) & 0x3F])
            ^ v32;
        v36 = des_sbox_2[((((v35 >> 1) | (v35 << 31)) ^ 0x48000) >> 14) & 0x3F]
            | des_sbox_6[((((v35 >> 1) | (v35 << 31)) ^ 0x40000) >> 18) & 0x3F]
            | des_sbox_7[((((v35 >> 1) | (v35 << 31)) >> 6) ^ 0xFFFFFFF2) & 0x3F]
            | des_sbox_5[((((v35 >> 1) | (v35 << 31)) >> 2) ^ 0xFFFFFFEF) & 0x3F]
            | des_sbox_4[((((v35 >> 1) | (v35 << 31)) >> 10) ^ 0xFFFFFFEF) & 0x3F]
            | des_sbox_1[((((v35 >> 1) | (v35 << 31)) >> 26) ^ 0xFFFFFFE5) & 0x3F]
            | des_sbox_3[((((v35 >> 1) | (v35 << 31)) ^ 0x7400000) >> 22) & 0x3F]
            | des_sbox_8[((2 * v35 | (v35 >> 31)) ^ 0x12) & 0x3F];
        *(unsigned int*)&buffer[20] = v35;
        *(unsigned int*)&buffer[16] = v36 ^ v33;
        buffer[23] = v35;
        buffer[16] = (v36 ^ v33) >> 24;
        buffer[19] = v36 ^ v33;
        v37 = buffer[17];
        buffer[20] = BYTE3(v35);
        buffer[17] = (v36 ^ v33) >> 16;
        LOBYTE(v34) = buffer[21];
        buffer[21] = BYTE2(v35);
        buffer[18] = v37;
        buffer[22] = v34;
        *(unsigned int*)&buffer[8] = *(unsigned int*)&buffer[16];
        *(unsigned int*)&buffer[12] = *(unsigned int*)&buffer[20];
        *(unsigned int*)buffer = 0;
        *(unsigned int*)&buffer[4] = 0;
        current_buf_ptr = &buffer[16];
        index_1 = 0;
        do {
            v40 = 8 * ((*current_buf_ptr & 0xF) - (*current_buf_ptr >> 4)) + 128;
            byte_mask_1 = 8 * (index_1 + (*current_buf_ptr >> 4)) + unk_des_mask2;
            bits_1 = 8;
            v43 = buffer;
            do {
                BYTE tmp_byte = *byte_mask_1 | *(v40 + byte_mask_1);
                *v43++ |= tmp_byte;
                byte_mask_1 = byte_mask_1 + 1;
                --bits_1;
            } while (bits_1);
            index_1 += 32;
            ++current_buf_ptr;
        } while (index_1 < 256);
        index = 0;
        data_2 = data_1;
        *(output + (21 - length_1)) = buffer[0] ^ *data_1;
        memcpy(&result_buffer[12], &result_buffer[13], 7u);
        result_buffer[19] = *data_2;
        data_1 = data_2 + 1;
        --length_1;

    } while (length_1);
    return bits_1;
}

/*
 * Calculate CRC from input buffer.
 */
auto get_crc(BYTE* input, size_t size) -> unsigned int
{
    static int crc_lookup_table[1024] = {};
    static bool crc_lookup_table_initialized = false;

    if (!crc_lookup_table_initialized) {
        crc_lookup_table_initialized = true;

        auto index = 0;
        do {
            auto value = index << 24;
            auto bits = 8;
            do {
                if (value < 0) {
                    value = (2 * value) ^ 0x8001801B;
                } else {
                    value *= 2;
                }
                --bits;
            } while (bits);
            crc_lookup_table[index++] = value;
        } while (index < 256);
    }

    unsigned int result = 0;
    do {
        unsigned int index = (result >> 24) ^ *(unsigned char*)input++;
        result = crc_lookup_table[index] ^ (result << 8);
        --size;
    } while (size);
    return result;
}

/*
 * Get random hash from input buffer.
 */
auto get_random_hash(BYTE* input, size_t size) -> void
{
    const auto start_seed = 129;
    const auto multiplier = 0x343FD;
    const auto increment = 0x269EC3;

    auto seed = start_seed;
    for (auto i = 0; i < size; ++i) {
        seed = (seed * multiplier) + increment;
        *(input + i) = (seed >> 16) & 0xff;
    }
}

/*
 * Get plaintext from RSA decryption.
 */
auto calculate_plaintext_from_hwid(securom_game_t game, hwid_t hwid) -> std::string
{
    auto hwid_str = hwid.to_string();
    //hwid_str = "010BE04D5A000000000037210000";
    //hwid_str = "010BE04D5A009B00000037210000";

    std::cout << "[+] hwid m: " << hwid_str << std::endl;
    //_ASSERT(hwid_str.compare("010BE04D5A009B00000037210000") == 0);

    auto c = bdNew();
    auto m = bdNew();
    auto e = bdNew();
    auto n = bdNew();

    std::cout << "[+] modulus n: " << game.rsa_modulus_n << std::endl;
    std::cout << "[+] exponent e: " << game.rsa_exponent_e << std::endl;

    bdConvFromHex(n, game.rsa_modulus_n);
    bdConvFromHex(e, game.rsa_exponent_e);
    bdConvFromHex(m, hwid_str.c_str());

    // c = m^e mod n
    bdModExp(c, m, e, n);

    char plaintext[33];
    bdConvToHex(c, plaintext, sizeof(plaintext));

    bdFree(&c);
    bdFree(&m);
    bdFree(&e);
    bdFree(&n);

    std::cout << "[+] plaintext c: " << plaintext << std::endl;

    return std::string(plaintext);
}

/*
 * Fill plaintext from string length to 30 characters with the character 'f'.
 * Then append the original string length at the end.
 */
auto process_plaintext(std::string& plaintext) -> bool
{
    const auto max_plaintext_length = 30;

    auto plaintext_length = plaintext.length();
    auto offset = max_plaintext_length - plaintext_length;

    if ((offset & 0x80000000) != 0) {
        std::cout << "[-] invalid plaintext offset!" << std::endl;
        return false;
    }

    if (max_plaintext_length != plaintext_length) {
        do {
            plaintext += "f";
            --offset;
        } while (offset);
    }

    plaintext += std::format("{:02x}", plaintext_length);

    return true;
}

auto main() -> int
{
    auto start = std::time(0);

    auto game = tron_evolution;
    auto hwid = hwid_t();

    std::cout << std::format("[+] version_hash: {:02x}", hwid.version_hash) << std::endl;
    std::cout << std::format("[+] cpu_hash: {:02x}", _byteswap_ushort(hwid.cpu_hash)) << std::endl;
    std::cout << std::format("[+] gpu_hash: {:02x}", hwid.gpu_hash) << std::endl;
    std::cout << std::format("[+] network_hash: {:02x}", hwid.network_hash) << std::endl;
    std::cout << std::format("[+] disk_hash: {:02x}", _byteswap_ushort(hwid.disk_hash)) << std::endl;

    auto plaintext = calculate_plaintext_from_hwid(game, hwid);
    if (!process_plaintext(plaintext)) {
        return 1;
    }

    BYTE buf[33] = {};
    memcpy(buf, plaintext.c_str(), plaintext.length());

    BYTE sbuf[56] = { 0x00, 0x01, 0xC7, 0x22 };
    auto sbufPtr = sbuf + 4;

    convert_hex_to_bytes(sbufPtr + 5, buf, 32);

    for (auto i = 0; i < 16; ++i) {
        *(sbufPtr + 5 + i) ^= *(game.appid + i);
    }

    MD5_CTX ctx = {};
    MD5_Init(&ctx);
    MD5_Update(&ctx, game.appid, sizeof(game.appid));
    BYTE data[32] = {};
    MD5_Final(data, &ctx);

    auto dataPtr = data;
    xor_data(dataPtr, 16, sbufPtr, 2u);

    // sigXored = cblock
    BYTE sigXored[19] = {};
    for (auto i = 0; i < 16; ++i) {
        *(sigXored + i) = *(game.appid + i) ^ *(game.appid + i + (16 * 1)) ^ *(game.appid + i + (16 * 2));
    }
    memcpy(sigXored + 16, buf, 3); // TODO: buf is after sigXored/cblock -> one byte is used for xoring

    BYTE des_buffer[21] = {}; // TODO: 19?
    memcpy(des_buffer, sbufPtr + 2, sizeof(des_buffer));

    encrypt_with_des(sigXored, sbufPtr + 2, des_buffer, sizeof(des_buffer) - 2);

    auto rounds = 0x20000;

    // TODO: remove ueselss copy and save some memory
    BYTE des_buffer2[64] = { *(sbufPtr + 0), *(sbufPtr + 1) };
    memcpy(des_buffer2 + 2, des_buffer, 19);

    do {
        for (auto i = 0; i < 8; ++i) {
            des_encrypt_with_sbox(des_buffer2, sbufPtr, 21);
            des_encrypt_with_sbox(sbufPtr, des_buffer2, 21);
        }
        --rounds;
    } while (rounds);

    auto crc = get_crc(sbuf + 1, 24);

    xor_data((BYTE*)&crc, 4, sbuf, 1);

    BYTE seed[17] = {};
    get_random_hash(seed, sizeof(seed) - 1);

    // TODO: Like above cblock should always be 16 bytes but encrypt_with_des hashes it again.
    //       However, for some reason they also use one additional byte at the end from the stack.
    memcpy(seed + 16, sigXored, 1);

    BYTE data2[32] = {};
    encrypt_with_des(seed, sbuf, data2, 25);

    BYTE v37[40] = {};
    figure_out_what_this_does_4(data2, 25, v37, sizeof(v37));

    decode_to_ascii(v37, des_buffer2, sizeof(v37));

    char unlock_code_buffer[64] = {};
    insert_hyphens((char*)des_buffer2, unlock_code_buffer, 40, 5);

    print_buffer_ascii("[+] unlock code: ", unlock_code_buffer, sizeof(unlock_code_buffer));

    _ASSERT(strcmp(unlock_code_buffer, "UB9NU-ZU3LF-6R5ZQ-LPYE5-8C3UH-RGR3P-HX5NP-P6GHQ") == 0);

    std::cout << "[+] generated code in " << std::difftime(std::time(0), start) << " seconds" << std::endl;
    std::cout << "[+] done" << std::endl;

    return 0;
}
