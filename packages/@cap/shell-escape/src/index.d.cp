// Type definitions for CP

export fn isSafePosixByte(b: u8): bool;
export fn isSafePosixWord(arg: string): bool;
export fn escapePosix(arg: string, out: []u8): usize;
export fn needsWindowsQuotes(arg: string): bool;
export fn escapeWindows(arg: string, out: []u8): usize;
export fn joinPosixCommand(args: &[]string, out: []u8): usize;
export fn isValidEnvName(name: string): bool;
