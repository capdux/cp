[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# publicsuffix

> **Source Attribution**: Clean-room MIT CP port of **crates.io** [`publicsuffix`](https://github.com/rushmorem/publicsuffix) (`v2` / `MIT OR Apache-2.0`), CP package `publicsuffix@1.0.0`.

Zero-heap Public Suffix List (PSL) parser for eTLD and registrable domain (eTLD+1) extraction with built-in common TLD/multi-level rules plus custom normal, wildcard (`*.`), and exception (`!`) rules.

## Why CP `publicsuffix`?

- **Zero Heap Allocation**: Parses domains up to 256 bytes inline in `DomainInfo`, and supports up to 64 custom PSL rules (`PSL_MAX_RULES = 64`) inside `PslRuleList` with zero dynamic allocation.
- **Zero-Overhead Stack Initialization**: Normalized domain buffers (`[256]u8`) and label offset tables (`[128]usize`) use `undefined` initialization, eliminating redundant stack zeroing on every lookup.
- **Full PSL Rule Precedence**: Supports normal rules (`co.uk`), wildcard rules (`*.ck`), and exception rules (`!www.ck`) where exception rules override wildcards per the Mozilla PSL spec.
- **Security-Ready Cookie & Origin Isolation**: Reliably distinguishes public suffixes (`co.uk`, `com.cn`, `github.io`) from registrable domains (`example.co.uk`) to prevent supercookie leaks.

## Zero-Cost Migration (Rust → CP)

Keep the exact same package name (`publicsuffix`) — no renaming required.

**Rust (`Cargo.toml` / `main.rs`):**
```rust
use publicsuffix::{List, Psl};
let domain = list.domain(b"www.example.co.uk").unwrap();
```

**CP (`cap.json` / `main.cp`):**
```typescript
import {
    parseDomain, suffix, registrableDomain, isPublicSuffix,
    PslRuleList, DomainInfo, PslRule, PSL_MAX_RULES
} from "publicsuffix";
```

## Quick Start

### Extract eTLD & Registrable Domain (Built-in Rules)

```typescript
import { parseDomain, suffix, registrableDomain, isPublicSuffix } from "publicsuffix";

export fn demoPsl(): bool {
    let sfx = suffix("api.example.co.uk");          // "co.uk"
    let reg = registrableDomain("api.example.co.uk"); // "example.co.uk"
    let pub_only = isPublicSuffix("co.uk");         // true

    if (let info = parseDomain("WWW.Example.CO.UK")) {
        // Case-normalized comparisons without allocating
        let ok = info.suffixEq("co.uk") && info.registrableEq("example.co.uk");
        return ok && pub_only && sfx != null && reg != null;
    }
    return false;
}
```

### Custom Wildcard & Exception Rules (`PslRuleList`)

```typescript
import { PslRuleList } from "publicsuffix";

export fn demoCustomRules(): bool {
    let mut list = PslRuleList.new(); // includes built-in rules
    _ = list.addRule("*.ck");
    _ = list.addRule("!www.ck");

    // "a.b.ck" -> suffix "b.ck", registrable "a.b.ck"
    // "www.ck" -> exception rule overrides wildcard -> suffix "ck", registrable "www.ck"
    if (let info = list.parseDomain("www.ck")) {
        return info.suffixEq("ck") && info.registrableEq("www.ck");
    }
    return false;
}
```

## API Reference

- **Top-Level Convenience Functions** (built-in PSL):
  - `parseDomain(host: string): DomainInfo?`
  - `suffix(host: string): string?`
  - `registrableDomain(host: string): string?`
  - `isPublicSuffix(host: string): bool`
- **`DomainInfo`**: Fields `norm: [256]u8`, `norm_len`, `suffix_start`, `suffix_len`, `root_start`, `root_len`, `has_root`, `is_known_suffix`; methods `writeHost(out)`, `writeSuffix(out)`, `writeRegistrable(out)`, `suffixEq(s)`, `registrableEq(s)`, `hostEq(s)`.
- **`PslRuleList`**: `new()` (with built-ins), `empty()` (custom rules only), `addRule(rule: string): bool`, `parseDomain(host)`, `suffix(host)`, `registrableDomain(host)`, `isPublicSuffix(host)`.
- **Constants**: `PSL_MAX_RULES (64)`, `PSL_RULE_NORMAL (0)`, `PSL_RULE_WILDCARD (1)`, `PSL_RULE_EXCEPTION (2)`.

## License

MIT
