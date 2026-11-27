// Type definitions for CP

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

export fn deStrEq(a: string, b: string): bool;
export fn deIsWs(b: u8): bool;
export fn deIsVarChar(b: u8): bool;
export fn deTrim(s: string): string;
export fn dePush(out: []u8, pos: usize, s: string): usize;
export fn parseDotenv(src: string): EnvMap;
export fn expandEnvValue(template: string, env: EnvMap, out: []u8): usize;
