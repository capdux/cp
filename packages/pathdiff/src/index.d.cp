// Type definitions for CP

export const PATHDIFF_MAX_SEGS: usize;

export fn hasSameRoot(path: string, base: string): bool;
export fn normalizePath(path: string, out: []u8): usize?;
export fn diffPaths(target: string, base: string, out: []u8): usize?;
export fn diffUtf8Paths(path: string, base: string, out: []u8): usize?;
