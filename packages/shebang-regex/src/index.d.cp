// Type definitions for CP

export class ShebangMatch {
    matched: bool,
    line_len: usize,
    cmd_start: usize,
    cmd_len: usize,
}

export fn hasShebang(input: string): bool;
export fn matchShebang(input: string): ShebangMatch;
export fn stripShebang(input: string): usize;
