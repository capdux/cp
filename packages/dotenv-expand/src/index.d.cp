// Type definitions for CP

import { isAlnum, strEq, trim, startsWith, findByte } from "cap:ascii";
import { writeStr } from "cap:fmt";
export class EnvEntry {
    key: string,
    value: string,
}

export class EnvMap {
    entries: [32]EnvEntry,
    count: usize,
    fn new(): EnvMap;
    fn len(): usize;
    fn set(_: auto, key: string, val: string): bool;
    fn get(key: string): string?;
    fn has(key: string): bool;
}

export fn deIsVarChar(b: u8): bool;
export fn parseDotenv(src: string): EnvMap;
export fn expandEnvValue(template: string, env: EnvMap, out: []u8): usize;
