// Type definitions for CP

import { strEq, isAlpha, isDigit } from "cap:ascii";
import { writeStr } from "cap:fmt";
export fn isVarStart(c: u8): bool;
export fn isVarCont(c: u8): bool;
export class EnvMap {
    keys: [MAX_ENV_VARS]string,
    vals: [MAX_ENV_VARS]string,
    len: usize,
    fn init(): EnvMap;
    fn set(_: auto, key: string, val: string);
    fn get(key: string): string?;
    fn has(key: string): bool;
}

export enum ExpandError {
    None,
    UnclosedBrace,
    MissingRequired,
    BufferOverflow
}

export class ExpandResult {
    ok: bool,
    err: ExpandError,
    len: usize,
    missing_var: string,
}

export fn expandImpl(tpl: string, env: EnvMap, out: []u8, start_out: usize, depth: u8): ExpandResult;
export fn expandEnv(tpl: string, env: EnvMap, out: []u8): ExpandResult;
export fn expandEnvStr(tpl: string, env: EnvMap, out: []u8): string;
export fn hasEnvVars(tpl: string): bool;
