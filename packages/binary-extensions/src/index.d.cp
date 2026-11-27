// Type definitions for CP

import { toLowerByte, rfindByte } from "cap:ascii";
export fn cmpLowerToAscii(a: string, b: string): i32;
export fn extensionCategory(ext: string): u8;
export fn isBinaryExtension(ext: string): bool;
export fn isBinaryPath(path: string): bool;
