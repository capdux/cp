[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# type-is

Zero-heap HTTP `Content-Type` matcher for CP supporting extension shorthands (`json`, `html`, `urlencoded`, `multipart`), wildcards (`text/*`, `*/*`), structured suffixes (`+json`, `application/*+json`), and request-body detection (`hasBody` / `requestHasBody`).

## Source & Zero-Cost Migration

| Field | Value |
| --- | --- |
| **Upstream Registry** | [npmjs.com/package/type-is](https://www.npmjs.com/package/type-is) |
| **Upstream Package** | `type-is` (v1.0.0 in CP) |
| **Upstream Repository** | https://github.com/jshttp/type-is |
| **License** | MIT |

Keep the exact package name when migrating from TypeScript/npm to CP:

```ts
// TypeScript (npm)
import typeis from "type-is";
const matched = typeis(req, ["json", "application/*+json", "urlencoded"]);
```

```cp
// CP — same package name, zero-heap HTTP Content-Type negotiation
import {
    RequestHeaders, typeis, typeisCsv, is, typeIs,
    hasBody, requestHasBody, isJson,
} from "type-is";
```

## Installation

```bash
cap add type-is
```

## Quick Start & Practical Examples

### 1. Request-Level Matching (`typeis`, `typeisCsv`, `requestHasBody`)

```cp
import {
    RequestHeaders, typeis, typeisCsv,
    hasBody, requestHasBody,
} from "type-is";

fn handleRequest() {
    let req = RequestHeaders {
        content_type: "application/vnd.api+json; charset=utf-8",
        transfer_encoding: "",
        content_length: "128",
    };

    if (requestHasBody(req)) {
        let candidates: [3]string = ["urlencoded", "+json", "text/*"];
        if (typeis(req, &candidates)) |matched| {
            // matched == "+json"
        }

        // Comma-separated shorthand variant
        if (typeisCsv(req, "html, application/*+json, text/*")) |m2| {
            // m2 == "application/*+json"
        }
    }

    // Standalone header check (true if transfer_encoding is non-empty or content_length > 0)
    let body_present = hasBody("chunked", ""); // true
}
```

### 2. Direct Content-Type Matching, Normalization & Quick Predicates

```cp
import {
    is, typeIs, typeMatches, matchType,
    normalize, normalizeTypePattern, extractEssence, isValidContentType,
    isJson, isText, isForm, isMultipart,
} from "type-is";

fn demoMatchers() {
    let ct = "application/json; charset=utf-8";

    // Strip parameters & validate essence
    if (extractEssence(ct)) |essence| {
        // essence == "application/json"
    }
    let valid = isValidContentType(ct); // true

    // Shorthand expansion via mime-types ("json" -> "application/json")
    if (normalize("json")) |norm| {
        // norm == "application/json"
    }
    let pat = normalizeTypePattern("urlencoded"); // "application/x-www-form-urlencoded"

    // Match against a slice or CSV list
    let list: [2]string = ["html", "json"];
    if (is(ct, &list)) |hit| {
        // hit == "json"
    }
    let csv_hit = typeIs(ct, "text/*, json"); // "json"

    // Single-pattern & convenience helpers
    let m1 = typeMatches(ct, "application/*");             // true
    let m2 = matchType("application/*+json", "application/ld+json"); // true
    let j = isJson(ct);                                    // true
    let t = isText("text/plain; charset=utf-8");           // true
    let f = isForm("application/x-www-form-urlencoded");   // true
    let mp = isMultipart("multipart/form-data; boundary=x"); // true
}
```

## Performance & Safety Highlights

- **Zero Heap Allocations**: All parsing, trimming, essence extraction, and case-insensitive comparisons operate on borrowed `string` slices with zero allocations.
- **RFC-Aware Case Insensitivity**: Delegates ASCII case-folding to `unicase` and extension expansion to `mime-types`, accurately matching headers like `Application/JSON; Charset=UTF-8`.
- **Null-Safe Invalid Input Handling**: Malformed `Content-Type` headers or body-less requests safely return `null` / `false` instead of throwing.

## API Reference

See [`src/index.d.cp`](src/index.d.cp) for complete type signatures, or [`README_AGENT.md`](README_AGENT.md) for the compact AI-agent reference.
