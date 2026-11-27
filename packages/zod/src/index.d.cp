// Type definitions for CP

import { isSpace, isDigit, isAlpha, isHexDigit, strEq, trim } from "cap:ascii";
import { kindOf, KIND_INVALID, KIND_NULL, KIND_UNDEFINED, KIND_BOOLEAN, KIND_NUMBER, KIND_STRING, KIND_ARRAY, KIND_OBJECT, KIND_DATE } from "../../kind-of/src/index.cp";
import { WhatwgUrl } from "../../url/src/index.cp";
export fn zdIsEmailLocalChar(c: u8): bool;
export fn zdIsDomainLabelChar(c: u8): bool;
export fn zdParseNumber(raw: string, out_val: &mut f64, out_is_int: &mut bool): bool;
export fn zdDecodeJsonString(raw: string, out: []u8, out_total_len: &mut usize): bool;
export fn zdCountJsonArrayItems(raw: string, out_count: &mut usize): bool;
export fn zString(raw: string, min_len: usize, max_len: usize): bool;
export fn zNumber(raw: string, min_v: f64, max_v: f64): bool;
export fn zEmail(s: string): bool;
export fn zUrl(s: string): bool;
export fn zUuid(s: string): bool;
export fn zEnum(s: string, csv: string): bool;
export class ZFieldRule {
    name: string,
    kind: u8,
    required: bool,
    min_val: f64,
    max_val: f64,
    has_min: bool,
    has_max: bool,
    min_len: usize,
    max_len: usize,
    has_min_len: bool,
    has_max_len: bool,
    enum_csv: string,
    fn empty(): ZFieldRule;
    fn basic(name: string, kind: u8, required: bool): ZFieldRule;
    fn strRange(name: string, required: bool, min_len: usize, max_len: usize): ZFieldRule;
    fn numRange(name: string, kind: u8, required: bool, min_val: f64, max_val: f64): ZFieldRule;
    fn enumRule(name: string, required: bool, enum_csv: string): ZFieldRule;
}

export class ZIssue {
    field: [48]u8,
    field_len: usize,
    code: i32,
    fn empty(): ZIssue;
    fn fieldName(): &[]u8;
}

export class ZParseResult {
    ok: bool,
    issue_count: usize,
    issues: [16]ZIssue,
    fn empty(): ZParseResult;
    fn addIssue(_: auto, field_name: string, code: i32);
    fn hasIssue(field_name: string, code: i32): bool;
}

export fn zdValidateFieldValue(rule: ZFieldRule, raw_val: string, res: &mut ZParseResult);
export fn zdScanJsonValue(json: string, pos: usize, end: usize, out_end: &mut usize): bool;
export class ZObjectSchema {
    rules: [24]ZFieldRule,
    rule_count: usize,
    fn new(): ZObjectSchema;
    fn field(_: auto, rule: ZFieldRule): bool;
    fn safeParseJson(json: string): ZParseResult;
}

