[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`mediatype`](https://github.com/picoHz/mediatype)（`MIT`）（原库名 `mediatype`，CP 包名 `mediatype`）。

# mediatype

Zero-allocation RFC 9110 / RFC 6838 HTTP Media Type (MIME) parser, structured suffix extractor, and parameter matcher for CP.

## Features

- **Zero heap allocation**: zero-copy slicing of `top`, `subtype`, `suffix`, and `params`.
- **Strict RFC 9110 / RFC 6838 validation**: `isValidToken` and `parseMediaType` reject malformed tokens, missing slashes, and invalid `+` suffixes.
- **Parameter extraction**: `getParam` performs case-insensitive parameter lookup with automatic quote stripping and backslash unescaping (`\"`, `\\`).
- **Classification & normalization**: `formatMediaType`, `isJsonLike`, and `isTextLike`.

## Usage

```cp
import { MediaType, parseMediaType, getParam, isJsonLike } from "mediatype";

let mt = MediaType{ top: "", subtype: "", suffix: "", params: "" };
if (parseMediaType("application/vnd.api+json; charset=\"utf-8\"", &mt)) {
    const json = isJsonLike(mt); // true
    let cs: [32]u8 = undefined;
    const n = getParam(mt, "CHARSET", cs[0..32]); // "utf-8"
}
```
