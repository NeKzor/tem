/*
 * Copyright (c) 2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

#[macro_export]
#[allow(unused)]
macro_rules! log {
    ($window:ident, $($arg:tt)*) => {{
        let message = format!($($arg)*);
        println!("{}" , message);
        $window.emit("log", message).unwrap();
    }};
}
