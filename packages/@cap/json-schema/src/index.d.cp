// Type definitions for CP

export enum JsonType {
    Any = 0,
    Null = 1,
    Boolean = 2,
    Integer = 3,
    Number = 4,
    String = 5,
    Array = 6,
    Object = 7
}

export class ValidationError {
    code: string,
    message: string,
    path: string,
    fn none(): ValidationError;
}

export class ValidationResult {
    valid: bool,
    error_count: usize,
    first_error: ValidationError,
    fn ok(): ValidationResult;
}

export class SchemaRule {
    expected_type: JsonType,
    has_min_len: bool,
    min_len: usize,
    has_max_len: bool,
    max_len: usize,
    has_min_val: bool,
    min_val: f64,
    has_max_val: bool,
    max_val: f64,
    exclusive_min: bool,
    exclusive_max: bool,
    required_keys: [8]string,
    required_count: usize,
    enum_values: [8]string,
    enum_count: usize,
    fn any(): SchemaRule;
    fn addRequired(_: auto, key: string);
    fn addEnum(_: auto, val: string);
    fn setExclusive(_: auto, min_excl: bool, max_excl: bool);
}

export fn stringRule(min_len: usize, max_len: usize): SchemaRule;
export fn numberRule(min_val: f64, max_val: f64): SchemaRule;
export fn integerRule(min_val: f64, max_val: f64): SchemaRule;
export fn arrayRule(min_items: usize, max_items: usize): SchemaRule;
export fn objectRule(): SchemaRule;
export fn isWs(c: u8): bool;
export fn isDigit(c: u8): bool;
export fn isHex(c: u8): bool;
export fn skipWs(s: string, start: usize): usize;
export fn eqStr(a: string, b: string): bool;
export fn skipJsonString(s: string, start: usize): usize?;
export class ParsedJsonNumber {
    ok: bool,
    is_int: bool,
    val: f64,
    next_pos: usize,
}

export fn pow10(exp: i32): f64;
export fn parseJsonNumber(s: string, start: usize): ParsedJsonNumber;
export fn skipJsonValue(s: string, start: usize, depth: usize): usize?;
export fn countJsonStringChars(s: string, start: usize, end: usize): usize;
export fn countArrayItems(s: string, start: usize): usize;
export fn detectJsonType(json: string): JsonType;
export fn addErr(res: ValidationResult, code: string, msg: string, path: string): ValidationResult;
export fn validateJson(json: string, rule: SchemaRule): ValidationResult;
