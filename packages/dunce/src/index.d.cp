// Type definitions for CP

import { isAlpha, eqIgnoreCase, findByte, startsWith } from "cap:ascii";
import { writeStr } from "cap:fmt";

export const MAX_LEGACY_PATH: usize;

export fn isIllegalWinChar(ch: u8): bool;
export fn isWindowsReservedName(component: string): bool;
export fn isSafeLegacyComponent(component: string): bool;
export fn hasWindowsVerbatimPrefix(path: string): bool;
export fn hasWindowsVerbatimUncPrefix(path: string): bool;
export fn canStripVerbatimPrefix(path: string): bool;
export fn canSimplifyUncPath(path: string): bool;
export fn simplifiedSlice(path: string): string;
export fn simplifiedLen(path: string): usize;
export fn simplified(path: string, out: []u8): usize;
export fn simplifiedUnc(path: string, out: []u8): usize;
export fn simplifiedWithUnc(path: string, out: []u8): usize;
