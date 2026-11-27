// Type definitions for CP

import { strEq, eqIgnoreCase, rfindByte } from "cap:ascii";
import { writeStr } from "cap:fmt";
export fn cosmiEndsWithIgnoreCase(s: string, suffix: string): bool;
export fn cosmiBasename(filepath: string): string;
export fn cosmiTrimTrailingSlash(dir: string): string;
export class ExplorerOptions {
    module_name: string,
    stop_dir: string,
    package_prop: string,
    ignore_empty: bool,
    fn defaults(module_name: string): ExplorerOptions;
}

export fn detectLoader(filepath: string): u8;
export fn formatSearchPlace(module_name: string, place_idx: usize, out: []u8): usize;
export fn buildCandidatePath(dir: string, place: string, out: []u8): usize;
export class SearchPlanner {
    start_dir: string,
    stop_dir: string,
    current_end: usize,
    done: bool,
    fn new(start_dir: string, stop_dir: string): SearchPlanner;
    fn nextDir(_: auto): string?;
    fn dirCount(): usize;
}

export class CosmiConfigResult {
    filepath: string,
    is_empty: bool,
    loader_kind: u8,
    found: bool,
    fn notFound(): CosmiConfigResult;
    fn hit(filepath: string, is_empty: bool): CosmiConfigResult;
}

