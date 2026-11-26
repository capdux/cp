// Type definitions for CP

export enum EnvPlatform {
    Linux,
    MacOS,
    Windows
}

export class EnvPathsInput {
    platform: EnvPlatform,
    home: string,
    xdg_data_home: string,
    xdg_config_home: string,
    xdg_cache_home: string,
    xdg_state_home: string,
    tmpdir: string,
    local_app_data: string,
    app_data: string,
    suffix: string,
    fn new(platform: EnvPlatform, home: string): EnvPathsInput;
    fn linux(home: string): EnvPathsInput;
    fn macos(home: string): EnvPathsInput;
    fn windows(home: string): EnvPathsInput;
}

export fn evpTrimTrailingSep(s: string, sep: u8): string;
export fn evpAppend(out: []u8, pos: usize, s: string): usize;
export fn evpAppendByte(out: []u8, pos: usize, b: u8): usize;
export fn evpAppendAppName(out: []u8, pos: usize, name: string, suffix: string): usize;
export fn evpJoinUnix2(out: []u8, base: string, name: string, suffix: string): usize;
export fn evpJoinUnix3(out: []u8, base: string, mid: string, name: string, suffix: string): usize;
export fn evpWinAppBase(out: []u8, explicit: string, home: string, fallback_sub: string): usize;
export fn evpWinDir(out: []u8, explicit: string, home: string, fallback_sub: string, name: string, suffix: string, leaf: string): usize;
export fn envDataDir(name: string, inp: EnvPathsInput, out: []u8): usize;
export fn envConfigDir(name: string, inp: EnvPathsInput, out: []u8): usize;
export fn envCacheDir(name: string, inp: EnvPathsInput, out: []u8): usize;
export fn envLogDir(name: string, inp: EnvPathsInput, out: []u8): usize;
export fn envTempDir(name: string, inp: EnvPathsInput, out: []u8): usize;
