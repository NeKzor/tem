/*
 * Copyright (c) 2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

use crate::launcher::{AppState, LauncherConfig};
use serde::Deserialize;
use std::io::{Read, Write};
use tauri::{AppHandle, Manager};

pub const APP_USER_AGENT: &str = concat!("TEM Launcher ", env!("CARGO_PKG_VERSION"));
pub const TEM_FILES: [&str; 3] = ["dinput8.dll", "patch.dat", "tem.dll"];

#[derive(Deserialize, Debug)]
#[allow(dead_code)]
pub struct GitHubReleaseAsset {
    pub url: String,
    pub id: i64,
    pub node_id: String,
    pub name: String,
    pub label: String,
    pub content_type: String,
    pub state: String,
    pub size: i64,
    pub download_count: i64,
    pub created_at: String,
    pub updated_at: String,
    pub browser_download_url: String,
}

#[derive(Deserialize, Debug)]
#[allow(dead_code)]
pub struct GitHubRelease {
    pub url: String,
    pub assets_url: String,
    pub upload_url: String,
    pub html_url: String,
    pub id: i64,
    pub node_id: String,
    pub tag_name: String,
    pub target_commitish: String,
    pub name: String,
    pub draft: bool,
    pub prerelease: bool,
    pub created_at: String,
    pub published_at: String,
    pub assets: Vec<GitHubReleaseAsset>,
}

pub fn set_game_mods(config: &LauncherConfig, app: &AppHandle) {
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

pub async fn download_mod(
    github_release_url: &str,
    mod_files: Vec<&str>,
    mod_dir: std::path::PathBuf,
) {
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
