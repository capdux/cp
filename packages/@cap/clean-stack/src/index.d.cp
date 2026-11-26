// Type definitions for CP

export class CleanStackOptions {
    pretty: bool,
    base_path: string,
    home_dir: string,
    fn defaults(): CleanStackOptions;
}

export fn csContains(hay: string, needle: string): bool;
export fn csStartsWith(s: string, prefix: string): bool;
export fn csAppend(out: []u8, pos: usize, s: string): usize;
export fn isInternalStackFrame(line: string): bool;
export fn csWriteCleanedLine(line: string, opts: CleanStackOptions, out: []u8, pos: usize): usize;
export fn cleanStackWithOptions(stack: string, opts: CleanStackOptions, out: []u8): usize;
export fn cleanStack(stack: string, out: []u8): usize;
