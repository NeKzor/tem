/*
 * Copyright (c) 2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

use crate::{
    config::rewrite_grid_engine_config,
    launcher::{launch_game, AppState, LauncherConfig},
    mods::{set_game_mods, APP_USER_AGENT},
};
use std::process::Command;
use tauri::{AppHandle, State, Window};

#[tauri::command]
pub fn launch_config(config: LauncherConfig, window: Window, app: AppHandle) -> Result<(), ()> {
    rewrite_grid_engine_config(&window, &app, &config);
    set_game_mods(&config, &app);
    launch_game(&window, &app);

    Ok(())
}

#[tauri::command]
pub fn console_execute(text: String, state: State<'_, AppState>) -> String {
    let text = text.trim();
    let (command, args) = text
        .split_once(" ")
        .or(Some((text.as_ref(), "")))
        .expect("failed to split command");

    match command.to_lowercase().as_str() {
        "echo" => args.to_string(),
        "ua" | "user-agent" => APP_USER_AGENT.into(),
        "game" => {
            let folder = state.game_install_path.clone();

            if let Ok(..) = Command::new("explorer").arg(&folder).spawn() {
                folder
            } else {
                "Unable to open game folder".into()
            }
        }
        "config" => {
            let folder = state.game_config_path.clone();

            if let Ok(..) = Command::new("explorer").arg(&folder).spawn() {
                folder
            } else {
                "Unable to open config folder".into()
            }
        }
        "help" => "Commands: user-agent, game, config".into(),
        _ => format!("Unknown command: {command}"),
    }
}
