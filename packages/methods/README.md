[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# methods

Zero-heap HTTP method registry and RFC 9110 / WebDAV semantic predicates with O(log n) case-insensitive lookup over all 34 standard HTTP methods, matching npm [`methods`](https://github.com/jshttp/methods) (120M+/wk).

## Source Alignment

- **Upstream**: npm `methods` (`jshttp/methods`, MIT — core dependency of Express / Koa)
- **CP Package**: `methods` v1.0.0 (clean-room MIT in pure CP)
- **Design**: Zero heap allocations; compile-time sorted `[34]string` tables (`METHODS` lowercase & `METHODS_UPPER` uppercase) with binary search and RFC 9110 §9.2 / WebDAV predicates.

## Installation & Import

```bash
cap add methods
```

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

## Core API

### Constants

| Symbol | Type | Description |
| :--- | :--- | :--- |
| `HTTP_METHOD_COUNT` | `const HTTP_METHOD_COUNT: usize` (`34`) | Total number of supported HTTP / WebDAV methods |
| `METHODS` | `const METHODS: [34]string` | Ascending sorted lowercase method names (`"acl"` .. `"unsubscribe"`) |
| `METHODS_UPPER` | `const METHODS_UPPER: [34]string` | Ascending sorted UPPERCASE method names (`"ACL"` .. `"UNSUBSCRIBE"`) |

### Lookup & Normalization Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `findMethodIndex` | `fn findMethodIndex(method: string): i32` | Case-insensitive binary search returning `0..33`, or `-1` if unknown |
| `isHttpMethod` | `fn isHttpMethod(method: string): bool` | Returns `true` if `method` is one of the 34 standard methods (case-insensitive) |
| `toLowerMethod` / `methodToLower` | `fn toLowerMethod(method: string): string` | Returns static canonical lowercase name (e.g. `"get"`), or `""` if unknown |
| `toUpperMethod` / `methodToUpper` | `fn toUpperMethod(method: string): string` | Returns static canonical UPPERCASE name (e.g. `"GET"`), or `""` if unknown |
| `methodAt` | `fn methodAt(idx: usize): string` | Returns lowercase method at `idx` (`0..33`), or `""` if out of bounds |
| `methodUpperAt` | `fn methodUpperAt(idx: usize): string` | Returns UPPERCASE method at `idx` (`0..33`), or `""` if out of bounds |
| `hmCmpLower` | `fn hmCmpLower(input: string, lower_target: string): i32` | Case-insensitive ASCII comparator against a lowercase target (`-1`, `0`, `1`) |

### RFC 9110 & WebDAV Semantic Predicates

| Function | Signature | Description |
| :--- | :--- | :--- |
| `isSafeMethod` | `fn isSafeMethod(method: string): bool` | RFC 9110 §9.2.1 read-only safe methods (`GET`, `HEAD`, `OPTIONS`, `TRACE`, `PROPFIND`, `REPORT`, `SEARCH`) |
| `isIdempotentMethod` | `fn isIdempotentMethod(method: string): bool` | RFC 9110 §9.2.2 idempotent methods (safe + `PUT`, `DELETE`, `COPY`, `MOVE`, `MKCOL`, `PROPPATCH`, `LOCK`, `UNLOCK`, `UNLINK`) |
| `isCacheableMethod` | `fn isCacheableMethod(method: string): bool` | RFC 9110 §9.2.3 cacheable methods (`GET`, `HEAD`, `POST`) |
| `mayHaveRequestBody` | `fn mayHaveRequestBody(method: string): bool` | True for methods that conventionally carry a request body (`POST`, `PUT`, `PATCH`, etc.) |
| `isStandardRfc9110Method` | `fn isStandardRfc9110Method(method: string): bool` | True for the 9 core RFC 9110 / RFC 5789 methods (`GET`, `HEAD`, `POST`, `PUT`, `DELETE`, `CONNECT`, `OPTIONS`, `TRACE`, `PATCH`) |
| `isWebDavMethod` | `fn isWebDavMethod(method: string): bool` | True for WebDAV / CalDAV / Versioning extension methods |
| `allowsResponseBody` | `fn allowsResponseBody(method: string): bool` | True for any valid HTTP method permitted to return a response body (all except `HEAD`) |

## Examples

### 1. Case-Insensitive Validation & Canonicalization

```cp
import { isHttpMethod, toUpperMethod, toLowerMethod } from "methods";
import { print } from "cap:io";

pub fn main() {
    if (isHttpMethod("pAtCh")) {
        print(toUpperMethod("pAtCh")); // "PATCH"
        print(toLowerMethod("PATCH")); // "patch"
    }
}
```

### 2. HTTP Router & Retry Policy Predicates

```cp
import { isSafeMethod, isIdempotentMethod, allowsResponseBody } from "methods";

pub fn canRetryRequest(method: string): bool {
    return isSafeMethod(method) || isIdempotentMethod(method);
}

pub fn shouldWriteBody(method: string): bool {
    return allowsResponseBody(method); // false for "HEAD" or unknown methods
}
```

### 3. Iterating All Registered Methods

```cp
import { HTTP_METHOD_COUNT, methodAt } from "methods";
import { print } from "cap:io";

pub fn listMethods() {
    let i: usize = 0;
    while (i < HTTP_METHOD_COUNT) {
        print(methodAt(i));
        i += 1;
    }
}
```

## Differences from npm `methods` & Safety Highlights

- **Zero Allocation**: All canonical strings (`toLowerMethod`, `toUpperMethod`, `methodAt`) return compile-time static string slices from `METHODS` / `METHODS_UPPER`.
- **O(log n) Binary Search**: Case-insensitive lookup without allocating a lowercased temporary copy.
- **RFC 9110 & WebDAV Predicates**: Built-in `isSafeMethod`, `isIdempotentMethod`, `isCacheableMethod`, `mayHaveRequestBody`, `isStandardRfc9110Method`, `isWebDavMethod`, and `allowsResponseBody`.
