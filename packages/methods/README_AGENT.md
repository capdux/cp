# methods — AI Agent Quick Reference

Zero-heap HTTP method registry (34 RFC 9110 / WebDAV methods) with O(log n) lookup and semantic predicates (npm `methods` parity).

## Import

```cp
import {
    HTTP_METHOD_COUNT,
    METHODS,
    METHODS_UPPER,
    hmCmpLower,
    findMethodIndex,
    isHttpMethod,
    toLowerMethod,
    methodToLower,
    toUpperMethod,
    methodToUpper,
    methodAt,
    methodUpperAt,
    isSafeMethod,
    isIdempotentMethod,
    isCacheableMethod,
    mayHaveRequestBody,
    isStandardRfc9110Method,
    isWebDavMethod,
    allowsResponseBody,
} from "methods";
```

## API Signatures

```cp
const HTTP_METHOD_COUNT: usize; // 34
const METHODS: [34]string;      // sorted lowercase
const METHODS_UPPER: [34]string;// sorted UPPERCASE

fn hmCmpLower(input: string, lower_target: string): i32;
fn findMethodIndex(method: string): i32; // 0..33 or -1
fn isHttpMethod(method: string): bool;
fn toLowerMethod(method: string): string;
fn methodToLower(method: string): string;
fn toUpperMethod(method: string): string;
fn methodToUpper(method: string): string;
fn methodAt(idx: usize): string;
fn methodUpperAt(idx: usize): string;
fn isSafeMethod(method: string): bool;
fn isIdempotentMethod(method: string): bool;
fn isCacheableMethod(method: string): bool;
fn mayHaveRequestBody(method: string): bool;
fn isStandardRfc9110Method(method: string): bool;
fn isWebDavMethod(method: string): bool;
fn allowsResponseBody(method: string): bool;
```

## Minimal Example

```cp
import { isHttpMethod, toUpperMethod, isIdempotentMethod } from "methods";

const ok = isHttpMethod("delete");             // true
const canon = toUpperMethod("delete");         // "DELETE"
const retryable = isIdempotentMethod("DELETE");// true
```

## Gotchas & Borrow Rules

- All lookup & predicate functions are case-insensitive and zero-allocation; returned strings are static literals (`""` on unknown method or out-of-bounds index).
- `findMethodIndex` returns `-1` (`i32`) when unknown — check `idx >= 0` before casting to `usize`.
- `allowsResponseBody("HEAD")` and `allowsResponseBody("UNKNOWN")` both return `false`.
