// Type definitions for CP

import { strEq } from "cap:ascii";
export fn add64(a: u64, b: u64): u64;
export fn mul64(a: u64, b: u64): u64;
export fn mix64(z_in: u64): u64;
export class PhfHashes {
    g: u32,
    f1: u32,
    f2: u32,
}

export class PhfBuildResult {
    ok: bool,
    seed: u64,
    table_len: usize,
    disp_len: usize,
}

export fn hashKey(key: string, seed: u64): PhfHashes;
export fn displace(f1: u32, f2: u32, d1: u32, d2: u32, len: usize): usize;
export fn buildPhfWithSeed(keys: &[]string, start_seed: u64, disps_d1: []u32, disps_d2: []u32, slot_to_key: []i32): PhfBuildResult;
export fn buildPhf(keys: &[]string, disps_d1: []u32, disps_d2: []u32, slot_to_key: []i32): PhfBuildResult;
export fn lookupIndex(key: string, seed: u64, keys: &[]string, disps_d1: &[]u32, disps_d2: &[]u32, slot_to_key: &[]i32): i64;
