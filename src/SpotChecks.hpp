/*
 * Copyright (c) 2022-2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include "Offsets.hpp"
#include "SDK.hpp"
#include <cstdint>

typedef uint32_t SpotCheckId;

enum class SpotCheckKey : SpotCheckId {
    ChangeVersion,
    DelayInputs,
    RemoveWeapon,
    Unknown3,
    CorruptSaveFile,
    FirstThreeMapsOnly,
    DisableUnpause,
    DisableXP,
    Unknown8,
    DoNotLaunchTheGame,
    Unknown10,
    MAX,
};

enum class SpotCheckResult {
    Ok,
    Err,
};

struct SpotCheck {
    SpotCheckResult result;
};

using SpotCheckPair = FPair<SpotCheckKey, SpotCheck>;
using SpotCheckMap = TMap<SpotCheckPair>;

extern auto bypass_spot_checks() -> void;
