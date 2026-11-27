[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/form-urlencoded

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:querystringify` ([https://github.com/unshiftio/querystringify](https://github.com/unshiftio/querystringify)) / `crates.io:urlencoding` ([https://github.com/kornelski/rust_urlencoding](https://github.com/kornelski/rust_urlencoding))（原库名 `querystringify` → CP 包名 `@cap/form-urlencoded`）

Zero-heap-allocation WHATWG `application/x-www-form-urlencoded` parser, serializer, and percent-encoding codec in pure CP (inspired by Rust `form_urlencoded` & `percent-encoding`).

## Features

- `percentEncode(input, space_as_plus, out): usize` & `percentDecode(input, plus_as_space, out): usize`
- `FormPair` & `parseFormPairs(query, out): usize` (handles leading `?`, empty segments, bare keys)
- `FormSerializer` with `appendPair(*_, out, key, val): bool` (atomic buffer check)
- `getParamDecoded(query, target_key, out): i64` (zero-heap decoded key lookup + value decoding)

## License

MIT
