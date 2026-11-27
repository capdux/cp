// Type definitions for CP

import { eqAscii } from "../../unicase/src/index.cp";
import { wildmatchIgnoreCase } from "../../wildmatch/src/index.cp";
export class CorsOptions {
    allow_any_origin: bool,
    reflect_origin: bool,
    allowed_origins: [16]string,
    allowed_origin_count: usize,
    allowed_methods: string,
    allowed_headers: string,
    exposed_headers: string,
    allow_credentials: bool,
    max_age: u32,
    has_max_age: bool,
    fn permissive(): CorsOptions;
    fn restrictive(): CorsOptions;
    fn addOrigin(_: auto, pattern: string): bool;
}

export class CorsDecision {
    allowed: bool,
    is_preflight: bool,
    allow_origin: string,
    vary_origin: bool,
    allow_credentials: bool,
    allow_methods: string,
    allow_headers: string,
    expose_headers: string,
    max_age: u32,
    has_max_age: bool,
}

export fn corsPush(out: []u8, pos: usize, s: string): usize;
export fn corsPushU32(out: []u8, pos: usize, v: u32): usize;
export fn corsIsWs(b: u8): bool;
export fn corsTrim(s: string): string;
export fn isOriginAllowed(opts: CorsOptions, request_origin: string): bool;
export fn isMethodAllowed(allowed_methods: string, method: string): bool;
export fn evaluateCors(opts: CorsOptions, method: string, origin: string, ac_req_method: string, ac_req_headers: string): CorsDecision;
export fn formatCorsHeaders(d: CorsDecision, out: []u8): usize;
