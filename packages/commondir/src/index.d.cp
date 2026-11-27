// Type definitions for CP

export const COMMONDIR_MAX_PATH: usize;

export fn normalizePosixPath(path: string, out: []u8): usize;
export fn resolvePosixPath(base: string, rel: string, out: []u8): usize;
export fn commonDirTwo(a: string, b: string): string;
export fn commonDir(paths: &[]string): string;
export fn commonDirTwoNormalized(a: string, b: string, out: []u8): usize;
export fn commonDirNormalized(paths: &[]string, out: []u8): usize;
export fn commonDirWithBase(base: string, files: &[]string, out: []u8): usize;
