// Type definitions for CP

import { MapObjResult } from "../../map-obj/src/index.cp";
import { DecamelizeKeysResult, decamelizeKeys } from "../../decamelize-keys/src/index.cp";

export fn kebabcaseString(input: string, out: []u8): usize;
export fn isKebabCase(s: string): bool;
export fn kebabcaseKeys(src: MapObjResult, exclude: []string): DecamelizeKeysResult;
export fn kebabcaseKeysAll(src: MapObjResult): DecamelizeKeysResult;
export fn kebabcaseEntries(keys: &[]string, vals: &[]string, exclude: &[]string): DecamelizeKeysResult;
export fn kebabcaseKeysDeep(src: MapObjResult, exclude: &[]string, stop_paths: &[]string): DecamelizeKeysResult;
export fn kebabcaseKeyInto(key: string, out: []u8): usize;
export fn kebabcasePathInto(dot_path: string, out: []u8): usize;
export fn kebabcaseKeysBatch(items: []MapObjResult, exclude: []string, out: []DecamelizeKeysResult): usize;
