// Type definitions for CP

import { normalizePathDefault } from "../../normalize-path/src/index.cp";
import { isGlobDefault } from "../../is-glob/src/index.cp";
export fn bytesEq(a: &[]u8, b: &[]u8): bool;
export fn startsWithDirPrefix(path: &[]u8, prefix: &[]u8): bool;
export fn globMatchAt(pat: &[]u8, pi_in: usize, txt: &[]u8, ti_in: usize): bool;
export fn matchPattern(pattern: string, test_path: string): bool;
export fn anymatchIndex(patterns: &[]string, test_path: string): i64;
export fn anymatch(patterns: &[]string, test_path: string): bool;
export fn countGlobPatterns(patterns: &[]string): usize;
