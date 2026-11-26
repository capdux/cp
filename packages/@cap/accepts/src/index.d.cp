// Type definitions for CP

import { bestEncoding, bestMediaType, bestLanguage, parseAcceptList } from "../../negotiator/src/index.cp";
export fn acceptsIsSpace(c: u8): bool;
export fn acceptsTrimSlice(s: &[]u8): &[]u8;
export fn parseQValue(s_in: &[]u8): i32;
export fn negotiateEncoding(accept_header: &[]u8, available: &[]&[]u8): i64;
export fn negotiateLanguage(accept_header: &[]u8, available: &[]&[]u8): i64;
export fn negotiateMediaType(accept_header: &[]u8, available: &[]&[]u8): i64;
