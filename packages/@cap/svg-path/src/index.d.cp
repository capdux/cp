// Type definitions for CP

export class PathCmd {
    code: u8,
    relative: bool,
    args: [7]f64,
    arg_count: u8,
    fn empty(): PathCmd;
}

export class ParsedPath {
    cmds: [64]PathCmd,
    count: usize,
    valid: bool,
    fn invalid(): ParsedPath;
    fn cmdAt(i: usize): PathCmd?;
}

export class PathBounds {
    valid: bool,
    min_x: f64,
    min_y: f64,
    max_x: f64,
    max_y: f64,
    width: f64,
    height: f64,
    fn empty(): PathBounds;
}

export fn isWs(c: u8): bool;
export fn isDigit(c: u8): bool;
export fn skipWs(s: string, start: usize): usize;
export fn skipWsComma(s: string, start: usize): usize;
export fn expectedArgCount(code: u8): u8?;
export fn isLowerCmd(code: u8): bool;
export fn isNumberStart(c: u8): bool;
export class ParsedNum {
    ok: bool,
    val: f64,
    next_pos: usize,
}

export fn pow10(exp: i32): f64;
export fn parseSvgNumber(s: string, start: usize): ParsedNum;
export fn parseSvgFlag(s: string, start: usize): ParsedNum;
export fn parseSvgPath(d: string): ParsedPath;
export class BoundsAcc {
    has_point: bool,
    min_x: f64,
    min_y: f64,
    max_x: f64,
    max_y: f64,
}

export fn includePoint(acc: BoundsAcc, x: f64, y: f64): BoundsAcc;
export fn computeBounds(p: ParsedPath): PathBounds;
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn appendU64(out: []u8, pos: usize, val: u64): usize;
export fn appendFloatCompact(out: []u8, start_pos: usize, val: f64): usize;
export fn formatSvgPath(p: ParsedPath, out: []u8): string;
