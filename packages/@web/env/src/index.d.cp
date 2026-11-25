// Type definitions for CP

import { App } from "../../../../lib/web/app.cp";
export enum EnvKind {
    Str,
    Int,
    Bool,
    Enum
}

export class EnvVarSpec {
    name: string,
    kind: EnvKind,
    required: bool,
    default_val: string,
    min_val: i64,
    max_val: i64,
    has_bounds: bool,
    enum_values: string,
}

export class EnvKV {
    key: string,
    value: string,
}

export class EnvValidationResult {
    ok: bool,
    missing_count: usize,
    invalid_count: usize,
    loaded_count: usize,
    first_error_key: string,
}

export fn envStrEql(a: string, b: string): bool;
export fn envParseI64(s: string): ?i64;
export fn envIsValidBool(s: string): bool;
export fn envInEnum(val: string, list: string): bool;
export fn envLookupRaw(entries: &[]EnvKV, key: string): ?string;
export fn registerEnv(app: &mut App, specs: &[]EnvVarSpec, entries: &[]EnvKV): EnvValidationResult;
