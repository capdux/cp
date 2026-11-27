// Type definitions for CP

import { FindUpOptions, FindUpWalker, joinDirAndName } from "../../find-up/src/index.cp";
import { dirnameOf } from "../../npm-run-path/src/index.cp";
export fn rfStrEq(a: string, b: string): bool;
export fn rfPush(out: []u8, pos: usize, s: string): usize;
export fn rfExists(path: string, files: []string): bool;
export fn isRelativeSpecifier(spec: string): bool;
export fn isScopedSpecifier(spec: string): bool;
export fn rfProbeBase(base: string, files: []string, out: []u8): usize?;
export fn rfResolveRelativeBase(from_dir: string, rel: string, out: []u8): usize;
export fn resolveFrom(from_dir: string, module_id: string, existing_files: []string, out: []u8): usize?;
