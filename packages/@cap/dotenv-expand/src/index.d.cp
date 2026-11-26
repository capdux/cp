// Type definitions for CP

export class EnvVar {
    key: &[]u8,
    val: &[]u8,
}

export fn sliceEq(a: &[]u8, b: &[]u8): bool;
export fn isIdentStart(b: u8): bool;
export fn isIdentChar(b: u8): bool;
export fn hasVar(vars: &[]EnvVar, key: &[]u8): bool;
export fn lookupVar(vars: &[]EnvVar, key: &[]u8): &[]u8;
export fn expandRec(tmpl: &[]u8, vars: &[]EnvVar, dst: []u8, start_out: usize, depth: usize): i64;
export fn expandValue(template: &[]u8, vars: &[]EnvVar, dst: []u8): i64;
