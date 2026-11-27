// Type definitions for CP

export class CliWidthOptions {
    default_width: u32,
    min_width: u32,
    max_width: u32,
}

export fn cliwIsSpace(c: u8): bool;
export fn defaultCliWidth(): u32;
export fn parseColumnsEnv(env_val: string): u32?;
export fn resolveCliWidth(tty_cols: u32, env_columns: string, opts: CliWidthOptions): u32;
