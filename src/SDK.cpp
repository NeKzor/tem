/*
 * Copyright (c) 2022 NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#include "SDK.hpp"
#include <windows.h>

FString::FString()
{
    this->data = nullptr;
    this->size = 0;
    this->max = 0;
}
FString::FString(const wchar_t* text)
{
    this->size = text && *text ? std::wcslen(text) + 1 : 0;
    this->max = this->size;

    if (this->size) {
        this->data = const_cast<wchar_t*>(text);
    }
}
FString::FString(const std::wstring& text)
    : FString(text.c_str())
{
}
auto FString::str() -> std::string { return wchar_to_utf8(this->data); }
auto FString::str() const -> std::string { return wchar_to_utf8(this->data); }
auto wchar_to_utf8(const wchar_t* data) -> std::string
{
    auto size = data && *data ? std::wcslen(data) + 1 : 0;
    if (!size) {
        return "";
    }

    auto string_size = WideCharToMultiByte(CP_UTF8, 0, &data[0], size, NULL, 0, NULL, NULL);
    auto string = std::string(string_size, 0);
    WideCharToMultiByte(CP_UTF8, 0, &data[0], size, &string[0], string_size, NULL, NULL);
    string.erase(std::find(string.begin(), string.end(), '\0'), string.end());
    return string;
}

auto UEngine::get_level_name() -> const wchar_t*
{
    if (lstrcmpW(this->last_url.map.c_str(), L"TronGame_P") == 0) {
        foreach_item(item, this->last_url.op)
        {
            if (wcsstr(item.c_str(), L"chapter=") != 0) {
                return item.c_str() + std::wcslen(L"chapter=");
            }
        }
    }

    return this->last_url.map.c_str();
}
