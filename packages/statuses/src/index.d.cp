// Type definitions for CP

import { eqIgnoreCase } from "cap:ascii";
export fn message(code: u16): string;
export fn codeFromMessage(msg: string): u16;
export fn isValid(code: u16): bool;
export fn isInformational(code: u16): bool;
export fn isSuccess(code: u16): bool;
export fn isRedirect(code: u16): bool;
export fn isEmpty(code: u16): bool;
export fn isRetry(code: u16): bool;
export fn isClientError(code: u16): bool;
export fn isServerError(code: u16): bool;
