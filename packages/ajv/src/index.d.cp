// Type definitions for CP

import { zEmail, zUrl, zUuid, zEnum } from "../../zod-schema/src/index.cp";
import { isValidIpv4, isValidIpv6 } from "../../ipaddr/src/index.cp";
export fn ajvIsSpace(c: u8): bool;
export fn ajvIsDigit(c: u8): bool;
export fn ajvStrEq(a: string, b: string): bool;
export fn ajvSliceEqStr(a: &[]u8, b: string): bool;
export fn ajvTrimBounds(s: string, out_start: &mut usize, out_end: &mut usize);
export fn ajvParseNumber(raw: string, out_val: &mut f64, out_is_int: &mut bool): bool;
export fn ajvDecodeJsonString(raw: string, out: []u8, out_len: &mut usize): bool;
export fn ajvScanValue(s: string, pos: usize, end: usize, out_end: &mut usize): bool;
export fn ajvClassifyStrict(raw: string, depth: usize): u8;
export fn ajvFindObjectProp(obj: string, target_key: string, out_start: &mut usize, out_end: &mut usize): bool;
export fn ajvCountArrayItems(arr: string): usize;
export fn ajvJsonValuesEqual(raw_a: string, raw_b: string): bool;
export fn ajvIsIsoDate(s: string): bool;
export fn validateFormat(fmt: string, value: string): bool;
export fn ajvMatchSingleType(type_name: string, data_kind: u8, data_raw: string): bool;
export fn ajvJoinPath(base: string, seg: string, out: []u8): usize;
export class AjvError {
    instance_path: [64]u8,
    instance_path_len: usize,
    keyword: [32]u8,
    keyword_len: usize,
    code: i32,
    fn empty(): AjvError;
    fn pathStr(): &[]u8;
    fn keywordStr(): &[]u8;
}

export class AjvResult {
    valid: bool,
    error_count: usize,
    errors: [16]AjvError,
    fn ok(): AjvResult;
    fn addError(_: auto, path: string, keyword: string, code: i32);
    fn hasError(path: string, keyword: string, code: i32): bool;
    fn hasCode(code: i32): bool;
}

export fn ajvValidateNode(schema: string, data: string, path: string, depth: usize, res: &mut AjvResult);
export class AjvSchema {
    source: string,
    ok: bool,
    fn compile(schema_json: string): AjvSchema;
    fn validate(data_json: string): AjvResult;
}

export fn validateWithSchema(schema_json: string, data_json: string): AjvResult;
