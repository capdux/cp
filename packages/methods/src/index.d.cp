// Type definitions for CP

import { toLowerByte } from "cap:ascii";
export fn hmCmpLower(input: string, lower_target: string): i32;
export fn findMethodIndex(method: string): i32;
export fn isHttpMethod(method: string): bool;
export fn toLowerMethod(method: string): string;
export fn toUpperMethod(method: string): string;
export fn methodAt(idx: usize): string;
export fn isSafeMethod(method: string): bool;
export fn isIdempotentMethod(method: string): bool;
export fn isCacheableMethod(method: string): bool;
export fn mayHaveRequestBody(method: string): bool;
