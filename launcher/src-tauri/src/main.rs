/*
 * Copyright (c) 2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use crate::commands::{console_execute, launch_config};
use config::load_config;
use launcher::{AppState, CheckForUpdatesOption};
use mods::{download_mod, TEM_FILES};
use tauri::{
    AppHandle, CustomMenuItem, Manager, SystemTray, SystemTrayEvent, SystemTrayMenu,
    SystemTrayMenuItem, WindowEvent,
};

#[macro_use]
mod macros;
mod commands;
mod config;
mod launcher;
mod mods;

const CONFIG_PATH: &str =
    r"Documents\Disney Interactive Studios\Tron Evolution\UnrealEngine3\GridGame\Config";

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

fn get_game_config_path() -> String {
    let user_profile = std::env::vars()
        .find(|(key, _)| key == "USERPROFILE")
        .expect("env var USERPROFILE not set")
        .1;

    format!("{user_profile}\\{CONFIG_PATH}")
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
        game_config_path: get_game_config_path(),
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

                    if let Ok(config) = load_config(&app) {
                        match config.check_for_updates {
                            CheckForUpdatesOption::OnLauncherExit => {
                                let tem_dir = app
                                    .path_resolver()
                                    .resolve_resource("mods/tem/")
                                    .expect("failed to resolve tem dir");

                                let xdead_dir = app
                                    .path_resolver()
                                    .resolve_resource("mods/xdead/")
                                    .expect("failed to resolve xdead dir");

                                tauri::async_runtime::block_on(async move {
                                    download_mod(
                                        "https://api.github.com/repos/NeKzor/tem/releases",
                                        TEM_FILES.into(),
                                        tem_dir,
                                    )
                                    .await;
                                });

                                tauri::async_runtime::block_on(async move {
                                    download_mod(
                                        "https://api.github.com/repos/NeKzor/xdead/releases",
                                        vec!["xlive.dll"],
                                        xdead_dir,
                                    )
                                    .await;
                                });
                            }
                            _ => {}
                        }
                    }
                }
                _ => {}
            },
            _ => {}
        })
        .manage(state)
        .invoke_handler(tauri::generate_handler![launch_config, console_execute])
        .setup(|app| {
            if let Ok(config) = load_config(&app.app_handle()) {
                match config.check_for_updates {
                    CheckForUpdatesOption::OnLauncherStart => {
                        let tem_dir = app
                            .path_resolver()
                            .resolve_resource("mods/tem/")
                            .expect("failed to resolve tem dir");

                        let xdead_dir = app
                            .path_resolver()
                            .resolve_resource("mods/xdead/")
                            .expect("failed to resolve xdead dir");

                        tauri::async_runtime::spawn(async move {
                            download_mod(
                                "https://api.github.com/repos/NeKzor/tem/releases",
                                TEM_FILES.into(),
                                tem_dir,
                            )
                            .await;
                        });

                        tauri::async_runtime::spawn(async move {
                            download_mod(
                                "https://api.github.com/repos/NeKzor/xdead/releases",
                                vec!["xlive.dll"],
                                xdead_dir,
                            )
                            .await;
                        });
                    }
                    _ => {}
                }
            }

            Ok(())
        })
        .on_window_event(|event| match event.event() {
            WindowEvent::CloseRequested { api, .. } => {
                event.window().hide().unwrap();
                api.prevent_close();
            }
            _ => {}
        })
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
