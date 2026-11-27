[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# depd

Zero-heap deprecation notice formatter and deduplicated warning tracker for CP, matching npm [`depd`](https://github.com/dougwilson/nodejs-depd) (150M+/wk).

## Source Alignment

- **Upstream**: npm `depd` (`dougwilson/nodejs-depd`, MIT)
- **CP Package**: `depd` v1.0.0 (clean-room MIT in pure CP, reusing `@sindresorhus/fnv1a` for 64-bit FNV-1a deduplication hashing)
- **Design**: Zero dynamic allocation — all formatted notices are written into caller-owned `[]u8` buffers, and deduplication uses a fixed 64-slot stack table (`MAX_SEEN_HASHES = 64`).

## Installation & Import

```bash
cap add depd
```

```cp
import {
    MAX_SEEN_HASHES,
    DepdOptions,
    Deprecator,
    isNamespaceSilenced,
    isNamespaceTraced,
    formatDeprecationLen,
    formatDeprecation,
    formatDeprecationWithTrace,
    depdHashKey,
} from "depd";
```

## Core API

### Types & Constants

| Symbol | Definition | Description |
| :--- | :--- | :--- |
| `MAX_SEEN_HASHES` | `const MAX_SEEN_HASHES: usize` (`64`) | Maximum unique `(namespace, message)` hashes cached per `Deprecator` instance |
| `DepdOptions` | `struct { no_deprecation: bool, trace_deprecation: bool }` | Configuration struct; call `DepdOptions.defaults()` for default (`false, false`) |
| `Deprecator` | `struct { namespace, silenced, no_deprecation, trace_deprecation, seen_hashes: [64]u64, seen_count, emitted_count }` | Stateful deduplicated deprecation emitter for a namespace |

### `Deprecator` Methods

| Method | Signature | Description |
| :--- | :--- | :--- |
| `Deprecator.new` | `fn new(namespace: string): Deprecator` | Create a new `Deprecator` with default options |
| `Deprecator.withOptions` | `fn withOptions(namespace: string, opts: DepdOptions): Deprecator` | Create a `Deprecator` with explicit `no_deprecation` / `trace_deprecation` flags |
| `setSilenced` | `fn setSilenced(*_, flag: bool)` | Toggle runtime silencing (requires mutable receiver) |
| `setNoDeprecation` | `fn setNoDeprecation(*_, flag: bool)` | Toggle `NO_DEPRECATION` suppression |
| `setTraceDeprecation` | `fn setTraceDeprecation(*_, flag: bool)` | Toggle `TRACE_DEPRECATION` stack trace inclusion |
| `isSilenced` | `fn isSilenced(): bool` | Returns `true` if `silenced \|\| no_deprecation` |
| `isTraceEnabled` | `fn isTraceEnabled(): bool` | Returns `true` if `trace_deprecation` is enabled |
| `seenCount` | `fn seenCount(): usize` | Number of unique warning hashes recorded so far (up to `64`) |
| `hasWarned` | `fn hasWarned(msg: string): bool` | Read-only check whether `msg` has already been emitted for this namespace |
| `warnOnce` | `fn warnOnce(*_, msg: string, site: string, out: []u8): usize` | Format notice into `out` on first occurrence; returns `0` if silenced or duplicate |
| `warnWithTrace` | `fn warnWithTrace(*_, msg: string, site: string, trace: string, out: []u8): usize` | Deduplicated warning that appends `\n<trace>` when `trace_deprecation` is enabled |
| `deprecate` | `fn deprecate(*_, msg: string, site: string, out: []u8): usize` | Alias for `warnOnce` |
| `deprecateFn` | `fn deprecateFn(*_, fn_name: string, msg: string, site: string, out: []u8): usize` | Emit deprecation for a function name (uses `fn_name` when `msg` is empty) |
| `reset` | `fn reset(*_)` | Clear deduplication table and reset `seen_count` / `emitted_count` to `0` |

### Free Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `isNamespaceSilenced` | `fn isNamespaceSilenced(namespace: string, no_deprecation_env: string): bool` | Match `namespace` against comma-separated `NO_DEPRECATION` env value (supports `*`) |
| `isNamespaceTraced` | `fn isNamespaceTraced(namespace: string, trace_deprecation_env: string): bool` | Match `namespace` against comma-separated `TRACE_DEPRECATION` env value (supports `*`) |
| `formatDeprecationLen` | `fn formatDeprecationLen(namespace: string, msg: string, site: string): usize` | Compute exact byte length of `<namespace> deprecated <msg>[ at <site>]` |
| `formatDeprecation` | `fn formatDeprecation(namespace: string, msg: string, site: string, out: []u8): usize` | Format notice without deduplication into `out`; returns bytes written |
| `formatDeprecationWithTrace` | `fn formatDeprecationWithTrace(namespace: string, msg: string, site: string, trace: string, include_trace: bool, out: []u8): usize` | Format notice with optional trailing `\n<trace>` into `out` |
| `depdHashKey` | `fn depdHashKey(ns: string, msg: string): u64` | Compute 64-bit FNV-1a deduplication key for `(ns, msg)` |
| `copySlice` | `fn copySlice(dst: []u8, pos: usize, src: string): usize` | Bounded helper copying `src` into `dst[pos..]` and returning the new offset |

## Examples

### 1. Deduplicated Warning Emission

```cp
import { Deprecator } from "depd";
import { print } from "cap:io";

pub fn main() {
    let dep = Deprecator.new("my-http");
    let buf: [256]u8 = [0; 256];

    const n1 = dep.deprecate("req.host is deprecated; use req.hostname", "router.cp:42", buf[0..256]);
    if (n1 > 0) {
        print(buf[0..n1] as string);
        // "my-http deprecated req.host is deprecated; use req.hostname at router.cp:42"
    }

    // Second identical call is deduplicated and returns 0
    const n2 = dep.deprecate("req.host is deprecated; use req.hostname", "router.cp:42", buf[0..256]);
    // n2 == 0, dep.seenCount() == 1
}
```

### 2. Environment-Driven Silencing & Stack Traces

```cp
import { DepdOptions, Deprecator, isNamespaceSilenced, isNamespaceTraced } from "depd";

pub fn initDeprecator(no_dep_env: string, trace_dep_env: string): Deprecator {
    const opts = DepdOptions {
        no_deprecation: isNamespaceSilenced("express", no_dep_env),
        trace_deprecation: isNamespaceTraced("express", trace_dep_env),
    };
    return Deprecator.withOptions("express", opts);
}
```

### 3. Pre-Sizing & Stateless Formatting

```cp
import { formatDeprecationLen, formatDeprecation } from "depd";

pub fn formatOne(out: []u8): usize {
    const needed = formatDeprecationLen("body-parser", "extended option required", "app.cp:18");
    if (needed > out.len) {
        return 0;
    }
    return formatDeprecation("body-parser", "extended option required", "app.cp:18", out);
}
```

## Differences from npm `depd` & Safety Highlights

- **Explicit Buffer Output**: Instead of writing directly to `process.stderr` or monkey-patching JS functions/properties, CP `depd` formats into caller-provided `[]u8` slices so callers control logging sinks and memory lifetimes.
- **Explicit Env Inputs**: `isNamespaceSilenced` and `isNamespaceTraced` take the environment string explicitly, keeping the library pure, deterministic, and easy to unit-test.
- **Zero-Heap Deduplication**: Uses a fixed-size `[64]u64` FNV-1a hash table on the stack with zero GC pressure.
