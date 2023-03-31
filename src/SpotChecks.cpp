/*
 * Copyright (c) 2022-2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#include "SpotChecks.hpp"
#include "Console.hpp"
#include <vector>

#define decl_spot_check_ok(_key)                                                                                       \
    {                                                                                                                  \
        .key = _key, .value = { .result = SpotCheckResult::Ok }, .unk0 = -1, .unk1 = int(_key)                         \
    }

static std::vector<SpotCheckPair> fixed_spot_check_data = {
    decl_spot_check_ok(SpotCheckKey::ChangeVersion),
    decl_spot_check_ok(SpotCheckKey::DelayInputs),
    decl_spot_check_ok(SpotCheckKey::RemoveWeapon),
    decl_spot_check_ok(SpotCheckKey::DisableTargetSelector),
    decl_spot_check_ok(SpotCheckKey::CorruptSaveFile),
    decl_spot_check_ok(SpotCheckKey::FirstThreeMapsOnly),
    decl_spot_check_ok(SpotCheckKey::DisableUnpause),
    decl_spot_check_ok(SpotCheckKey::DisableXP),
    decl_spot_check_ok(SpotCheckKey::UnknownKey8),
    decl_spot_check_ok(SpotCheckKey::DoNotLaunchTheGame),
    decl_spot_check_ok(SpotCheckKey::UnusedKey10),
};

auto bypass_spot_checks() -> void
{
    auto spot_check_map = reinterpret_cast<SpotCheckMap*>(Offsets::spot_check_map);

    // Insertion has to be done before the game inserts them itself
    // as bad code could already be executed at this point.

    if (spot_check_map->value.size != 0) {
        return println("[spot checks] Unable to bypass spot checks");
    }

    using _spot_check_map_insert = int(__thiscall*)(void* map, void* key, void* value, void* found);
    auto spot_check_map_insert = reinterpret_cast<_spot_check_map_insert>(Offsets::spot_check_map_insert);

    for (auto& spot_check : fixed_spot_check_data) {
        spot_check_map_insert(spot_check_map, &spot_check.key, &spot_check, nullptr);
    }

    println("[spot check] Bypassed {} spot checks", fixed_spot_check_data.size());
}
