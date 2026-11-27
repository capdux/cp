[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `qs`

Zero-allocation nested query string parser and serializer for CP — clean-room parity with **npm `qs`**, supporting nested brackets (`a[b][c]`), dot notation, indexed/empty-bracket/comma arrays, strict `null` semantics, RFC 3986 percent-encoding, and built-in depth/parameter resource-exhaustion bounds.

## Provenance & Zero-Cost Migration

| Field | Details |
| :--- | :--- |
| **Source Registry** | `npm:qs` ([ljharb/qs](https://github.com/ljharb/qs), v6 parity) |
| **CP Package** | `qs@1.0.0` (`MIT`) |
| **Dependencies** | None (stdlib `cap:ascii` only) |

Keep the exact original package name — no renaming required:

```ts
// TypeScript (npm qs)
import qs from "qs";
const obj = qs.parse("a[b][c]=1&tags=go,zig,cp", { comma: true });
```

```cp
// CP — identical package name, zero-heap stack pool
import { QsParsed, QsParseOptions } from "qs";

let parsed = QsParsed.parse("a[b][c]=1&tags=go,zig,cp", QsParseOptions{
    comma_arrays: true,
}) catch return;
const v = parsed.getNested3("a", "b", "c"); // "1"
```

## ReleaseFast Performance

Measured on Intel Xeon Platinum 8175M (`cpc 1.0.0` + `zig 0.17.0 -OReleaseFast -fllvm`, see [`bench/ECOSYSTEM_PERF_REPORT.md`](../../bench/ECOSYSTEM_PERF_REPORT.md)):

| Operation | Payload | Latency | Throughput | Bandwidth | Architecture |
| :--- | :--- | --: | --: | --: | :--- |
| `percentDecode` | 118 B percent-encoded | **170.40 ns/op** | **5,868,609 ops/s** | **660.42 MB/s** | 16-lane SIMD scan |
| `QsParsed.parseInto` | 91 B (8 nested/array params) | **367.35 ns/op** | **2,722,234 ops/s** | 236.25 MB/s | 16-lane SIMD + inline stack pool |

- **Zero stack-clear inline pool**: `QsParsed` embeds a 4 KiB stack decode pool (`POOL_CAPACITY = 4096`) and up to 64 entries (`MAX_PARAMS = 64`), lazily initialized (`undefined`) with zero upfront zeroing overhead.
- **Zero-copy fast path**: Keys and values without `%` or `+` are sliced directly from the input buffer; decoded entries address the inline pool via offsets so returning `QsParsed` by value never dangles.

## Examples

### 1. Parsing Nested Brackets, Dot Notation & Arrays

```cp
import { QsParsed, QsParseOptions } from "qs";

fn demoParse() {
    const query = "?a[b][c]=1&arr[0]=x&arr[]=y&tags=go,zig,cp&user.name=alice";
    let parsed = QsParsed.parse(query, QsParseOptions{
        allow_dots: true,
        comma_arrays: true,
    }) catch return;

    const deep = parsed.getNested3("a", "b", "c").?;   // "1"
    const uname = parsed.getNested2("user", "name").?; // "alice"
    const a0 = parsed.getArrayItem("arr", 0).?;        // "x"
    const a1 = parsed.getArrayItem("arr", 1).?;        // "y"
    const cnt = parsed.countArrayItems("tags");        // 3
    _ = deep; _ = uname; _ = a0; _ = a1; _ = cnt;
}
```

### 2. Zero-Alloc Container Reuse (`parseInto`), Strict Nulls & Percent Codec

```cp
import { QsParsed, QsParseOptions, percentEncode, percentDecode } from "qs";

fn demoReuseAndStrictNull() {
    // Reuse a single stack QsParsed across requests via parseInto()
    let parsed = QsParsed.init();
    parsed.parseInto("flag&empty=&msg=hello+world%21", QsParseOptions{
        strict_null_handling: true,
    }) catch return;

    const is_null = parsed.isNull("flag"); // true (get("flag") == null)
    const empty = parsed.get("empty").?;   // "" (non-null empty string)
    const msg = parsed.get("msg").?;       // "hello world!"

    let enc_buf: [64]u8 = undefined;
    let dec_buf: [64]u8 = undefined;
    const enc = percentEncode("CP / 100%", false, enc_buf[0..]) catch return;
    const dec = percentDecode(enc, false, dec_buf[0..]) catch return;
    _ = is_null; _ = empty; _ = msg; _ = dec;
}
```

### 3. `QsBuilder` Serialization & Error Handling (`QsError`)

```cp
import { QsBuilder, QsParsed, QsParseOptions, QsError } from "qs";

fn demoBuilderAndErrors() {
    let buf: [256]u8 = undefined;
    let b = QsBuilder.initWithOptions(buf[0..], true, true);
    b.add("q", "hello world") catch return;
    b.addNested2("filter", "status", "active") catch return;
    b.addArrayItem("ids", 0, "101") catch return;
    b.addBracketArrayItem("ids", "202") catch return;
    b.addNull("debug") catch return;
    const qs_str = b.written(); // "?q=hello+world&filter[status]=active&ids[0]=101&ids[]=202&debug"

    // Depth & parameter limits reject malformed / excessive inputs with QsError
    let small_opts = QsParseOptions{ max_depth: 2, max_params: 16 };
    _ = QsParsed.parse("a[b][c][d]=1", small_opts) catch |err| {
        // err == QsError.DepthExceeded
        _ = err;
        return;
    };
    _ = qs_str;
}
```

## API Overview

- **Errors**: `QsError` (`BufferTooSmall`, `TooManyParams`, `DepthExceeded`, `InvalidPercentEncoding`, `InvalidSyntax`)
- **Parser**: `QsParsed` (`init`, `parse`, `parseInto`, `get`, `getNested2`, `getNested3`, `getArrayItem`, `countArrayItems`, `isNull`, `len`, `entryAt`, `entryKey`, `entryValue`)
- **Serializer**: `QsBuilder` (`init`, `initWithOptions`, `add`, `addNested2`, `addNested3`, `addArrayItem`, `addBracketArrayItem`, `addNull`, `written`, `reset`)
- **Percent Codec**: `percentDecode`, `percentEncode`, `needsDecode`, `isUnreserved`

See [`src/index.d.cp`](src/index.d.cp) for full signatures.
