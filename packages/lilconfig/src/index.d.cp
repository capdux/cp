// Type definitions for CP

import { LOADER_UNKNOWN, LOADER_JSON, LOADER_YAML, LOADER_JS, LOADER_PKG_JSON, SearchPlanner, CosmiConfigResult, detectLoader, buildCandidatePath } from "../../cosmiconfig/src/index.cp";
export fn lilToLower(b: u8): u8;
export fn lilEqIgnoreCase(a: string, b: string): bool;
export fn lilEndsWithIgnoreCase(s: string, suffix: string): bool;
export fn lilWriteStr(out: []u8, pos: usize, s: string): usize;
export fn lilBasename(filepath: string): string;
export fn lilIsNoExt(filepath: string): bool;
export class LilconfigOptions {
    module_name: string,
    stop_dir: string,
    package_prop: string,
    ignore_empty_search_places: bool,
    fn defaults(module_name: string): LilconfigOptions;
}

export fn formatLilSearchPlace(module_name: string, idx: usize, out: []u8): usize;
export fn isSupportedByDefault(filepath: string): bool;
export class CustomLoaderEntry {
    ext: string,
    loader_id: u8,
}

export class LilExplorer {
    opts: LilconfigOptions,
    planner: SearchPlanner,
    custom_loaders: [8]CustomLoaderEntry,
    custom_count: usize,
    fn new(start_dir: string, opts: LilconfigOptions): LilExplorer;
    fn registerLoader(_: auto, ext: string, loader_id: u8): bool;
    fn resolveLoader(filepath: string): u8;
    fn nextSearchDir(_: auto): string?;
    fn candidatePath(dir: string, place_idx: usize, out: []u8): usize;
    fn makeResult(filepath: string, is_empty: bool): CosmiConfigResult;
}

