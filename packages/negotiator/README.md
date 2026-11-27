[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# negotiator

Zero-heap RFC 7231 / RFC 9110 HTTP content negotiation for `Accept`, `Accept-Encoding`, `Accept-Language`, and `Accept-Charset` with q-value (`0.000..1.000`) and specificity ranking. Clean-room port of npm `negotiator`.

## Provenance & Zero-Friction Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | npm |
| **Upstream Package** | `negotiator` (`jshttp/negotiator`) |
| **CP Package** | `negotiator` (`v1.0.0`) |
| **License** | MIT |

Keep the exact package name in CP — no `@cap/*` prefix or renaming required:

```typescript
// TypeScript / Node.js (npm: negotiator)
import Negotiator from "negotiator";
const neg = new Negotiator(req);
const enc = neg.encoding(["br", "gzip", "deflate"]);
```

```typescript
// CP (same package name: "negotiator")
import { negotiateEncoding } from "negotiator";
let avail: [3]string = ["br", "gzip", "deflate"];
let enc = negotiateEncoding("gzip;q=0.8, br;q=1.0", avail[0..3]); // "br"
```

## Installation

```bash
cap add negotiator
```

## Quick Start & Practical Examples

### 1. High-Level Token Negotiation (`Encoding`, `Language`, `Charset`)

```typescript
import {
    negotiateEncoding,
    negotiateLanguage,
    negotiateCharset,
} from "negotiator";

fn negotiateHeaders(): void {
    // Accept-Encoding
    let encs: [3]string = ["br", "gzip", "identity"];
    if (let best_enc = negotiateEncoding("gzip;q=0.8, br;q=1.0", encs[0..3])) {
        // best_enc == "br"
    }

    // Accept-Language (supports BCP-47 prefix matching e.g. "en-US" <-> "en")
    let langs: [3]string = ["en", "zh-CN", "fr"];
    if (let best_lang = negotiateLanguage("zh-CN, en;q=0.9", langs[0..3])) {
        // best_lang == "zh-CN"
    }

    // Accept-Charset
    let charsets: [2]string = ["utf-8", "iso-8859-1"];
    if (let best_cs = negotiateCharset("iso-8859-1;q=0.5, utf-8;q=1.0", charsets[0..2])) {
        // best_cs == "utf-8"
    }
}
```

### 2. Index-Based Core Negotiation (`bestMediaType`, `bestEncoding`, `bestLanguage`)

```typescript
import { bestMediaType, bestEncoding, bestLanguage } from "negotiator";

fn selectMediaType(): void {
    let types: [3]string = ["text/html", "application/json", "text/plain"];
    let idx = bestMediaType("text/*;q=0.5, application/json;q=0.9", types[0..3]);
    if (idx >= 0) {
        let chosen = types[idx as usize]; // "application/json"
    }

    let encs: [2]string = ["gzip", "br"];
    let e_idx = bestEncoding("br, gzip;q=0.8", encs[0..2]); // 1

    let langs: [2]string = ["en-US", "fr"];
    let l_idx = bestLanguage("en;q=0.8, fr;q=0.5", langs[0..2]); // 0
}
```

### 3. Low-Level Weighted Header Parsing (`parseWeightedList`, `parseAcceptList`, `parseQValue`)

```typescript
import {
    QualitySpec,
    NegSpec,
    NEG_MAX_SPECS,
    parseQValue,
    parseWeightedList,
    parseAcceptList,
} from "negotiator";

fn inspectHeader(): void {
    let q = parseQValue("0.75"); // 750 (scaled 0..1000)

    let mut qspecs: [16]QualitySpec = undefined;
    let n1 = parseWeightedList("br;q=1.0, gzip;q=0.8, *;q=0.1", qspecs[0..16]);

    let mut specs: [32]NegSpec = undefined;
    let n2 = parseAcceptList("text/html, application/json;q=0.9", specs[0..32]);
}
```

## API Reference

### High-Level Negotiation
- `negotiateEncoding(accept_encoding: string, available: &[]string): string?`
- `negotiateLanguage(accept_language: string, available: &[]string): string?`
- `negotiateCharset(accept_charset: string, available: &[]string): string?`
- `negNegotiateToken(header: string, available: &[]string, is_lang: bool): string?`

### Core Index-Returning Negotiation (re-exported from `core`)
- `bestMediaType(header: string, available: []const string): i64` (`-1` if none acceptable)
- `bestEncoding(header: string, available: []const string): i64`
- `bestLanguage(header: string, available: []const string): i64`
- `parseAcceptList(header: string, out: []NegSpec): usize`

### Parsing Types, Constants & Helpers
- Constants: `NEG_MAX_SPECS` (`32`), `SPEC_WILDCARD` (`1`), `SPEC_PARTIAL` (`2`), `SPEC_EXACT` (`4`)
- `QualitySpec { value: string, q_milli: u32, order: usize }`
- `NegSpec { start: usize, len: usize, q_milli: u16, specificity: u8, order: usize }`
- `parseQValue(raw: string): u32`, `parseWeightedList(header: string, out: []QualitySpec): usize`
- `negTokenSpecificity(spec: string, candidate: string, is_lang: bool): u32`, `negTrim(s: string): string`, `negIsWs(b: u8): bool`
