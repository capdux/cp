// Type definitions for CP

export fn isSlash(ch: u8): bool;
export fn copySlice(out: []u8, pos: usize, src: string): usize;
export fn writeByte(out: []u8, pos: usize, b: u8): usize;
export fn trimTrailingSlashes(s: string): usize;
export fn bytesPrefixEq(path: string, prefix: string): bool;
export fn resolveHomeDir(home_env: string, userprofile_env: string, homedrive_env: string, homepath_env: string, is_windows: bool, out: []u8): usize;
export fn expandTilde(path: string, home_dir: string, out: []u8): usize;
export fn contractTilde(path: string, home_dir: string, out: []u8): usize;
export fn joinConfigDir(home_dir: string, xdg_config_home: string, app_name: string, is_windows: bool, out: []u8): usize;
