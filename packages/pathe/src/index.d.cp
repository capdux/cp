// Type definitions for CP

export fn pthIsSep(b: u8): bool;
export fn pthIsAlpha(b: u8): bool;
export fn pthIsSchemeChar(b: u8): bool;
export fn pthHasDrivePrefix(s: string): bool;
export fn pthProtocolPrefixLen(s: string): usize;
export fn pthPush(out: []u8, pos: usize, b: u8): usize;
export fn pthPushSlice(out: []u8, pos: usize, s: string): usize;
export fn pthStrEq(a: string, b: string): bool;
export fn isAbsolute(path: string): bool;
export fn normalize(path: string, out: []u8): usize;
export fn join(a: string, b: string, out: []u8): usize;
export fn joinPath(a: string, b: string, out: []u8): usize;
export fn pthStripTrailingSlash(out: []u8, len: usize): usize;
export fn resolve(base: string, rel: string, out: []u8): usize;
export fn dirname(path: string): string;
export fn basename(path: string): string;
export fn extname(path: string): string;
export fn pthSplitSegments(s: string, starts: []usize, lens: []usize): usize;
export fn relative(from_path: string, to_path: string, out: []u8): usize;
