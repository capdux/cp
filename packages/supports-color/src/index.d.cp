// Type definitions for CP

import { eqIgnoreCase } from "cap:ascii";
import { hasFlag } from "../../has-flag/src/index.cp";
export class ColorSupport {
    level: u8,
    has_basic: bool,
    has_256: bool,
    has_16m: bool,
}

export class TerminalEnv {
    is_tty: bool,
    force_color: string,
    no_color: string,
    term: string,
    colorterm: string,
    term_program: string,
    ci: string,
    ci_name: string,
    is_windows: bool,
    win_build: u32,
    fn empty(): TerminalEnv;
    fn tty(): TerminalEnv;
}

export fn scContainsIgnoreCase(haystack: string, needle: string): bool;
export fn translateLevel(level: u8): ColorSupport;
export fn scDetectCliOverride(argv: &[]string): i8;
export fn scParseForceColor(val: string): i8;
export fn detectColorLevel(env: TerminalEnv, argv: &[]string): u8;
export fn detectColorSupport(env: TerminalEnv, argv: &[]string): ColorSupport;
