// Type definitions for CP

import { isAlpha, eqIgnoreCase, findByte, startsWith } from "cap:ascii";
import { writeStr } from "cap:fmt";
export fn isIllegalWinChar(ch: u8): bool;
export fn isWindowsReservedName(component: string): bool;
export fn hasWindowsVerbatimPrefix(path: string): bool;
export fn canStripVerbatimPrefix(path: string): bool;
export fn simplified(path: string, out: []u8): usize;
