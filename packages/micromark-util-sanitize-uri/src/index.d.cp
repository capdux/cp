// Type definitions for CP

import { toLowerByte, strEq, isHexDigit, isAlnum, hexCharUpper } from "cap:ascii";
export fn surIsSafeUriByte(b: u8): bool;
export fn isSafeUri(uri: string): bool;
export fn sanitizeUri(uri: string, out: []u8): usize;
