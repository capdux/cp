// Type definitions for CP

export class GlobOptions {
    extended: bool,
    globstar: bool,
    anchored: bool,
}

export fn pushByte(dst: []u8, pos: usize, b: u8): i64;
export fn pushSlice(dst: []u8, pos: usize, src: &[]u8): i64;
export fn isEscapeMeta(b: u8): bool;
export fn globToRegex(glob: &[]u8, opts: GlobOptions, dst: []u8): i64;
export fn globToRegexDefault(glob: &[]u8, dst: []u8): i64;
