[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/utf16-iter

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:utf16_iter` ([https://github.com/hsivonen/utf16_iter](https://github.com/hsivonen/utf16_iter), `Apache-2.0 OR MIT`)

Zero-heap-allocation bidirectional UTF-16 Unicode scalar value iterator and Latin-1 / UTF-8 conversion helpers in pure CP (inspired by ICU4X / hsivonen `utf16_iter`).

## Features

- `Utf16Chars`: double-ended iterator (`new`, `next`, `nextBack`, `asSlice`, `isEmpty`) over `[]const u16`
  - Combines valid surrogate pairs (`0xD800..=0xDBFF` + `0xDC00..=0xDFFF`) into supplementary plane code points (`U+10000..=U+10FFFF`)
  - Replaces unpaired high or low surrogates with `U+FFFD` (`0xFFFD`)
- `Utf16Latin1`: zero-heap Latin-1 (`[]const u8`) to UTF-16 (`[]u16`) bidirectional iterator & bulk expander (`new`, `next`, `nextBack`, `isEmpty`, `asSlice`, `writeTo`, `encode`)
- `countUtf16Chars(slice: []const u16): usize`: counts decoded Unicode scalar values
- `utf16ToUtf8(slice: []const u16, out: []u8): usize`: zero-heap UTF-16 to UTF-8 encoding into caller buffer

## License

Apache-2.0 OR MIT
