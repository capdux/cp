# `cap:data_url`

纯 CP 零堆分配 RFC 2397 Data URL（`data:[<mediatype>][;base64],<data>`）解析、解码与最短编码器，吸收自 `/data/dev/bun/src/resolver/data_url.cp`。

## 导出符号

- `DataUrl`：`parse(url)`、`effectiveMimeType()`（空时默认 `text/plain;charset=US-ASCII`）、`decodePayload(out)`（自动处理 percent-decoding 与可选 Base64 解码）
- `percentDecode(input, out)` / `base64Encode(src, out)` / `base64Decode(src, out)`
- `encodeBase64DataUrl` / `encodePercentDataUrl` / `encodeShortestDataUrl`（自动在 percent-escaped 与 Base64 之间选取最短字节编码）
