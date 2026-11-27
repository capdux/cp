[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `utf8parse`

Zero-allocation RFC 3629 strict UTF-8 validator, 16-lane SIMD ASCII fast-path scanner, WHATWG lossy repair (`U+FFFD`), and UTF-16 code-unit length calculator for CP — clean-room parity with **crates.io `utf8parse`** & **`utf8_iter`**.

## Provenance & Zero-Cost Migration

| Field | Details |
| :--- | :--- |
| **Source Registries** | `crates.io:utf8parse` ([alacritty/vte](https://github.com/alacritty/vte), v0.2 parity) & `crates.io:utf8_iter` ([hsivonen/utf8_iter](https://github.com/hsivonen/utf8_iter), v1 parity) |
| **CP Package** | `utf8parse@1.0.0` (`MIT`) |
| **Dependencies** | None (pure CP, zero external dependencies) |

Keep the exact original package name — no renaming required:

```rust
// Rust (std / utf8parse)
let ok = std::str::from_utf8(bytes).is_ok();
let repaired = String::from_utf8_lossy(bytes);
```

```cp
// CP — identical package name, 16-lane SIMD fast path
import { validateUtf8, isValidUtf8, toLossyUtf8 } from "utf8parse";

const ok = isValidUtf8("Hello, 世界 🚀"); // true
const res = validateUtf8("Hello, 世界 🚀"); // res.valid == true, res.codepoint_count == 11
```

## ReleaseFast Performance

Measured on Intel Xeon Platinum 8175M (`cpc 1.0.0` + `zig 0.17.0 -OReleaseFast -fllvm`, see [`bench/ECOSYSTEM_PERF_REPORT.md`](../../bench/ECOSYSTEM_PERF_REPORT.md)):

| Operation | Payload | Latency | Throughput | Bandwidth | Architecture |
| :--- | :--- | --: | --: | --: | :--- |
| `validateUtf8` | 64 KB ASCII buffer | **3,508.13 ns/op** | 285,052 ops/s | **17,815.77 MB/s (17.82 GB/s)** | 16-lane SIMD ASCII fast path, zero-heap |

- **17.82 GB/s L1/L2 cache saturation**: 16-lane SIMD vectorization checks 16 ASCII bytes per iteration before dropping to strict RFC 3629 multibyte validation.
- **Strict RFC 3629 compliance**: Rejects overlong encodings, UTF-16 surrogates (`U+D800..U+DFFF`), code points `> U+10FFFF`, and truncated trailing sequences.

## Examples

### 1. Strict Validation, Codepoint Counting & ASCII Fast Check

```cp
import { validateUtf8, isValidUtf8, isAscii, utf16Len } from "utf8parse";

fn demoValidation() {
    const ascii_ok = isAscii("GET /api/v1/health HTTP/1.1"); // true
    const utf8_ok = isValidUtf8("你好 CP 🚀");                // true

    const res = validateUtf8("你好 CP 🚀");
    if (res.valid) {
        const cp_count = res.codepoint_count; // 7 Unicode scalar values
        // UTF-16 code units (🚀 is a surrogate pair = 2 units)
        const u16_units = utf16Len("你好 CP 🚀"); // 8
        _ = cp_count; _ = u16_units;
    }
    _ = ascii_ok; _ = utf8_ok;
}
```

### 2. Inspecting Invalid UTF-8 Offsets (`valid_up_to`, `error_len`) & WHATWG Lossy Repair

```cp
import { validateUtf8, toLossyUtf8 } from "utf8parse";

fn demoRepair() {
    // 0xFF is never valid in UTF-8
    const bad = "ok\xFFtail";
    const res = validateUtf8(bad);
    if (!res.valid) {
        const good_prefix = res.valid_up_to; // 2 ("ok")
        const err_len = res.error_len;       // 1 (or 0 if truncated at EOF)
        _ = good_prefix; _ = err_len;
    }

    // Replace maximal invalid subparts with U+FFFD (EF BF BD) into caller stack buffer
    let out: [64]u8 = undefined;
    const repaired = toLossyUtf8(bad, out[0..]); // "ok\u{FFFD}tail"
    _ = repaired;
}
```

### 3. Stepping Sequence-by-Sequence (`checkSeqAt`) & Continuation Byte Check

```cp
import { checkSeqAt, isCont } from "utf8parse";

fn demoStepping() {
    const s = "Aé🚀";
    let pos: usize = 0;
    while (pos < s.len) {
        const step = checkSeqAt(s, pos);
        if (!step.valid) { break; }
        pos += step.step_len; // 1 ('A'), 2 ('é'), 4 ('🚀')
    }
    const cont = isCont(0x80); // true (10xxxxxx)
    _ = cont;
}
```

## API Overview

- **Types**: `Utf8CheckResult` (`valid`, `valid_up_to`, `error_len`, `codepoint_count`), `StepResult` (`valid`, `step_len`, `error_len`), `Utf8Error`
- **Functions**: `validateUtf8(data)`, `isValidUtf8(data)`, `isAscii(data)`, `toLossyUtf8(data, out)`, `utf16Len(data)`, `checkSeqAt(data, pos)`, `isCont(b)`

See [`src/index.d.cp`](src/index.d.cp) for full signatures.
