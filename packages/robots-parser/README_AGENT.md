# robots-parser — AI Agent Quick Ref

Zero-allocation RFC 9309 robots.txt parser and URL allow/disallow evaluator for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    isSpaceOrTab,
    trimSpaces,
    startsWithIgnoreCase,
    stripBom,
    cleanLine,
    findColon,
    patMatches,
    parseDelaySeconds,
} from "robots-parser";
```

## API Signatures

```cp
fn isSpaceOrTab(c: u8): bool;
fn trimSpaces(s: &[]u8): &[]u8;
fn startsWithIgnoreCase(haystack: &[]u8, prefix: &[]u8): bool;
fn stripBom(src: &[]u8): &[]u8;
fn cleanLine(raw: &[]u8): &[]u8;
fn findColon(line: &[]u8): usize;
fn patMatches(pattern: &[]u8, path: &[]u8): bool;
fn parseDelaySeconds(val: &[]u8): i64;
fn uaMatchScore(ruleUa: &[]u8, targetUa: &[]u8): i64;
fn isAllowed(robots_txt: &[]u8, user_agent: &[]u8, path: &[]u8): bool;
fn crawlDelay(robots_txt: &[]u8, user_agent: &[]u8): i64;
fn firstSitemap(robots_txt: &[]u8): &[]u8;
```

## Minimal Usage

```cp
import { isSpaceOrTab } from "robots-parser";

pub fn main() {
    let result = isSpaceOrTab(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"robots-parser"`; install via `cap add robots-parser`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
