// Type definitions for CP

import { PicoOptions, isMatchWithOptions, isMatchAny, scanBase } from "../../picomatch/src/index.cp";
import { BraceResult, expandBraces } from "../../braces/src/index.cp";
export fn mmHasBrace(s: string): bool;
export fn mmCountLeadingNeg(pat: string): usize;
export fn mmMatchPositiveExpanded(path: string, inner: string, opts: PicoOptions): bool;
export fn mmWriteWrapped(dst: []u8, prefix: string, mid: string, suffix: string): usize;
export fn mmContainsSinglePositive(path: string, pat: string, opts: PicoOptions): bool;
export fn mmIsMatch(path: string, pattern: string, opts: PicoOptions): bool;
export fn mmAll(path: string, patterns: &[]string, opts: PicoOptions): bool;
export fn mmAny(path: string, patterns: &[]string, opts: PicoOptions): bool;
export fn mmSome(paths: &[]string, patterns: &[]string, opts: PicoOptions): bool;
export fn mmEvery(paths: &[]string, patterns: &[]string, opts: PicoOptions): bool;
export fn mmFilter(paths: &[]string, patterns: &[]string, opts: PicoOptions, out_indices: []usize): usize;
export fn mmNot(paths: &[]string, patterns: &[]string, opts: PicoOptions, out_indices: []usize): usize;
export fn mmContains(path: string, pattern: string, opts: PicoOptions): bool;
