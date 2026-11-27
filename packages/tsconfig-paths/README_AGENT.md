# tsconfig-paths — AI Agent Quick Ref

Zero-allocation TypeScript tsconfig.json baseUrl + paths alias wildcard matcher and path resolver for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PathRule,
    PatternMatch,
    CandidateList,
    PathsMatcher,
    SegSpan,
    BestRuleHit,
    matchesPattern,
    appendByte,
} from "tsconfig-paths";
```

## Core Types & Constants

```cp
class PathRule { pattern: string, targets: [4]string, target_count: usize } // methods: empty(...)
class PatternMatch { matched: bool, is_exact: bool, prefix_len: usize, captured: string } // methods: none(...)
class CandidateList { items: [4]string, count: usize } // methods: empty(...) +1
class PathsMatcher { base_url: string, rules: [32]PathRule, rule_count: usize } // methods: init(...) +2
class SegSpan { start: usize, end: usize }
class BestRuleHit { found: bool, rule_idx: usize, match_info: PatternMatch }
```

## API Signatures

```cp
fn matchesPattern(pattern: string, specifier: string): PatternMatch;
fn appendByte(out: []u8, pos: usize, c: u8): usize;
fn writeNormalizedPath(raw: string, out: []u8, start_pos: usize): usize;
fn expandTargetInto(base_url: string, target: string, captured: string, out: []u8, start_pos: usize): usize;
fn findBestRule(matcher: PathsMatcher, specifier: string): BestRuleHit;
fn resolvePath(matcher: PathsMatcher, specifier: string, out: []u8): string?;
fn resolveAllCandidates(matcher: PathsMatcher, specifier: string, out_buf: []u8): CandidateList;
```

## Minimal Usage

```cp
import { matchesPattern } from "tsconfig-paths";

pub fn main() {
    let result = matchesPattern("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"tsconfig-paths"`; install via `cap add tsconfig-paths`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
