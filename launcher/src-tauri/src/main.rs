/*
 * Copyright (c) 2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

static CON_PREFIX: &str = "[launcher]";

#[tauri::command]
fn console_execute(text: String) -> String {
    let text = text.trim_start().trim_end();
    let (command, args) = text.split_once(" ").or(Some((text.as_ref(), ""))).unwrap();
    match command {
        "echo" => format!("{} {}", CON_PREFIX, args),
        _ => format!("{} unknown command: {}", CON_PREFIX, command),
    }
}

fn main() {
    tauri::Builder::default()
        .invoke_handler(tauri::generate_handler![console_execute])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
