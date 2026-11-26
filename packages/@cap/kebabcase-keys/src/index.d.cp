// Type definitions for CP

import { MapObjResult } from "../../map-obj/src/index.cp";
import { DecamelizeKeysResult, decamelizeKeys } from "../../decamelize-keys/src/index.cp";
export fn kckCopySlice(src_buf: [1024]u8, off: usize, len: usize, out: []u8, dst_off: usize): usize;
export fn kebabcaseKeys(src: MapObjResult, exclude: []string): DecamelizeKeysResult;
export fn kebabcaseKeysAll(src: MapObjResult): DecamelizeKeysResult;
export fn kebabcaseKeyInto(key: string, out: []u8): usize;
export fn kebabcasePathInto(dot_path: string, out: []u8): usize;
export fn kebabcaseKeysBatch(items: []MapObjResult, exclude: []string, out: []DecamelizeKeysResult): usize;
