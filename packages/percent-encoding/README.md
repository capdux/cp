[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# percent-encoding

> **Source Attribution**: Clean-room MIT CP implementation of [`crates.io:percent-encoding`](https://github.com/servo/rust-url) (MIT) | CP Package Version: `1.0.0` | Zero external dependencies

Zero-allocation WHATWG URL Standard percent-encoding and percent-decoding library for CP. Provides comptime-built 128-bit `AsciiSet` bitmasks, standard URL encode sets (`CONTROLS`, `FRAGMENT`, `QUERY`, `PATH`, `USERINFO`, `COMPONENT`, `NON_ALPHANUMERIC`), bulk `@memcpy` fast-path runs, and strict / lenient / form-`+` decoders.

## Zero-Cost Migration (Rust → CP)

Keep the exact same package name (`"percent-encoding"`) — no renaming required.

| Rust (`crates.io:percent-encoding`) | CP (`percent-encoding`) |
| :--- | :--- |
| `use percent_encoding::{utf8_percent_encode, percent_decode_str, NON_ALPHANUMERIC};` | `import { percentEncode, percentDecode, nonAlphanumericSet } from "percent-encoding";` |
| `utf8_percent_encode(s, NON_ALPHANUMERIC)` | `percentEncode(s, nonAlphanumericSet(), out)` |
| `percent_decode_str(s)` | `percentDecode(s, out)` / `percentDecodeStrict(s, out)` |
| `CONTROLS.add(b' ').remove(b'~')` | `let s = controlsSet(); s.add(32); s.remove(126);` |

## API Reference

### `AsciiSet` & Predefined WHATWG Encode Sets

| Symbol / Signature | Description |
| :--- | :--- |
| `AsciiSet { bits: [4]u32 }` | 128-bit ASCII bitset (`empty()`, `add(*_, byte)`, `remove(*_, byte)`, `unionWith(other)`, `shouldPercentEncode(byte)`). |
| `controlsSet(): AsciiSet` | C0 controls (`0x00..=0x1F`) and `0x7F` (DEL). |
| `fragmentSet(): AsciiSet` | `CONTROLS` + space, `"`, `<`, `>`, `` ` ``. |
| `querySet(): AsciiSet` | `CONTROLS` + space, `"`, `#`, `<`, `>`. |
| `pathSet(): AsciiSet` | `FRAGMENT` + `#`, `?`, `{`, `}`. |
| `userinfoSet(): AsciiSet` | `PATH` + `/`, `:`, `;`, `=`, `@`, `[`, `\`, `]`, `^`, `\|`. |
| `componentSet(): AsciiSet` | `USERINFO` + `$`, `%`, `&`, `+`, `,`. |
| `nonAlphanumericSet(): AsciiSet` | Encodes everything except ASCII `0-9`, `A-Z`, `a-z`. |

### Encoding & Decoding Functions

| Signature | Description |
| :--- | :--- |
| `percentEncodeLen(input: &[]u8, set: AsciiSet): usize` | Exact encoded byte length for pre-sizing buffers. |
| `percentEncode(input: &[]u8, set: AsciiSet, out: []u8): usize` | Percent-encode `input` using `set` into `out`; returns bytes written. |
| `percentDecode(input: string, out: []u8): usize` | Lenient decode (invalid `%XX` passed through literally); returns bytes written. |
| `percentDecodeStrict(input: string, out: []u8): i64` | Strict decode; returns `-1` on malformed/truncated `%XX` or short `out`. |
| `percentDecodePlus(input: string, out: []u8): usize` | `application/x-www-form-urlencoded` decode (`+` → space, `%XX` → byte). |

## Usage Examples

### Basic URL Component Encoding & Decoding

```cp
import {
    componentSet,
    percentEncodeLen,
    percentEncode,
    percentDecode,
    percentDecodePlus,
} from "percent-encoding";

fn demoBasic() {
    const raw = "hello world & cp=100%";
    const set = componentSet();
    const need = percentEncodeLen(raw, set);

    let enc: [64]u8 = undefined;
    const n = percentEncode(raw, set, enc[0..]);
    // enc[0..n] == "hello%20world%20%26%20cp%3D100%25"

    let dec: [64]u8 = undefined;
    const m = percentDecode(enc[0..n], dec[0..]);
    // dec[0..m] == raw

    const f = percentDecodePlus("a+b%2Bc", dec[0..]);
    // dec[0..f] == "a b+c"
    * = need; * = m; * = f;
}
```

### Custom `AsciiSet` & Strict Validation

```cp
import { querySet, percentEncode, percentDecodeStrict } from "percent-encoding";

fn demoCustomAndStrict() {
    let custom = querySet();
    custom.add(39);    // also encode '\''
    custom.remove(35); // keep '#' unescaped

    let out: [64]u8 = undefined;
    const n = percentEncode("it's #1", custom, out[0..]);
    // out[0..n] == "it%27s%20#1"

    // Strict mode rejects malformed percent escapes with -1
    const bad = percentDecodeStrict("abc%2G", out[0..]);
    // bad == -1
    * = n; * = bad;
}
```

## Performance & Safety Highlights

- **Zero Heap Allocation**: All encode/decode operations write directly into caller-owned `out: []u8` slices.
- **Comptime LUTs & Bitsets**: `PE_HEX_LUT: [256]u8` and all 7 WHATWG `AsciiSet` bitmasks are computed at compile time — O(1) lookup with zero runtime setup.
- **Bulk `@memcpy` Run Copying**: Both encoder and decoder scan contiguous pass-through runs and copy them in chunks via `@memcpy` instead of byte-by-byte loops.
- **Strict Mode Validation**: `percentDecodeStrict` rejects truncated `%` sequences, non-hex digits, and insufficient output buffers cleanly (`-1`).

## License

MIT
