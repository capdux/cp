// Type definitions for CP

import { startsWithBytes } from "cap:bin";
import { writeStr } from "cap:fmt";
export fn isSlash(ch: u8): bool;
export fn writeByte(out: []u8, pos: usize, b: u8): usize;
export fn trimTrailingSlashes(s: string): usize;
export fn resolveHomeDir(home_env: string, userprofile_env: string, homedrive_env: string, homepath_env: string, is_windows: bool, out: []u8): usize;
export fn expandTilde(path: string, home_dir: string, out: []u8): usize;
export fn contractTilde(path: string, home_dir: string, out: []u8): usize;
export fn joinConfigDir(home_dir: string, xdg_config_home: string, app_name: string, is_windows: bool, out: []u8): usize;
