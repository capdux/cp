# depd — AI Agent Quick Reference

Zero-heap deprecation notice formatter and deduplicated warning tracker (npm `depd` parity).

## Import

```cp
import {
    MAX_SEEN_HASHES, DepdOptions, Deprecator, depdHashKey, copySlice,
    isNamespaceSilenced, isNamespaceTraced,
    formatDeprecationLen, formatDeprecation, formatDeprecationWithTrace,
} from "depd";
```

## API Signatures

```cp
const MAX_SEEN_HASHES: usize; // 64

struct DepdOptions {
    no_deprecation: bool,
    trace_deprecation: bool,
    static fn defaults(): DepdOptions;
}

struct Deprecator {
    namespace: string,
    silenced: bool,
    no_deprecation: bool,
    trace_deprecation: bool,
    seen_hashes: [64]u64,
    seen_count: usize,
    emitted_count: u64,
    static fn new(namespace: string): Deprecator;
    static fn withOptions(namespace: string, opts: DepdOptions): Deprecator;
    fn setSilenced(*_, flag: bool);
    fn setNoDeprecation(*_, flag: bool);
    fn setTraceDeprecation(*_, flag: bool);
    fn isSilenced(): bool;
    fn isTraceEnabled(): bool;
    fn seenCount(): usize;
    fn hasWarned(msg: string): bool;
    fn warnOnce(*_, msg: string, site: string, out: []u8): usize;
    fn warnWithTrace(*_, msg: string, site: string, trace: string, out: []u8): usize;
    fn deprecate(*_, msg: string, site: string, out: []u8): usize;
    fn deprecateFn(*_, fn_name: string, msg: string, site: string, out: []u8): usize;
    fn reset(*_);
}

fn depdHashKey(ns: string, msg: string): u64;
fn copySlice(dst: []u8, pos: usize, src: string): usize;
fn isNamespaceSilenced(namespace: string, no_deprecation_env: string): bool;
fn isNamespaceTraced(namespace: string, trace_deprecation_env: string): bool;
fn formatDeprecationLen(namespace: string, msg: string, site: string): usize;
fn formatDeprecation(namespace: string, msg: string, site: string, out: []u8): usize;
fn formatDeprecationWithTrace(namespace: string, msg: string, site: string, trace: string, include_trace: bool, out: []u8): usize;
```

## Minimal Example

```cp
import { Deprecator } from "depd";

let dep = Deprecator.new("api");
let buf: [256]u8 = [0; 256];
const n = dep.deprecate("oldFn removed in v2", "main.cp:12", buf[0..256]);
// First call: n > 0 ("api deprecated oldFn removed in v2 at main.cp:12"); second call: 0
```

## Gotchas & Borrow Rules

- Mutating methods (`warnOnce`, `warnWithTrace`, `deprecate`, `deprecateFn`, `set*`, `reset`) take `*_` — declare `Deprecator` with `let`, not `const`.
- `out: []u8` is truncated silently if too small; check `formatDeprecationLen(ns, msg, site) <= out.len` beforehand when exact output is required.
- Deduplication caches up to `MAX_SEEN_HASHES` (64) unique hashes per `Deprecator`; call `reset()` to clear.
- `site == ""` omits the `" at <site>"` suffix; `deprecateFn` falls back to `fn_name` if `msg == ""`.
