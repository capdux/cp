# publicsuffix (AI Agent Quick Ref)

- **Import**: `import { parseDomain, suffix, registrableDomain, isPublicSuffix, PslRuleList, DomainInfo, PslRule, PSL_MAX_RULES, PSL_RULE_NORMAL, PSL_RULE_WILDCARD, PSL_RULE_EXCEPTION } from "publicsuffix";`
- **Source**: `crates.io:publicsuffix` (MIT, zero-heap PSL parser, max 256B host, max 64 custom rules, `undefined` stack buffers).

## Core Signatures

```cp
export class DomainInfo {
    norm: [256]u8, norm_len: usize,
    suffix_start: usize, suffix_len: usize,
    root_start: usize, root_len: usize,
    has_root: bool, is_known_suffix: bool,
    fn writeHost(out: []u8): usize;
    fn writeSuffix(out: []u8): usize;
    fn writeRegistrable(out: []u8): usize;
    fn suffixEq(s: string): bool;
    fn registrableEq(s: string): bool;
    fn hostEq(s: string): bool;
}

export class PslRuleList {
    rules: [64]PslRule, count: usize, include_builtin: bool,
    fn new(): PslRuleList;
    fn empty(): PslRuleList;
    fn addRule(_: auto, rule: string): bool;
    fn parseDomain(host: string): DomainInfo?;
    fn suffix(host: string): string?;
    fn registrableDomain(host: string): string?;
    fn isPublicSuffix(host: string): bool;
}

export fn parseDomain(host: string): DomainInfo?;
export fn suffix(host: string): string?;
export fn registrableDomain(host: string): string?;
export fn isPublicSuffix(host: string): bool;
```

## Constants

`PSL_MAX_RULES = 64`, `PSL_RULE_NORMAL = 0`, `PSL_RULE_WILDCARD = 1`, `PSL_RULE_EXCEPTION = 2`.

## Minimal Pattern

```cp
if (let info = parseDomain("Sub.Example.Co.Uk")) {
    let mut reg: [256]u8 = undefined;
    let n = info.writeRegistrable(reg[0..256]); // "example.co.uk"
}
```

## Pitfalls

1. **Optional returns**: `parseDomain`, `suffix`, and `registrableDomain` return `null` for invalid hosts (empty, leading/trailing dot issues, illegal chars) or when no registrable domain exists (e.g., bare `"co.uk"`).
2. **Case normalization**: `suffix(host)` / `registrableDomain(host)` return slices of the original `host` string; use `DomainInfo.writeSuffix`/`writeRegistrable` or `*Eq` methods for ASCII-lowercased output.
3. **Custom list cap**: `PslRuleList.addRule` returns `false` once `count >= 64` or if rule is empty/comment.
4. **Pure custom mode**: Use `PslRuleList.empty()` when you want to disable built-in TLDs and evaluate only explicitly added rules.
