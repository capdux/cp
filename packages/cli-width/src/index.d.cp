// Type definitions for CP

export class CliWidthOptions {
    default_width: u32,
    min_width: u32,
    max_width: u32,
    static fn defaults(): CliWidthOptions;
}

export class TtyStreamInfo {
    is_tty: bool,
    columns: u32,
    rows: u32,
    static fn none(): TtyStreamInfo;
    static fn tty(columns: u32, rows: u32): TtyStreamInfo;
}

export class CliWidthConfig {
    default_width: u32,
    output: TtyStreamInfo,
    tty: TtyStreamInfo,
    env_columns: string,
    min_width: u32,
    max_width: u32,
    static fn defaults(): CliWidthConfig;
}

export fn defaultCliWidth(): u32;
export fn clampCliWidth(width: u32, min_width: u32, max_width: u32): u32;
export fn parseColumnsEnv(env_val: string): u32?;
export fn parseColumnsFromEnvList(env_entries: []const string): u32?;
export fn resolveCliWidth(tty_cols: u32, env_columns: string, opts: CliWidthOptions): u32;
export fn cliWidth(cfg: CliWidthConfig): u32;
