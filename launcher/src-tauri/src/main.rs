/*
 * Copyright (c) 2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use serde::{Deserialize, Serialize};

const CON_PREFIX: &str = "[launcher]";
const CONFIG_PATH: &str =
    "Documents\\Disney Interactive Studios\\Tron Evolution\\UnrealEngine3\\GridGame\\Config";
const GAME_EXE: &str = "GridGame.exe";
const SECUROM_BUFFER_SIZE: usize = 1723;

struct AppState {
    game_install_path: String,
}

#[derive(Clone, serde::Serialize)]
struct EventPayload {
    message: String,
}

#[derive(Serialize, Deserialize, Debug)]
struct GameResolution {
    width: i32,
    height: i32,
}

#[derive(Serialize, Deserialize, Debug)]
struct LauncherConfig {
    name: String,
    #[serde(rename = "createdAt")]
    created_at: i64,
    #[serde(rename = "modifiedAt")]
    modified_at: i64,
    resolution: GameResolution,
    #[serde(rename = "isFullscreen")]
    is_fullscreen: bool,
    #[serde(rename = "disableSplashScreen")]
    disable_splash_screen: bool,
    #[serde(rename = "isDefault")]
    is_default: bool,
    #[serde(rename = "useTEM")]
    use_tem: bool,
    #[serde(rename = "useXDead")]
    use_x_dead: bool,
}

#[tauri::command]
async fn launch_config(
    config: LauncherConfig,
    window: tauri::Window,
    state: tauri::State<'_, AppState>,
) -> Result<(), ()> {
    println!("launching game with config: {}", config.name);

    rewrite_grid_engine_config(&config);
    set_game_mods(&config, &state);

    Ok(())
}

#[tauri::command]
fn console_execute(text: String) -> String {
    let text = text.trim();
    let (command, args) = text
        .split_once(" ")
        .or(Some((text.as_ref(), "")))
        .expect("failed to split command");

    match command {
        "echo" => format!("{CON_PREFIX} {args}"),
        _ => format!("{CON_PREFIX} unknown command: {command}"),
    }
}

fn rewrite_grid_engine_config(config: &LauncherConfig) {
    let user_profile = std::env::vars()
        .find(|(key, _)| key == "USERPROFILE")
        .expect("env var USERPROFILE not set")
        .1;

    let config_path = format!("{user_profile}\\{CONFIG_PATH}");

    use std::fs;
    use std::io::{BufRead, BufReader, BufWriter, Write};
    use std::path::Path;

    let config_file_path = format!("{config_path}\\GridEngine.ini");
    let backup_file_path = format!("{config_path}\\GridEngine.backup.ini");

    if !Path::new(&backup_file_path).exists() {
        fs::copy(&config_file_path, &backup_file_path).expect("failed to create a backup file");
    }

    let grid_engine_config = fs::OpenOptions::new()
        .read(true)
        .open(&backup_file_path)
        .expect("failed to open backup file");

    let new_grid_engine_config = fs::OpenOptions::new()
        .write(true)
        .truncate(true)
        .open(&config_file_path)
        .expect("failed to open GridEngine.ini");

    let reader = BufReader::new(&grid_engine_config);
    let mut writer = BufWriter::new(&new_grid_engine_config);

    let mut is_system_settings = false;
    let mut is_fullscreen_movie = false;

    let mut line_index = 1;

    for line in reader.lines() {
        let Ok(line) = line else {
            println!("error at line {line_index}");
            line_index += 1;
            continue;
        };

        let text = line.trim();
        let is_empty = text.len() == 0;
        if is_empty {
            write!(writer, "\r\n").unwrap();
            continue;
        }

        let is_section = text.starts_with("[") && text.ends_with("]");

        if is_section {
            is_system_settings = text == "[SystemSettings]";
            is_fullscreen_movie = text == "[FullScreenMovie]";
        } else if let Some((key, value)) = text.split_once("=") {
            if is_system_settings {
                match key {
                    "ResX" => {
                        println!("Set RexX to {}", config.resolution.width);
                        write!(writer, "RexX={}\r\n", config.resolution.width).unwrap();
                        continue;
                    }
                    "ResY" => {
                        println!("Set RexY to {}", config.resolution.height);
                        write!(writer, "RexY={}\r\n", config.resolution.height).unwrap();
                        continue;
                    }
                    "Fullscreen" => {
                        println!(
                            "Set Fullscreen to {}",
                            if config.is_fullscreen {
                                "True"
                            } else {
                                "False"
                            }
                        );
                        write!(
                            writer,
                            "Fullscreen={}\r\n",
                            if config.is_fullscreen {
                                "True"
                            } else {
                                "False"
                            }
                        )
                        .unwrap();
                        continue;
                    }
                    _ => {}
                }
            } else if is_fullscreen_movie {
                if text.starts_with(";StartupMovies") || text.starts_with("StartupMovies") {
                    println!(
                        "{} StartupMovies={value}",
                        if config.disable_splash_screen {
                            "Disabled"
                        } else {
                            "Enabled"
                        }
                    );
                    write!(
                        writer,
                        "{}StartupMovies={value}\r\n",
                        if config.disable_splash_screen {
                            ";"
                        } else {
                            ""
                        }
                    )
                    .unwrap();
                    continue;
                }
            }
        }

        write!(writer, "{line}\r\n").unwrap();
        line_index += 1;
    }
}

fn set_game_mods(config: &LauncherConfig, state: &AppState) {
    use std::path::Path;

    let tem_game_path = format!("{}\\tem.dll", state.game_install_path);
    let dinput8_game_path = format!("{}\\dinput8.dll", state.game_install_path);
    let xdead_game_path = format!("{}\\xlive.dll", state.game_install_path);

    let tem_path = Path::new(tem_game_path.as_str());
    let dinput8_path = Path::new(dinput8_game_path.as_str());
    let xdead_path = Path::new(xdead_game_path.as_str());

    if config.use_tem {
        if !tem_path.exists() {
            // TODO: install tem.dll
        }
        if !dinput8_path.exists() {
            // TODO: install dinput8.dll
        }

        if config.use_x_dead {
            if !xdead_path.exists() {
                // TODO: install xlive.dll
            }
        } else {
            if xdead_path.exists() {
                // TODO: remove xlive.dll
            }
        }
    } else {
        if tem_path.exists() {
            // TODO: remove tem.dll
        }
        if dinput8_path.exists() {
            // TODO: remove dinput8.dll
        }
        if xdead_path.exists() {
            // TODO: remove xlive.dll
        }
    }
}

fn launch_game(state: &AppState) {
    use std::process::Command;
    use windows::core::PCSTR;
    use windows::Win32::Foundation::*;
    use windows::Win32::System::Memory::*;

    println!("game is installed in {:#?}", state.game_install_path);

    let file = format!("-=[SMS_{GAME_EXE}_SMS]=-");

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
        println!("Could not create file mapping object {:#?}", result.err());
        return;
    };

    let result = unsafe { MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, SECUROM_BUFFER_SIZE) };

    if result.is_ok() {
        println!("created file mapping");

        let mut child = Command::new(format!("{}\\{GAME_EXE}", state.game_install_path))
            .spawn()
            .expect(format!("failed to start {GAME_EXE}").as_str());

        child
            .wait()
            .expect(format!("failed to wait on {GAME_EXE}").as_str());

        println!("game exited");
    } else {
        println!("Could not map view of file {:#?}", result.err());
    }

    unsafe {
        CloseHandle(handle);
    };
}

fn get_game_install_path() -> String {
    use windows::w;
    use windows::Win32::Foundation::*;
    use windows::Win32::System::Registry::*;

    let mut install_path_buffer = [0u16; MAX_PATH as usize];
    let mut install_path_size = MAX_PATH;

    let result = unsafe {
        RegGetValueW(
            HKEY_LOCAL_MACHINE,
            w!("Software\\WOW6432Node\\Disney Interactive Studios\\tr2npc"),
            w!("InstallPath"),
            RRF_RT_ANY,
            None,
            Some(install_path_buffer.as_mut_ptr() as _),
            Some(&mut install_path_size as *mut _),
        )
    };

    if result != ERROR_SUCCESS {
        println!("game does not seem to be installed: {:#?}", result);
        return "".into();
    }

    let install_path = String::from_utf16(install_path_buffer.as_slice())
        .expect("failed to create valid utf16 string for install path");

    install_path.trim_end_matches(char::from(0)).into()
}

fn main() {
    let state = AppState {
        game_install_path: get_game_install_path(),
    };

    launch_game(&state);

    // tauri::Builder::default()
    //     .manage(state)
    //     .invoke_handler(tauri::generate_handler![launch_config, console_execute])
    //     .run(tauri::generate_context!())
    //     .expect("error while running tauri application");
}
