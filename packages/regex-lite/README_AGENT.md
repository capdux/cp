# regex-lite@1.0.0 — AI Agent Quick Ref

- **Source**: `crates.io:regex-lite` (`rust-lang/regex`, MIT, v0.1 parity) | **Deps**: `regex-syntax`, `memchr`
- **Import**: `import { RegexLite, RegexMatch, RL_MAX_INSTS } from "regex-lite";`
- **Full Types**: `packages/regex-lite/src/index.d.cp`

## Constants & Limits
```cp
RL_MAX_INSTS: usize = 128 // max NFA instructions (max 16 char classes, 64B literal prefix)
```

## Core Types & Methods
```cp
class RegexMatch {
    matched: bool, start: usize, end: usize,
    fn len(): usize
}

class RegexLite {
    valid: bool, err_code: i32,
    fn compile(pattern: string): RegexLite
    fn isOk(): bool
    fn errCode(): i32
    fn matchAnchored(text: string, start_pos: usize): RegexMatch
    fn findAt(text: string, start_pos: usize): RegexMatch
    fn find(text: string): RegexMatch
    fn isMatch(text: string): bool
    fn findAll(text: string, out_matches: []RegexMatch): usize
    fn replaceAll(text: string, replacement: string, out: []u8): usize
    fn splitCount(text: string): usize
}
```

## Minimal Pattern
```cp
import { RegexLite, RegexMatch } from "regex-lite";

const re = RegexLite.compile("\\b[a-z]+\\d+\\b");
if (re.isOk()) {
    const m = re.find("id: item42 ok"); // m.matched == true, m.start == 4, m.end == 10
    let out: [64]u8 = undefined;
    const n = re.replaceAll("item42", "REPL", out[0..]); // "REPL"
}
```

## Pitfalls & Notes
1. **Check `re.isOk()`**: Invalid syntax or patterns exceeding `RL_MAX_INSTS` (128) / 16 character classes yield `valid == false` (and `isMatch`/`find` safely return no match).
2. **No Backreferences / Lookaround**: Pure Thompson NFA guarantees linear-time ReDoS immunity; supports `.`, `*`, `+`, `?`, `{m,n}`, `|`, `(...)`, `[...]`, `^`, `$`, `\b`, `\d`, `\w`, `\s`, `\xHH`.
3. **Replacement Is Literal**: `replaceAll` inserts `replacement` bytes literally (no `$1` capture expansion).
4. **Zero-Length Matches**: `findAll` / `replaceAll` / `splitCount` advance by `+1` on empty matches to guarantee termination.
5. **Internal Helpers**: `rl*` functions and `RlInst`/`RlClass`/`RlCompileCtx` are not exported — only import `RegexLite`, `RegexMatch`, `RL_MAX_INSTS`.
