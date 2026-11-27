// Type definitions for CP

export class TargetTriple {
    arch_start: usize,
    arch_len: usize,
    vendor_start: usize,
    vendor_len: usize,
    os_start: usize,
    os_len: usize,
    env_start: usize,
    env_len: usize,
    has_env: bool,
    valid: bool,
}

export class TargetEnv {
    arch: string,
    os: string,
    family: string,
    env: string,
    pointer_width: u32,
    is_big_endian: bool,
}

export class CfgEvalResult {
    matched: bool,
    ok: bool,
}

export fn invalidTriple(): TargetTriple;
export fn parseTargetTriple(triple: string): TargetTriple;
export class CfgParser {
    pos: usize,
    ok: bool,
    depth: usize,
}

export fn isWs(c: u8): bool;
export fn isIdentStart(c: u8): bool;
export fn isIdentCont(c: u8): bool;
export fn skipWs(p: &mut CfgParser, src: string);
export fn strEq(a: string, b: string): bool;
export fn u32MatchesDecimal(n: u32, s: string): bool;
export fn evalKeyValue(key: string, val: string, target: TargetEnv): bool;
export fn evalFlag(flag: string, target: TargetEnv): bool;
export fn parsePredicate(p: &mut CfgParser, src: string, target: TargetEnv): bool;
export fn evalCfgExpr(expr: string, target: TargetEnv): CfgEvalResult;
