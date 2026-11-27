// Type definitions for CP

import { toLowerByte, isSpace, eqIgnoreCase, trim } from "cap:ascii";
export fn findSchemeSep(s: string): i64;
export fn isValidOrigin(origin: string): bool;
export fn matchOrigin(origin: string, pattern: string, allow_credentials: bool): bool;
export fn matchOriginList(origin: string, patterns: &[]string, allow_credentials: bool): bool;
export fn isMethodAllowed(method: string, allowed_methods: string): bool;
export fn areHeadersAllowed(req_headers: string, allowed_headers: string): bool;
export fn isSimpleMethod(method: string): bool;
