/*
 * Copyright (c) 2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use std::os::windows::prelude::OsStrExt;

use serde::{Deserialize, Serialize};
use tauri::{AppHandle, Manager};
use tauri::{CustomMenuItem, SystemTray, SystemTrayEvent, SystemTrayMenu, SystemTrayMenuItem};

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
#[serde(rename_all = "camelCase")]
struct LauncherConfig {
    name: String,
    created_at: i64,
    modified_at: i64,
    window_width: i32,
    window_height: i32,
    is_fullscreen: bool,
    disable_splash_screen: bool,
    is_default: bool,
    #[serde(rename = "useTEM")]
    use_tem: bool,
    #[serde(rename = "useXDead")]
    use_xdead: bool,
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
    launch_game(&window, &state);

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
                        println!("Set ResX to {}", config.window_width);
                        write!(writer, "ResX={}\r\n", config.window_width).unwrap();
                        continue;
                    }
                    "ResY" => {
                        println!("Set ResY to {}", config.window_height);
                        write!(writer, "ResY={}\r\n", config.window_height).unwrap();
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

        if config.use_xdead {
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

fn launch_game(window: &tauri::Window, state: &AppState) {
    use windows::core::{PCSTR, PWSTR};
    use windows::Win32::Foundation::*;
    use windows::Win32::System::Memory::*;
    use windows::Win32::System::Threading::*;

    println!("game is installed in {:#?}", state.game_install_path);

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
        println!("Could not create file mapping object {:#?}", result.err());
        return;
    };

    let result = unsafe { MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, SECUROM_BUFFER_SIZE) };

    if result.is_ok() {
        println!("created file mapping");

        let cmd = format!("{}\\{GAME_EXE}", state.game_install_path);
        println!("launching {cmd}");

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
            let result = CreateProcessW(
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

            println!("CreateProcessW = {:#?}", result);
        }

        println!("hProcess {:#?}", pi.hProcess);
        println!("hThread {:#?}", pi.hThread);
        println!("dwProcessId {:#?}", pi.dwProcessId);
        println!("dwThreadId {:#?}", pi.dwThreadId);

        window
            .emit("game-launched", true)
            .expect("game launched event");

        unsafe {
            let result = WaitForSingleObject(pi.hProcess, INFINITE);
            println!("WaitForSingleObject = {:#?}", result);
        }

        // let mut child = Command::new(cmd)
        //     .spawn()
        //     .expect(format!("failed to start {GAME_EXE}").as_str());

        // child
        //     .wait()
        //     .expect(format!("failed to wait on {GAME_EXE}").as_str());

        window
            .emit("game-launched", false)
            .expect("game exited event");

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

fn show_launcher(app: &AppHandle) {
    let window = app.get_window("main").expect("unable to find main window");
    window.show().expect("unable to hide main window");
    window
        .unminimize()
        .expect("unable to unminimize main window");
    window
        .set_focus()
        .expect("unable to set focus to main window");
}

fn main() {
    let state = AppState {
        game_install_path: get_game_install_path(),
    };

    let tray_menu = SystemTrayMenu::new()
        .add_item(CustomMenuItem::new("show".to_string(), "TEM Launcher"))
        .add_native_item(SystemTrayMenuItem::Separator)
        .add_item(CustomMenuItem::new("quit".to_string(), "Quit"));

    tauri::Builder::default()
        .system_tray(SystemTray::new().with_menu(tray_menu))
        .on_system_tray_event(|app, event| match event {
            SystemTrayEvent::LeftClick { .. } => {
                show_launcher(app);
            }
            SystemTrayEvent::MenuItemClick { id, .. } => match id.as_str() {
                "show" => {
                    show_launcher(app);
                }
                "quit" => {
                    let window = app.get_window("main").expect("unable to find main window");
                    window.close().expect("unable to close main window");
                }
                _ => {}
            },
            _ => {}
        })
        .manage(state)
        .invoke_handler(tauri::generate_handler![launch_config, console_execute])
        .setup(|app| {
            let tem_dll = app
                .path_resolver()
                .resolve_resource("mods/tem/tem.dll")
                .expect("failed to resolve tem");

            if !tem_dll.exists() {
                // TODO: download latest release
            }

            println!("tem_dll {}", tem_dll.exists());

            let xdead = app
                .path_resolver()
                .resolve_resource("mods/xdead/xdead.dll")
                .expect("failed to resolve xdead");

            if !xdead.exists() {
                // TODO: download latest release
            }

            println!("xdead {}", xdead.exists());

            Ok(())
        })
        .on_window_event(|event| match event.event() {
            tauri::WindowEvent::CloseRequested { api, .. } => {
                event.window().hide().unwrap();
                api.prevent_close();
            }
            _ => {}
        })
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
