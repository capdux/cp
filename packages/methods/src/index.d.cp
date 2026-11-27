// Type definitions for CP

import { toLowerByte } from "cap:ascii";

export const HTTP_METHOD_COUNT: usize;
export const METHODS: [34]string;
export const METHODS_UPPER: [34]string;

export fn hmCmpLower(input: string, lower_target: string): i32;
export fn findMethodIndex(method: string): i32;
export fn isHttpMethod(method: string): bool;
export fn toLowerMethod(method: string): string;
export fn methodToLower(method: string): string;
export fn toUpperMethod(method: string): string;
export fn methodToUpper(method: string): string;
export fn methodAt(idx: usize): string;
export fn methodUpperAt(idx: usize): string;
export fn isSafeMethod(method: string): bool;
export fn isIdempotentMethod(method: string): bool;
export fn isCacheableMethod(method: string): bool;
export fn mayHaveRequestBody(method: string): bool;
export fn isStandardRfc9110Method(method: string): bool;
export fn isWebDavMethod(method: string): bool;
export fn allowsResponseBody(method: string): bool;
