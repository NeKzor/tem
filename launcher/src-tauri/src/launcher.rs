/*
 * Copyright (c) 2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

use serde::{Deserialize, Serialize};
use tauri::{Window, AppHandle, Manager};

pub struct AppState {
    pub game_install_path: String,
    pub game_config_path: String,
}

#[derive(Serialize, Deserialize, Debug)]
pub enum SortOptionDirection {
    #[serde(rename = "asc")]
    Ascending,
    #[serde(rename = "desc")]
    Descending,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct SortOption {
    pub key: String,
    pub direction: SortOptionDirection,
}

#[derive(Serialize, Deserialize, Debug)]
pub enum SortOrderOption {
    #[serde(rename = "createdAt-asc")]
    CreatedAtAsc,
    #[serde(rename = "createdAt-desc")]
    CreatedAtDesc,
    #[serde(rename = "name-asc")]
    NameAscending,
    #[serde(rename = "name-desc")]
    NameDescending,
}

#[derive(Serialize, Deserialize, Debug)]
pub enum CheckForUpdatesOption {
    #[serde(rename = "disabled")]
    Disabled,
    #[serde(rename = "on-launcher-start")]
    OnLauncherStart,
    #[serde(rename = "on-launcher-exit")]
    OnLauncherExit,
}

#[derive(Serialize, Deserialize, Debug)]
#[serde(rename_all = "camelCase")]
pub struct AppConfig {
    pub configs: Vec<LauncherConfig>,
    pub sort: SortOption,
    pub check_for_updates: CheckForUpdatesOption,
}

#[derive(Serialize, Deserialize, Debug)]
#[serde(rename_all = "camelCase")]
pub struct LauncherConfig {
    pub name: String,
    pub created_at: i64,
    pub modified_at: i64,
    pub window_width: i32,
    pub window_height: i32,
    pub is_fullscreen: bool,
    pub disable_splash_screen: bool,
    pub is_default: bool,
    #[serde(rename = "useTEM")]
    pub use_tem: bool,
    #[serde(rename = "useXDead")]
    pub use_xdead: bool,
}

pub const GAME_EXE: &str = "GridGame.exe";
pub const SECUROM_BUFFER_SIZE: usize = 1723;

pub fn launch_game(window: &Window, app: &AppHandle) {
    use windows::core::{PCSTR, PWSTR};
    use windows::Win32::Foundation::*;
    use windows::Win32::System::Memory::*;
    use windows::Win32::System::Threading::*;
    use std::os::windows::ffi::OsStrExt;

    let state = app.state::<AppState>();

    log!(
        window,
        "Game is installed in {:#?}",
        state.game_install_path
    );

    let file = format!("-=[SMS_{GAME_EXE}_SMS]=-\0");

    let result = unsafe {
        CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            None,
            PAGE_READWRITE,
            0,
            SECUROM_BUFFER_SIZE as u32,
            PCSTR(file.as_ptr()),
        )
    };

    let Ok(handle) = result else {
        log!(window, "Could not create file mapping object {:#?}", result.err());
        return;
    };

    let result = unsafe { MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, SECUROM_BUFFER_SIZE) };

    if result.is_ok() {
        log!(window, "Created file mapping");

        let cmd = format!("{}\\{GAME_EXE}", state.game_install_path);
        log!(window, "Launching {cmd}");

        let si = STARTUPINFOW {
            cb: std::mem::size_of::<STARTUPINFOW>() as u32,
            ..Default::default()
        };
        let mut pi = PROCESS_INFORMATION::default();

        let mut cmd = std::ffi::OsString::from(cmd)
            .encode_wide()
            .chain(Some(0))
            .collect::<Vec<u16>>();

        unsafe {
            CreateProcessW(
                None,
                PWSTR(cmd.as_mut_ptr()),
                None,
                None,
                FALSE,
                PROCESS_CREATION_FLAGS(0u32),
                None,
                None,
                &si,
                &mut pi,
            );
        }

        window
            .emit("game-launched", true)
            .expect("game launched event");

        unsafe {
            WaitForSingleObject(pi.hProcess, INFINITE);
        }

        window
            .emit("game-launched", false)
            .expect("game exited event");

        log!(window, "Game exited");
    } else {
        log!(window, "Could not map view of file {:#?}", result.err());
    }

    unsafe {
        CloseHandle(handle);
    };
}
