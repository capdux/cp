// Type definitions for CP

import { strEq } from "cap:ascii";
import { writeStr } from "cap:fmt";
import { FindUpOptions, FindUpWalker, joinDirAndName } from "../../find-up/src/index.cp";
import { dirnameOf } from "../../npm-run-path/src/index.cp";
export fn rfExists(path: string, files: []string): bool;
export fn isRelativeSpecifier(spec: string): bool;
export fn isScopedSpecifier(spec: string): bool;
export fn rfProbeBase(base: string, files: []string, out: []u8): usize?;
export fn rfResolveRelativeBase(from_dir: string, rel: string, out: []u8): usize;
export fn resolveFrom(from_dir: string, module_id: string, existing_files: []string, out: []u8): usize?;
export class ResolveResult {
    found: bool,
    len: usize,
    err_code: i32,
}

export class ResolveOptions {
    extensions: [8]string,
    ext_count: usize,
    include_node_modules: bool,
    include_cap_modules: bool,
    fn defaults(): ResolveOptions;
}

export fn isBareSpecifier(spec: string): bool;
export fn resolveFromSilent(from_dir: string, module_id: string, existing_files: []string, out: []u8): usize?;
export fn rfProbeBaseOpts(base: string, files: []string, opts: ResolveOptions, out: []u8): ResolveResult;
export fn resolveFromWithOptions(from_dir: string, module_id: string, existing_files: []string, opts: ResolveOptions, out: []u8): ResolveResult;
