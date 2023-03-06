#include "Tests.hpp"
#include "Unlocker.hpp"
#include <iostream>

std::vector<UnitTest*>& UnitTest::test_cases()
{
    static std::vector<UnitTest*> list;
    return list;
}

UnitTest::UnitTest(const char* case_name, const char* description, _UnitTestFunction callback)
    : case_name(case_name)
    , description(description)
    , callback(callback)
{
    UnitTest::test_cases().push_back(this);
}

auto buffer_to_hex_string(BYTE* buffer, size_t length) -> std::string
{
    auto hex = std::string("");
    while (length--) {
        hex += std::format("{:02X}", *buffer++);
    }
    return hex;
}

#define to_hex(buffer) buffer_to_hex_string(buffer, sizeof(buffer))

TEST(hwid_t_from_and_to_string, correct_hwid)
{
    auto hwid = hwid_t::from_string("010BE04D5A009B00000037210000");
    auto hwid_str = hwid.to_string();

    EXPECT_EQ(hwid_str, "010BE04D5A009B00000037210000");
}

TEST(calculate_plaintext_from_hwid, correct_plaintext)
{
    auto game = tron_evolution;
    auto hwid = hwid_t::from_string("010BE04D5A009B00000037210000");

    auto plaintext = calculate_plaintext_from_hwid(game, hwid);

    EXPECT_EQ(plaintext, "f29770cb34be7418ec656bba8dae2");

    auto processed_plaintext = process_plaintext(plaintext);

    EXPECT_TRUE(processed_plaintext);
    EXPECT_EQ(plaintext, "f29770cb34be7418ec656bba8dae2f1d");
}

TEST(convert_hex_to_bytes, correct_conversion)
{
    auto plaintext = std::string("f29770cb34be7418ec656bba8dae2f1d");

    BYTE data_buffer[56] = { 0x00, 0x01, 0xC7, 0x22 };
    auto data_buffer_ptr = data_buffer + 4;

    convert_hex_to_bytes(data_buffer_ptr + 5, (BYTE*)plaintext.c_str(), 32);

    auto data_buffer_hex = to_hex(data_buffer);
    // clang-format off
    EXPECT_EQ(data_buffer_hex, "0001C7220000000000F29770CB34BE7418EC656BBA8DAE2F1D00000000000000000000000000000000000000000000000000000000000000");
    // clang-format on
}

TEST(des_encrypt_with_sbox, corret_encryption)
{
    BYTE output_buffer[] = { 0x48, 0x02, 0x22, 0xAC, 0x0F, 0xD8, 0x5C, 0x56, 0xB3, 0x77, 0x90, 0x85, 0xD1, 0x8E, 0x60,
        0xC3, 0xC3, 0xA2, 0x13, 0x3D, 0xB7 };
    BYTE data_buffer_ptr[] = { 0x48, 0x02, 0x00, 0x00, 0x00, 0x0B, 0x14, 0x0A, 0xD6, 0x16, 0x91, 0x10, 0x6C, 0xC4, 0xAE,
        0x78, 0x8A, 0xBF, 0x7E, 0xFF, 0x11 };

    des_encrypt_with_sbox(output_buffer, data_buffer_ptr, 21);

    auto output_buffer_hex = to_hex(output_buffer);
    EXPECT_EQ(output_buffer_hex, "480222AC0FD85C56B3779085D18E60C3C3A2133DB7");

    auto data_buffer_ptr_hex = to_hex(data_buffer_ptr);
    EXPECT_EQ(data_buffer_ptr_hex, "65EEA820CA7E05D996D43B670132072AC088075DC9");
}

TEST(get_crc, corret_crc_value)
{
    BYTE data_buffer[56] = { 0x00, 0x01, 0xC7, 0x22, 0x65, 0xA1, 0xE0, 0xAA, 0x94, 0xB1, 0x9A, 0xCC, 0xD9, 0x72, 0x11,
        0x8F, 0x68, 0x21, 0x18, 0x65, 0x25, 0x7A, 0x75, 0x22, 0xEC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00 };

    auto crc = get_crc(data_buffer + 1, 24);

    EXPECT_EQ(crc, 0xDD17A846);
}

TEST(xor_data, corret_buffer)
{
    BYTE data_buffer[56] = { 0x00, 0x01, 0xC7, 0x22, 0x65, 0xA1, 0xE0, 0xAA, 0x94, 0xB1, 0x9A, 0xCC, 0xD9, 0x72, 0x11,
        0x8F, 0x68, 0x21, 0x18, 0x65, 0x25, 0x7A, 0x75, 0x22, 0xEC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00 };
    BYTE crc[4] = { 0x46, 0xA8, 0x17, 0xDD };

    xor_data(crc, 4, data_buffer, 1);

    auto data_buffer_hex = to_hex(data_buffer);
    // clang-format off
    EXPECT_EQ(data_buffer_hex, "2401C72265A1E0AA94B19ACCD972118F68211865257A7522EC00000000000000000000000000000000000000000000000000000000000000");
    // clang-format on
}

TEST(get_random_hash, corret_hash)
{
    BYTE cblock_seed[17] = {};

    get_random_hash(cblock_seed, sizeof(cblock_seed) - 1);

    auto cblock_seed_hex = to_hex(cblock_seed);
    EXPECT_EQ(cblock_seed_hex, "CB57D42C2BC90F0B1D8755F761E5FEF600");
}

TEST(encrypt_with_des, corret_encryption)
{
    BYTE cblock_seed[17]
        = { 0xCB, 0x57, 0xD4, 0x2C, 0x2B, 0xC9, 0x0F, 0x0B, 0x1D, 0x87, 0x55, 0xF7, 0x61, 0xE5, 0xFE, 0xF6, 0xB5 };
    BYTE data_buffer[56] = { 0x24, 0x01, 0xC7, 0x22, 0x65, 0xA1, 0xE0, 0xAA, 0x94, 0xB1, 0x9A, 0xCC, 0xD9, 0x72, 0x11,
        0x8F, 0x68, 0x21, 0x18, 0x65, 0x25, 0x7A, 0x75, 0x22, 0xEC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00 };
    BYTE unlock_code_des_buffer[25] = {};

    encrypt_with_des(cblock_seed, data_buffer, unlock_code_des_buffer, 25);

    auto unlock_code_des_buffer_hex = to_hex(unlock_code_des_buffer);
    EXPECT_EQ(unlock_code_des_buffer_hex, "3A1DAABF0EB2913BBE95D5B36154D0EFBA1BEAEB83D64ADCB3");
}

TEST(decode_des_buffer, corret_decoding)
{
    BYTE unlock_code_des_buffer[32] = { 0x3A, 0x1D, 0xAA, 0xBF, 0x0E, 0xB2, 0x91, 0x3B, 0xBE, 0x95, 0xD5, 0xB3, 0x61,
        0x54, 0xD0, 0xEF, 0xBA, 0x1B, 0xEA, 0xEB, 0x83, 0xD6, 0x4A, 0xDC, 0xB3 };
    BYTE unlock_code_hex_buffer[40] = {};

    decode_des_buffer(unlock_code_des_buffer, 25, unlock_code_hex_buffer, sizeof(unlock_code_hex_buffer));

    auto unlock_code_hex_buffer_hex = to_hex(unlock_code_hex_buffer);
    EXPECT_EQ(
        unlock_code_hex_buffer_hex, "1A0907141A1F1A01120D0417031F1612151E0C03060A011A0F170E1701150F1D03141515040E0F16");
}

TEST(decode_to_ascii, corret_decoding)
{
    BYTE unlock_code_hex_buffer[40] = { 0x1A, 0x09, 0x07, 0x14, 0x1A, 0x1F, 0x1A, 0x01, 0x12, 0x0D, 0x04, 0x17, 0x03,
        0x1F, 0x16, 0x12, 0x15, 0x1E, 0x0C, 0x03, 0x06, 0x0A, 0x01, 0x1A, 0x0F, 0x17, 0x0E, 0x17, 0x01, 0x15, 0x0F,
        0x1D, 0x03, 0x14, 0x15, 0x15, 0x04, 0x0E, 0x0F, 0x16 };
    BYTE output_buffer[64] = {};

    decode_to_ascii(unlock_code_hex_buffer, output_buffer, sizeof(unlock_code_hex_buffer));

    auto output_buffer_hex = to_hex(output_buffer);
    // clang-format off
    EXPECT_EQ(output_buffer_hex, "5542394E555A55334C463652355A514C50594535384333554852475233504858354E505036474851000000000000000000000000000000000000000000000000");
    // clang-format on
}

TEST(insert_hyphens, corret_code)
{
    BYTE output_buffer[64] = { 0x55, 0x42, 0x39, 0x4E, 0x55, 0x5A, 0x55, 0x33, 0x4C, 0x46, 0x36, 0x52, 0x35, 0x5A, 0x51,
        0x4C, 0x50, 0x59, 0x45, 0x35, 0x38, 0x43, 0x33, 0x55, 0x48, 0x52, 0x47, 0x52, 0x33, 0x50, 0x48, 0x58, 0x35,
        0x4E, 0x50, 0x50, 0x36, 0x47, 0x48, 0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    char unlock_code_buffer[64] = {};

    insert_hyphens((char*)output_buffer, unlock_code_buffer, 40, 5);

    auto unlock_code = std::string(unlock_code_buffer);
    EXPECT_EQ(unlock_code, "UB9NU-ZU3LF-6R5ZQ-LPYE5-8C3UH-RGR3P-HX5NP-P6GHQ");
}

auto run_all_tests() -> int
{
    auto succeeded = 0;
    auto failed = 0;

    std::cout << "running " << UnitTest::test_cases().size() << " test cases..." << std::endl << std::endl;

    for (auto& test_case : UnitTest::test_cases()) {
        try {
            test_case->callback(test_case);
            std::cout << std::format("[succeeded] {} - {}", test_case->case_name, test_case->description) << std::endl
                      << std::endl;
            ++succeeded;
        } catch (std::exception ex) {
            std::cout << ex.what() << std::endl << std::endl;
            ++failed;
        }
    }

    std::cout << "completed all tests: " << std::endl;
    std::cout << " - succeeded: " << succeeded << std::endl;
    std::cout << " - failed: " << failed << std::endl;

    return failed == 0 ? 0 : 1;
}
