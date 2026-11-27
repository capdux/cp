// Type definitions for CP

import { toLowerByte } from "cap:ascii";
export enum StatusClass {
    Invalid = 0,
    Informational = 1,
    Success = 2,
    Redirection = 3,
    ClientError = 4,
    ServerError = 5
}

export fn reasonPhrase(code: u16): string;
export fn isSep(c: u8): bool;
export fn eqNormalizedPhrase(input: string, target: string): bool;
export fn codeFromPhrase(phrase: string): u16?;
export fn statusClass(code: u16): StatusClass;
export fn isValidStatus(code: u16): bool;
export fn isKnownStatus(code: u16): bool;
export fn isInformational(code: u16): bool;
export fn isSuccess(code: u16): bool;
export fn isRedirect(code: u16): bool;
export fn isClientError(code: u16): bool;
export fn isServerError(code: u16): bool;
export fn isEmptyBody(code: u16): bool;
export fn isRetryable(code: u16): bool;
export fn isCacheableByDefault(code: u16): bool;
