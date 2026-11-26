// Type definitions for CP

import { MapObjResult, MAP_OBJ_MAX_ENTRIES } from "../../map-obj/src/index.cp";
import { toSnakeCase } from "../../heck-case/src/index.cp";
export class SnakecaseKeysResult {
    key_buf: [1024]u8,
    key_buf_len: usize,
    key_offs: [32]usize,
    key_lens: [32]usize,
    values: [32]string,
    active: [32]bool,
    count: usize,
    fn empty(): SnakecaseKeysResult;
    fn len(): usize;
    fn get(k: string): string?;
    fn pushRaw(_: auto, k: string, v: string): bool;
    fn pushSnake(_: auto, k: string, v: string): bool;
}

export fn sckStrEq(a: string, b: string): bool;
export fn sckIsExcluded(k: string, exclude: []string): bool;
export fn snakecaseKeyInto(key: string, out: []u8): usize;
export fn snakecaseKeys(src: MapObjResult, exclude: []string): SnakecaseKeysResult;
