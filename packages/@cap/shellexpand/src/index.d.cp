// Type definitions for CP

export class EnvEntry {
    key: string,
    value: string,
}

export fn sexpIsVarStart(b: u8): bool;
export fn sexpIsVarChar(b: u8): bool;
export fn sexpStrEq(a: string, b: string): bool;
export fn sexpLookup(vars: &[]EnvEntry, key: string): string?;
export fn sexpAppend(out: []u8, pos: usize, s: string): usize;
export fn tildeWithHome(input: string, home_dir: string, out: []u8): usize;
export fn envExpand(input: string, vars: &[]EnvEntry, keep_unknown: bool, out: []u8): usize;
export fn fullExpand(input: string, home_dir: string, vars: &[]EnvEntry, out: []u8): usize;
