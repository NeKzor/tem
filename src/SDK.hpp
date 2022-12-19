/*
 * Copyright (c) 2022 NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once
#include <math.h>
#include <string>

#define PG_PAWN 0x197B
#define PLAYER_CONTROLLER 0xAAF
#define PG_PLAYER_CONTROLLER 0x1A75
#define TICK 0x150
#define DESTROYED 0x12D
#define POST_INIT_ANIM_TREE 0x1DEF
#define CONSOLE 0x2FE
#define SET_CAMERA_TARGET_TIMER 0x841E

struct FNameEntry {
    int unk0; // 0
    int unk1; // 4
    int index; // 8
    int unk2; // 12
    char name[1024]; // 16
};

struct FName {
    unsigned int index; // 0
    unsigned int number; // 4
};

struct UClass;

struct UObject {
    void* vtable; // 0
    char unk0[36]; // 4
    UObject* outer_object; // 40
    FName name; // 44
    UClass* class_object; // 52
    UObject* object_archetype; //56

    inline auto is(unsigned int index) -> bool { return this->name.index == index; }
    template <typename T> inline auto as() -> T* { return reinterpret_cast<T*>(this); }
};

#define foreach_item(item, array)                                                                                      \
    for (auto [i, item] = std::tuple{ 0u, array[0] }; i < array.size; ++i, item = i < array.size ? array[i] : item)

template <typename T> struct TArray {
    T* data; // 0
    unsigned int size; // 4
    unsigned int max; // 8

    inline auto operator[](unsigned int index) -> T { return this->at(index); }
    inline auto at(unsigned int index) -> T
    {
        if (this->data && index < this->size) {
            return this->data[index];
        } else {
            return T();
        }
    }
};

struct FString : TArray<wchar_t> {
    FString();
    FString(const wchar_t* text);
    FString(const std::wstring& text);
    inline auto c_str() -> const wchar_t* { return this->data; }
    inline auto c_str() const -> const wchar_t* { return this->data; }
    inline auto wstr() -> std::wstring { return std::wstring(this->data); }
    inline auto wstr() const -> std::wstring { return std::wstring(this->data); }
    auto str() -> std::string;
    auto str() const -> std::string;
};

auto wchar_to_utf8(const wchar_t* data) -> std::string;

struct Vector2 {
    float x; // 0
    float y; // 4

    inline auto length() -> float { return sqrtf((this->x * this->x) + (this->y * this->y)); }
};

struct Vector3 {
    float x; // 0
    float y; // 4
    float z; // 8

    inline auto length() -> float { return sqrtf((this->x * this->x) + (this->y * this->y) + (this->z * this->z)); }
    inline auto length_2d() -> float { return sqrtf((this->x * this->x) + (this->y * this->y)); }
};

struct Rotation {
    unsigned short value; // 0
    unsigned short sign; // 2

    inline auto degree() -> float { return (this->value / float(0xffffui16)) * 360.0f; }
};

struct Color {
    float r; // 0
    float g; // 4
    float b; // 8
    float a; // 12
    Color(int r, int g, int b, int a = 255)
        : r(r / 255.0f)
        , g(g / 255.0f)
        , b(b / 255.0f)
        , a(a / 255.0f)
    {
    }
};

struct FURL : FString {
    FString host; // 0x0C
    int port; // 0x18
    FString map; // 0x1C
    TArray<FString> op; // 0x28
    FString portal; // 0x34
    int valid; // 0x40
};

struct PgTeamInfo {
    char unk0[504];
    float score; // 504
    int team_index; // 508
    Color team_color; // 512
};

struct PgPlayerReplicationInfo {
    char unk0[532]; // 0
    PgTeamInfo* team; // 532
};

struct PgPawn {
    char unk0[84]; // 0
    Vector3 position; // 84
    char unk1[148]; // 96
    float timer; // 244
    char unk2[68]; // 248
    Vector3 velocity; // 316
    char unk3[176]; // 328
    PgPawn* next_pawn; // 504
    char unk4[232]; // 508
    int health; // 740
    int max_health; // 744
    int least_health; // 748
    int least_max_health; // 752
    char unk5[88]; // 756
    PgPlayerReplicationInfo* replication_info; // 844
    char unk6[72]; // 848
    PgPawn* driven_vehicle; // 920
    char unk7[28]; // 924
    Rotation rotation; // 952
    char unk8[276]; // 956
    int energy; // 1232
    int max_energy; // 1236

    inline auto is_in_team(PgTeamInfo* team) -> bool
    {
        return this->replication_info && this->replication_info->team == team;
    }
    inline auto equals(PgPawn* other) -> bool { return this == other; }
    inline auto is_vehicle() -> bool
    {
        // TODO: Verify if this works when the tank bug below happens
        return this->get_outer_pawn() != this;
        //return this->next_pawn && this->next_pawn->driven_vehicle == this;
    }
    inline auto get_outer_pawn() -> PgPawn*
    {
        // Player pawn should always be last I think.
        // For some reason when driving a tank it's possible to spawn it multiple times
        // which doubles its sound effect even when it looks like you drive one vehicle.
        auto outer = this;
        while (outer) {
            if (!outer->next_pawn) {
                break;
            }
            outer = outer->next_pawn;
        }
        return outer;
    }
};

struct FAutoCompleteCommand {
    FString command; // 0
    FString description; // 12
};

struct UConsole {
    char unk0[140]; // 0
    FName console_key; // 0x8c
    FName type_key; // 0x94
    char unk1[248];
    TArray<FAutoCompleteCommand> manual_auto_complete_list; // 0x194
    TArray<FAutoCompleteCommand> auto_complete_list; // 0x1a0
};

struct UGameViewportClient {
    char unk0[96]; // 0
    UConsole* viewport_console; // 0x60
};

enum ETransitionType {
    TT_None = 0,
    TT_Paused = 1,
    TT_Loading = 2,
    TT_Saving = 3,
    TT_Connecting = 4,
    TT_Precaching = 5,
    TT_MAX = 6,
};

struct FRotator {
    int paitch;
    int yaw;
    int roll;
};

struct AGameInfo {
    char unk0[612];
    FString default_player_name; // 0x264
    FString game_name; // 0x270
};

struct AWorldInfo {
    char unk0[228];
    PgPawn* pawn_list; // 0xe4
    char unk1[652];
    float time; // 0x374
    float real_time; // 0x378
    float audio_time; // 0x37c
    float delta; // 0x380
    char unk2[72];
    UObject* gri; // 0x3cc
    char unk3[4];
    FString computer_name; // 0x3d4
    FString engine_version; // 0x3e0
    FString min_net_version; // 0x3fc
    AGameInfo* game; // 0x3f8
    char unk4[4];
    float gravity; // 0x400
    float default_gravity; // 0x404
    char unk5[16];
    PgPawn* pawn_list2; // 0x418
};

struct PgCheatManager {
    char unk0[64];
    UObject* debug_camera_controller; // 0x40
    char unk1[44];
    UObject* anim_behaviour_conditional; // 0x70
    UObject* condition_playing_anim; // 0x74
    char unk2[16];
    UObject* transient; // 0x88
    char unk3[8];
    UObject* data_provider_online_players; // 0x94
    UObject* data_provider_online_players2; // 0x98
    char unk4[4];
    UObject* data_store_online_players; // 0xa0
    char unk5[20];
    UObject* local_player; // 0xb8
};

struct FKeyBind {
    FName name; // 0
    FString command; // 8
    int control : 1; // 20
    int shift : 1; // 20
    int alt : 1; // 20
    int ignore_ctrl : 1; // 20
    int ignore_shift : 1; // 20
    int ignore_alt : 1; // 20
};

struct PgPlayerInput {
    char unk0[108];
    TArray<FKeyBind> bindings; // 0x6c
    TArray<FName> pressed_keys; // 0x78
};

struct FRenderCommandFence {
    int num_pending_fences;
};

struct PgHud {
    char pad_0000[152]; // 0x00
    UObject* player_owner; // 0x98
    char pad_009C[76]; // 0x9C
    UObject* world_info; // 0xE8
    char pad_00EC[76]; // 0xEC
    UObject* default_physics_volume; // 0x138
    char pad_013C[128]; // 0x13C
    UObject* local_message; // 0x1BC
    UObject** seq_event_touches; // 0x1C0
    char pad_01C4[44]; // 0x1C4
    UObject* player_controller; // 0x1F0
    char pad_01F4[4]; // 0x1F4
    UObject* replication_info; // 0x1F8
    char pad_01FC[68]; // 0x1FC
};

struct DLCItem : FName {
    FString dlc_key; // 0x08
    TArray<FString> live_conent; // 0x14
    int live_license_flag; // 0x20
    TArray<FString> dlc_content; // 0x24
};

struct PgDLCUtils : UObject {
    TArray<DLCItem> dlc_items; // 0x3C
    FString ps3_dlc_folder; // 0x48
};

struct PgUnlockItemPlayerSkin : UObject {
    char pad_003C[132]; // 0x3C
    int skin_index; // 0xC0
    char pad_00C4[28]; // 0xC4
};

struct PgMPLoadoutBuilder : UObject {
    char pad_003C[48]; // 0x3C
};

struct PgUnlockItemDiscPower : UObject {
    char pad_003C[144]; // 0x3C
};

struct PgUnlockItemPlayerUpgrade : UObject {
    char pad_003C[256]; // 0x3C
};

struct PgStoryLoadoutBuilder : UObject {
    TArray<PgUnlockItemDiscPower*> inventory_items; // 0x3C
    TArray<PgUnlockItemPlayerUpgrade*> upgrades; // 0x48
};

struct PgUnlockSystem : UObject {
    TArray<PgUnlockItemPlayerSkin*> all_items; // 0x3C
    int num_loadout_slots; // 0x48
    int current_loadout; // 0x4C
    PgMPLoadoutBuilder* loadout_builder; // 0x50
    PgStoryLoadoutBuilder* story_builder; // 0x54
    int max_loadouts; // 0x58
    char pad_005C[24]; // 0x5C
    FString login_options; // 0x74
    FString override_connection_string; // 0x80
};

#define PLAYER_CONTROLLER_FLAGS__GOD_MODE (1 << 1)

struct PgPlayerController {
    char unk0[96];
    FRotator rotation; // 0x60
    float draw_scale; // 0x6c
    Vector3 draw_scale_3d; // 0x70
    Vector3 pre_pivot; // 0x7c
    FRenderCommandFence detach_fence; // 0x88
    float custom_time_dilation; // 0x8c
    char unk1[88];
    AWorldInfo* world_info; // 0xe8
    char unk2[56];
    TArray<PgPlayerReplicationInfo*> replication_infos; // 0x124
    char unk3[8];
    UObject* physics_volume; // 0x138
    char unk4[92];
    UObject* cylinder_component; // 0x198
    char unk5[32];
    UObject* local_messages; // 0x1bc
    TArray<UObject*> spline_actors; // 0x1c0
    char unk6[24];
    PgPawn* pawn; // 0x1e4
    PgPlayerReplicationInfo* replication_info; // 0x1e8
    char unk7[8];
    int flags; // 0x1f4
    char unk8[8];
    UObject* navigation_handle_class; // 0x200
    UObject* navigation_handle; // 0x204
    char unk9[184];
    UObject* player_start; // 0x2c0
    char unk10[136];
    PgPawn* enemy; // 0x34c
    char unk11[28];
    UObject* player; // 0x36c
    UObject* player_camera; // 0x370
    UObject* player_camera_class; // 0x374
    UObject* player_owner_data_store; // 0x378
    UObject* player_owner_data_store_class; // 0x37c
    char unk12[12];
    PgPawn* acknowledge_pawn; // 0x38c
    char unk13[8];
    PgPawn* view_target; // 0x398
    char unk14[52];
    PgHud* hud; // 0x3d0
    UObject* saved_move; // 0x3d4
    char unk15[184];
    PgCheatManager* cheat_manager; // 0x490
    UObject* cheat_manager_class; // 0x494
    PgPlayerInput* player_input; // 0x498
    UObject* player_input_class; // 0x49c
    char unk16[12];
    UObject* cylinder_component2; // 0x4ac
    char unk17[12];
    UObject* force_feedback_manager; // 0x4bc
    TArray<PgPlayerInput*> player_inputs; // 0x4c0
    char unk18[260];
    UObject* player_cam; // 0x5d0
    PgPawn* pawn4; // 0x5d4
    char unk19[364]; // 0x5d8
    PgUnlockSystem* unlock_system; // 0x744

    inline auto set_god_mode(bool on) -> void
    {
        if (on) {
            this->flags |= PLAYER_CONTROLLER_FLAGS__GOD_MODE;
        } else {
            this->flags &= ~(PLAYER_CONTROLLER_FLAGS__GOD_MODE);
        }
    }
};

struct ULocalPlayer {
    char unk0[64];
    PgPlayerController* actor; // 0x40
    char unk1[32];
    UGameViewportClient* viewport_client; // 0x64
};

#define ENGINE_CONFIG__ON_SCREEN_KISMET_WARNINGS 1 << 13

struct UEngine {
    struct UEngine_vtable* vtable; // 0
    char unk0[728];
    int config; // 0x2dc
    char unk1[388];
    uintptr_t client; // 0x464
    TArray<ULocalPlayer*> game_players; // 0x468
    UGameViewportClient* viewport_client; // 0x474
    char unk2[12];
    float tick_cycles; // 0x484
    float game_cycles; // 0x488
    float client_cycles; // 0x48C
    char pad_0490[84]; // 0x490
    ETransitionType transition_type; // 0x4E4
    FString transition_description; // 0x4E8
    FString transition_game_type; // 0x4F4
    char pad_04F8[52]; // 0x4F8
    UObject* terrain_collision_material; // 0x534
    char pad_0538[204]; // 0x538
    FURL last_url; // 0x604
    char pad_0648[92]; // 0x648
    int travel_type; // 0x6A4
    char pad_06A8[4]; // 0x6A8
    UObject* online_subsystems; // 0x6AC

    inline auto get_local_player() -> ULocalPlayer* { return this->game_players.at(0); }
    inline auto is_paused() -> bool { return this->is_in_transition(TT_Paused); }
    inline auto is_in_transition(ETransitionType type) -> bool { return this->transition_type == type; }
    auto get_level_name() -> const wchar_t*;
};

struct UField : UObject {
    UField* super_field; // 0x3C
    UField* next; // 0x40
};

struct UStruct : UField {
    char pad_0044[8]; // 0x44
    UField* children; // 0x4C
    int property_size; // 0x50
    char pad_0054[60]; // 0x54
};

struct UFunction : UStruct {
    int function_flags; // 0x90
    uint16_t i_native; // 0x94
    uint16_t rep_offset; // 0x96
    FName friendly_name; // 0x98
    uint16_t num_params; // 0xA0
    uint16_t params_size; // 0xA2
    int return_value_offset; // 0xA4
    int padding; // 0xA8
    void* func; // 0xAC

    inline auto is(unsigned int index) -> bool { return this->name.index == index; }
};

struct UState : UStruct {
    char pad_0090[84]; // 0x90
};

struct UClass : UState {
    char pad_00E4[248]; // 0xE4
};

struct UEnum : UField {
    TArray<FName> names; // 0x44
};

struct UConst : UField {
    FString value; // 0x44
};

struct UScriptStruct : UStruct {
    char pad_0090[28]; // 0x90
};

struct UProperty : UField {
    int array_dim; // 0x44
    int element_size; // 0x48
    int property_flags; // 0x4C
    int property_size; // 0x50
    char pad_0054[16]; // 0x54
    int offset; // 0x64
    char pad_0064[28]; // 0x68
};

struct UByteProperty : UProperty {
    UEnum* enum_object; // 0x84
};

struct UIntProperty : UProperty {};

struct UFloatProperty : UProperty {};

struct UBoolProperty : UProperty {
    int bit_mask; // 0x84
};

struct UStrProperty : UProperty {};

struct UNameProperty : UProperty {};

struct UDelegateProperty : UProperty {
    char pad_0084[8]; // 0x84
};

struct UObjectProperty : UProperty {
    UStruct* property_class; // 0x84
};

struct UClassProperty : UProperty {
    UStruct* meta_class; // 0x84
    char pad_0088[4]; // 0x88
};

struct UInterfaceProperty : UProperty {
    UStruct* interface_class; // 0x84
};

struct UStructProperty : UProperty {
    UStruct* property_struct; // 0x84
};

struct UArrayProperty : UProperty {
    UProperty* inner; // 0x84
};

struct UMapProperty : UProperty {
    UProperty* key; // 0x84
    UProperty* value; // 0x88
};

struct UComponentProperty : UProperty {
    UObject* component; // 0x84
};

template <typename K = void*, typename V = void*>
struct FPair {
    K key;
    V value;
    int unk0;
    int unk1;
};

template <typename T>
struct TMap {
    TArray<T> value; // 0x00
    char unk0[48]; // 0x12
};
