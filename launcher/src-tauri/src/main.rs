/*
 * Copyright (c) 2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use std::io::{Read, Write};
use std::os::windows::prelude::OsStrExt;

use serde::{Deserialize, Serialize};
use tauri::{
    AppHandle, CustomMenuItem, Manager, SystemTray, SystemTrayEvent, SystemTrayMenu,
    SystemTrayMenuItem, Window, WindowEvent,
};

const APP_USER_AGENT: &str = "TEM Launcher v0.1.0";
const CON_PREFIX: &str = "[launcher]";
const CONFIG_PATH: &str =
    "Documents\\Disney Interactive Studios\\Tron Evolution\\UnrealEngine3\\GridGame\\Config";
const GAME_EXE: &str = "GridGame.exe";
const SECUROM_BUFFER_SIZE: usize = 1723;
const TEM_FILES: [&str; 3] = ["dinput8.dll", "patch.dat", "tem.dll"];

struct AppState {
    game_install_path: String,
}

#[derive(Clone, serde::Serialize)]
struct EventPayload {
    message: String,
}

#[derive(Serialize, Deserialize, Debug)]
enum SortOptionDirection {
    #[serde(rename = "asc")]
    Ascending,
    #[serde(rename = "desc")]
    Descending,
}

#[derive(Serialize, Deserialize, Debug)]
struct SortOption {
    key: String,
    direction: SortOptionDirection,
}

#[derive(Serialize, Deserialize, Debug)]
enum SortOrderOption {
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
enum CheckForUpdatesOption {
    #[serde(rename = "disabled")]
    Disabled,
    #[serde(rename = "on-launcher-start")]
    OnLauncherStart,
    #[serde(rename = "on-launcher-exit")]
    OnLauncherExit,
}

#[derive(Serialize, Deserialize, Debug)]
#[serde(rename_all = "camelCase")]
struct AppConfig {
    configs: Vec<LauncherConfig>,
    sort: SortOption,
    check_for_updates: CheckForUpdatesOption,
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

#[derive(Deserialize, Debug)]
#[allow(dead_code)]
struct GitHubReleaseAsset {
    url: String,
    id: i64,
    node_id: String,
    name: String,
    label: String,
    content_type: String,
    state: String,
    size: i64,
    download_count: i64,
    created_at: String,
    updated_at: String,
    browser_download_url: String,
}

#[derive(Deserialize, Debug)]
#[allow(dead_code)]
struct GitHubRelease {
    url: String,
    assets_url: String,
    upload_url: String,
    html_url: String,
    id: i64,
    node_id: String,
    tag_name: String,
    target_commitish: String,
    name: String,
    draft: bool,
    prerelease: bool,
    created_at: String,
    published_at: String,
    assets: Vec<GitHubReleaseAsset>,
}

#[tauri::command]
async fn launch_config(
    config: LauncherConfig,
    window: Window,
    app: AppHandle,
) -> Result<(), ()> {
    println!("launching game with config: {}", config.name);

    rewrite_grid_engine_config(&config);
    set_game_mods(&config, &app);
    launch_game(&window, &app);

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
    use std::io::{BufRead, BufReader, BufWriter};
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

fn set_game_mods(config: &LauncherConfig, app: &AppHandle) {
    use std::path::Path;

    let state = app.state::<AppState>();
    let tem_path = format!("{}\\tem.dll", state.game_install_path);
    let xdead_path = format!("{}\\xlive.dll", state.game_install_path);

    let tem_mod = Path::new(tem_path.as_str());
    let xdead_mod = Path::new(xdead_path.as_str());

    if config.use_tem {
        if !tem_mod.exists() {
            let tem_dir = app
                .path_resolver()
                .resolve_resource("mods/tem/")
                .expect("failed to resolve tem dir");

            for file_name in TEM_FILES {
                let mut tem_file = tem_dir.clone();
                tem_file.push(file_name);

                std::fs::copy(
                    tem_file,
                    Path::new(format!("{}\\{file_name}", state.game_install_path).as_str()),
                )
                .expect("unable to copy file");
            }
        }

        if config.use_xdead {
            if !xdead_mod.exists() {
                let xdead_dir = app
                    .path_resolver()
                    .resolve_resource("mods/xdead/")
                    .expect("failed to resolve xdead dir");

                let file_name = "xlive.dll";
                let mut xdead_file = xdead_dir.clone();
                xdead_file.push(file_name);

                std::fs::copy(xdead_file, xdead_mod).expect("unable to copy file");
            }
        } else {
            if xdead_mod.exists() {
                std::fs::remove_file(xdead_mod).expect("unable to remove file");
            }
        }
    } else {
        if tem_mod.exists() {
            for file_name in TEM_FILES {
                std::fs::remove_file(Path::new(
                    format!("{}\\{file_name}", state.game_install_path).as_str(),
                ))
                .expect("unable to remove file");
            }
        }
        if xdead_mod.exists() {
            std::fs::remove_file(xdead_mod).expect("unable to remove file");
        }
    }
}

fn launch_game(window: &Window, app: &AppHandle) {
    use windows::core::{PCSTR, PWSTR};
    use windows::Win32::Foundation::*;
    use windows::Win32::System::Memory::*;
    use windows::Win32::System::Threading::*;

    let state = app.state::<AppState>();

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

async fn download_mod(github_release_url: &str, mod_files: Vec<&str>, mod_dir: std::path::PathBuf) {
    std::fs::create_dir_all(&mod_dir).expect("unable to create mod dir");

    let mut mod_release = mod_dir.clone();
    mod_release.push("release.txt");

    let mut release_version = String::new();

    if let Ok(mut release_file) = std::fs::OpenOptions::new().read(true).open(&mod_release) {
        release_file
            .read_to_string(&mut release_version)
            .expect("unable to read release file");
    }

    let client = reqwest::Client::builder()
        .user_agent(APP_USER_AGENT)
        .build()
        .expect("unable to build http client");

    let releases = client
        .get(github_release_url)
        .send()
        .await
        .expect("request to GitHub releases failed")
        .json::<Vec<GitHubRelease>>()
        .await
        .expect("failed to deserialize json response of GitHub releases");

    let allow_unstable_releases = true;
    let Some(release) = releases.iter().find(|release| allow_unstable_releases || !release.prerelease) else {
        println!("no release found");
        return;
    };

    let Some(asset) = release.assets.iter().nth(0) else {
        println!("no asset found");
        return;
    };

    if release_version == asset.name {
        println!("already using latest release");
        return;
    }

    println!("updating to latest release");

    let file = client
        .get(&asset.browser_download_url)
        .send()
        .await
        .expect("failed to download asset");

    println!("downloaded latest asset {}", asset.name);

    std::fs::OpenOptions::new()
        .create(true)
        .write(true)
        .truncate(true)
        .open(&mod_release)
        .expect("unable to read or create release version")
        .write_all(asset.name.as_bytes())
        .expect("unable to write release version");

    let reader = std::io::Cursor::new(file.bytes().await.expect("unable to read response"));

    let mut zip = zip::ZipArchive::new(reader).expect("unable to create zip archive");

    for file_name in mod_files {
        if let Ok(mut file) = zip.by_name(file_name) {
            let mut file_path = mod_dir.clone();
            file_path.push(file_name);

            let mut outfile = std::fs::File::create(file_path).expect("unable to create file");

            std::io::copy(&mut file, &mut outfile).expect("unable to copy to to file");
        }
    }
}

fn load_config(app: &AppHandle) -> Result<AppConfig, std::io::Error> {
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
