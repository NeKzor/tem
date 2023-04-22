/*
 * Copyright (c) 2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

use std::io::Write;
use tauri::{AppHandle, Manager, Window};
use crate::launcher::{AppConfig, AppState, LauncherConfig};

pub fn rewrite_grid_engine_config(window: &Window, app: &AppHandle, config: &LauncherConfig) {
    use std::fs;
    use std::io::{BufRead, BufReader, BufWriter};
    use std::path::Path;

    let state = app.state::<AppState>();

    let config_file_path = format!("{}\\GridEngine.ini", state.game_config_path);
    let backup_file_path = format!("{}\\GridEngine.backup.ini", state.game_config_path);

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
            log!(window, "Error at line {line_index}");
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
                        log!(window, "Set ResX to {}", config.window_width);
                        write!(writer, "ResX={}\r\n", config.window_width).unwrap();
                        continue;
                    }
                    "ResY" => {
                        log!(window, "Set ResY to {}", config.window_height);
                        write!(writer, "ResY={}\r\n", config.window_height).unwrap();
                        continue;
                    }
                    "Fullscreen" => {
                        log!(
                            window,
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
                    log!(
                        window,
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

pub fn load_config(app: &AppHandle) -> Result<AppConfig, std::io::Error> {
    let mut app_local_data = app
        .path_resolver()
        .app_local_data_dir()
        .expect("failed to resolve local data dir");

    app_local_data.push("config.json");

    let config_file = std::fs::File::open(app_local_data)?;

    Result::Ok(
        serde_json::from_reader(std::io::BufReader::new(config_file))
            .expect("unable to deserialize app config"),
    )
}
