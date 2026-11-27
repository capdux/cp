# bowser — AI Agent Quick Ref

Zero-allocation User-Agent parser for browser, OS, engine, and platform detection (npm bowser parity) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PlatformType,
    ParsedUA,
    BwsPair,
    bwsIsVersionChar,
    bwsFindSubstrCI,
    bwsContains,
    bwsContainsCI,
    bwsExtractVersionAt,
} from "bowser";
```

## Core Types & Constants

```cp
enum PlatformType { Desktop, Mobile, Tablet, Bot, Unknown }
class ParsedUA { browser_name: string, browser_version: string, os_name: string, os_version: string, /* +2 fields */ }
class BwsPair { name: string, version: string }
```

## API Signatures

```cp
fn bwsIsVersionChar(c: u8): bool;
fn bwsFindSubstrCI(haystack: string, needle: string): usize?;
fn bwsContains(haystack: string, needle: string): bool;
fn bwsContainsCI(haystack: string, needle: string): bool;
fn bwsExtractVersionAt(ua: string, start: usize): string;
fn bwsExtractAfter(ua: string, token: string): string;
fn bwsDetectBrowser(ua: string): BwsPair;
fn bwsDetectOS(ua: string): BwsPair;
fn bwsDetectEngine(ua: string): string;
fn bwsDetectPlatform(ua: string, os_name: string): PlatformType;
fn parseUserAgent(ua: string): ParsedUA;
```

## Minimal Usage

```cp
import { parseUserAgent } from "bowser";

pub fn main() {
    let result = parseUserAgent("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"bowser"`; install via `cap add bowser`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
