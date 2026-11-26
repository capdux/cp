// Type definitions for CP

import { mmIsMatch, mmFilter, mmNot } from "../../micromatch/src/index.cp";
import { PicoOptions } from "../../picomatch/src/index.cp";
export class MinimatchOptions {
    dot: bool,
    nocase: bool,
    match_base: bool,
    noglobstar: bool,
    nonegate: bool,
    nocomment: bool,
    flip_negate: bool,
    fn defaults(): MinimatchOptions;
}

export fn miniToPico(opts: MinimatchOptions): PicoOptions;
export fn isCommentPattern(pattern: string, opts: MinimatchOptions): bool;
export fn isNegatedPattern(pattern: string, opts: MinimatchOptions): bool;
export fn minimatchWithOptions(path: string, pattern: string, opts: MinimatchOptions): bool;
export fn minimatch(path: string, pattern: string): bool;
export fn minimatchList(paths: &[]string, pattern: string, opts: MinimatchOptions, out_indices: []usize): usize;
export fn minimatchFilterAll(paths: &[]string, patterns: &[]string, opts: MinimatchOptions, out_indices: []usize): usize;
export fn minimatchRejectAll(paths: &[]string, patterns: &[]string, opts: MinimatchOptions, out_indices: []usize): usize;
