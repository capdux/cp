// Type definitions for CP

import { MapObjResult, MAP_OBJ_MAX_ENTRIES } from "../../map-obj/src/index.cp";
import { decamelize } from "../../decamelize/src/index.cp";
export class DecamelizeKeysResult {
    key_buf: [1024]u8,
    key_buf_len: usize,
    key_offs: [32]usize,
    key_lens: [32]usize,
    values: [32]string,
    active: [32]bool,
    count: usize,
    fn empty(): DecamelizeKeysResult;
    fn len(): usize;
    fn get(k: string): string?;
    fn pushRaw(_: auto, k: string, v: string): bool;
    fn pushDecamelized(_: auto, k: string, sep: string, v: string): bool;
}

export fn dckStrEq(a: string, b: string): bool;
export fn dckIsExcluded(k: string, exclude: []string): bool;
export fn decamelizeKeys(src: MapObjResult, sep: string, exclude: []string): DecamelizeKeysResult;
