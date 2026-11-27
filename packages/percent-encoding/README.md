[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/percent-encoding

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:percent-encoding` ([https://github.com/servo/rust-url](https://github.com/servo/rust-url))

Pure CP, zero-allocation WHATWG URL Standard percent-encoding and decoding library (clean-room MIT parity with Rust `percent-encoding`).

## Features

- **128-bit `AsciiSet` Bitmap**: O(1) per-byte lookup via `bits: [4]u32` with `add`, `remove`, `unionWith`, and `shouldPercentEncode`. Automatically encodes all non-ASCII bytes (`>= 0x80`).
- **Complete WHATWG URL Standard Sets**:
  - `controlsSet()` — C0 controls (`0x00..=0x1F`), `0x7F`, and non-ASCII
  - `fragmentSet()` — controls + ` `, `"`, `<`, `>`, `` ` ``
  - `querySet()` — controls + ` `, `"`, `#`, `<`, `>`
  - `pathSet()` — fragment + `#`, `?`, `{`, `}`
  - `userinfoSet()` — path + `/`, `:`, `;`, `=`, `@`, `[`, `\`, `]`, `^`, `|`
  - `componentSet()` — userinfo + `$`, `%`, `&`, `+`, `,`
  - `nonAlphanumericSet()` — everything except ASCII `A-Z`, `a-z`, `0-9`
- **Zero Heap Allocation**: Writes directly into caller-provided `[]u8` buffers with exact length pre-calculation (`percentEncodeLen`) and overflow-safe bounds checks.
- **Lenient, Strict, and Form Decoders**: `percentDecode` (preserves invalid `%`), `percentDecodeStrict` (returns `-1` on invalid `%`), and `percentDecodePlus` (`+` -> space for `application/x-www-form-urlencoded`).

## API

- `struct AsciiSet { bits: [4]u32 }`
  - `AsciiSet.empty(): AsciiSet`
  - `add(*_, byte: u8)`
  - `remove(*_, byte: u8)`
  - `unionWith(other: AsciiSet): AsciiSet`
  - `shouldPercentEncode(byte: u8): bool`
- `controlsSet(): AsciiSet`
- `fragmentSet(): AsciiSet`
- `querySet(): AsciiSet`
- `pathSet(): AsciiSet`
- `userinfoSet(): AsciiSet`
- `componentSet(): AsciiSet`
- `nonAlphanumericSet(): AsciiSet`
- `percentEncodeLen(input: []const u8, set: AsciiSet): usize`
- `percentEncode(input: []const u8, set: AsciiSet, out: []u8): usize`
- `percentDecode(input: string, out: []u8): usize`
- `percentDecodeStrict(input: string, out: []u8): i64`
- `percentDecodePlus(input: string, out: []u8): usize`

## License

MIT
