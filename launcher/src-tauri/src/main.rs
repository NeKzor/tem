/*
 * Copyright (c) 2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use serde::{Deserialize, Serialize};

static CON_PREFIX: &str = "[launcher]";
static CONFIG_PATH: &str =
    "Documents\\Disney Interactive Studios\\Tron Evolution\\UnrealEngine3\\GridGame\\Config";
static GAME_EXE: &str = "GridGame.exe";
static SECUROM_BUFFER_SIZE: usize = 1723;

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
fn launch_config(config: LauncherConfig) {
    println!("launching game with config: {}", config.name);
}

#[tauri::command]
fn console_execute(text: String) -> String {
    let text = text.trim();
    let (command, args) = text.split_once(" ").or(Some((text.as_ref(), ""))).unwrap();
    match command {
        "echo" => format!("{} {}", CON_PREFIX, args),
        _ => format!("{} unknown command: {}", CON_PREFIX, command),
    }
}

fn main() {
    use windows::core::PCSTR;
    use windows::w;
    use windows::Win32::Foundation::*;
    use windows::Win32::System::Memory::*;
    use windows::Win32::System::Registry::*;

    unsafe {
        let mut install_path = Vec::<u16>::with_capacity(MAX_PATH as usize);
        let mut install_path_size = MAX_PATH;

        let result = RegGetValueW(
            HKEY_LOCAL_MACHINE,
            w!("Software\\WOW6432Node\\Disney Interactive Studios\\tr2npc"),
            w!("InstallPath"),
            RRF_RT_ANY,
            None,
            Some(install_path.as_mut_ptr() as _),
            Some(&mut install_path_size as *mut _),
        );

        if result != ERROR_SUCCESS {
            println!("game does not seem to be installed: {:#?}", result);
        } else {
            println!("game is installed");

            let file = format!("-=[SMS_{}_SMS]=-", GAME_EXE);

            let result = CreateFileMappingA(
                INVALID_HANDLE_VALUE,
                None,
                PAGE_READWRITE,
                0,
                SECUROM_BUFFER_SIZE as u32,
                PCSTR(file.as_ptr()),
            );

            if let Ok(handle) = result {
                let result = MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, SECUROM_BUFFER_SIZE);
                if result.is_err() {
                    println!("Could not map view of file {:#?}", GetLastError());
                    CloseHandle(handle);
                } else {
                    println!("Created file mapping!");
                    CloseHandle(handle);
                }
            } else {
                println!("Could not create file mapping object {:#?}", GetLastError());
            }
        }
    }

    let user_profile = std::env::vars()
        .find(|(key, _)| key == "USERPROFILE")
        .unwrap()
        .1;

    let config_path = format!("{user_profile}\\{CONFIG_PATH}");

    use std::fs;
    use std::io::{BufRead, BufReader, BufWriter, Write};

    let grid_engine_config = fs::OpenOptions::new()
        .read(true)
        .write(true)
        .open(format!("{config_path}\\GridEngine.ini"))
        .unwrap();

    let reader = BufReader::new(&grid_engine_config);
    let mut writer = BufWriter::new(&grid_engine_config);

    let mut is_system_settings = false;
    let mut is_fullscreen_movie = false;

    let mut line_index = 1;

    for line in reader.lines() {
        if let Ok(line) = line {
            let text = line.trim();
            let is_empty = text.len() == 0;
            if is_empty {
                write!(writer, "\r\n").unwrap();
                continue;
            }

            let is_section = text.starts_with("[") && text.ends_with("]");

            if is_section {
                is_system_settings = false;
                is_fullscreen_movie = false;

                if text == "[SystemSettings]" {
                    is_system_settings = true;
                } else if text == "[FullScreenMovie]" {
                    is_fullscreen_movie = true;
                }
            } else {
                if let Some((key, value)) = text.split_once("=") {
                    if is_system_settings {
                        match key {
                            "ResX" => {
                                println!("Rewriting RexX: {value} -> {}", 0);
                                write!(writer, "RexX={}\r\n", 1920).unwrap();
                                continue;
                            }
                            "ResY" => {
                                println!("Rewriting RexY: {value} -> {}", 0);
                                write!(writer, "RexY={}\r\n", 1080).unwrap();
                                continue;
                            }
                            "Fullscreen" => {
                                println!("Rewriting Fullscreen: {value} -> {}", "true");
                                write!(writer, "Fullscreen={}\r\n", "true").unwrap();
                                continue;
                            }
                            _ => {}
                        }
                    } else if is_fullscreen_movie {
                        if text.starts_with(";StartupMovies") || text.starts_with("StartupMovies") {
                            println!("{} StartupMovies={value}", "Enabling");
                            write!(writer, "StartupMovies={value}\r\n").unwrap();
                            continue;
                        }
                    }
                }

                write!(writer, "{line}\r\n").unwrap();
            }
        } else {
            println!("error at line {line_index}");
        }

        line_index += 1;
    }

    // let system_settings = conf.section(Some("FullScreenMovie")).unwrap();

    // tauri::Builder::default()
    //     .invoke_handler(tauri::generate_handler![launch_config, console_execute])
    //     .run(tauri::generate_context!())
    //     .expect("error while running tauri application");
}
