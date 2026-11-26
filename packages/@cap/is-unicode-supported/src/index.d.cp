// Type definitions for CP

export class EnvInfo {
    is_windows: bool,
    ci: bool,
    wt_session: bool,
    terminus_sublime: bool,
    conemu_task: string,
    term_program: string,
    term: string,
    terminal_emulator: string,
    fn posix(): EnvInfo;
    fn posixWithTerm(term: string): EnvInfo;
    fn linuxConsole(): EnvInfo;
    fn windows(): EnvInfo;
    fn windowsTerminal(): EnvInfo;
    fn vscodeWindows(): EnvInfo;
}

export fn iusStrEq(a: string, b: string): bool;
export fn isUnicodeSupportedTerm(term: string): bool;
export fn isUnicodeSupportedTermProgram(term_program: string): bool;
export fn isUnicodeSupported(env: EnvInfo): bool;
export fn isUnicodeSupportedPlatform(is_windows: bool, term: string): bool;
export fn selectFallback(env: EnvInfo, unicode_val: string, ascii_val: string): string;
