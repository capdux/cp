// Type definitions for CP

import { FindUpOptions, findUpInList, findUpMultipleInList } from "../../find-up/src/index.cp";
import { dirnameOf } from "../../npm-run-path/src/index.cp";
export fn pkdPush(out: []u8, pos: usize, s: string): usize;
export fn packageDirectoryInList(cwd: string, existing_files: []string, out: []u8): usize?;
export fn capPackageDirectoryInList(cwd: string, existing_files: []string, out: []u8): usize?;
export fn packageDirectoryBounded(cwd: string, stop_at: string, existing_files: []string, out: []u8): usize?;
