# `cap:data_url` — RFC 2397 Data URL 零分配解析与最短编码器

> 参考来源：`/data/dev/bun/src/resolver/data_url.cp` | 吸收日期：2026-11-22

纯 CP 零堆分配 RFC 2397 Data URL（`data:[<mediatype>][;base64],<data>`）解析、解码与最短编码器，吸收自 `/data/dev/bun/src/resolver/data_url.cp`。

## 导出符号

- `DataUrl`：`parse(url)`、`effectiveMimeType()`（空时默认 `text/plain;charset=US-ASCII`）、`decodePayload(out)`（自动处理 percent-decoding 与可选 Base64 解码）
- `percentDecode(input, out)` / `base64Encode(src, out)` / `base64Decode(src, out)`
- `encodeBase64DataUrl` / `encodePercentDataUrl` / `encodeShortestDataUrl`（自动在 percent-escaped 与 Base64 之间选取最短字节编码）

## 上游行为契约与边界注意事项

1. **RFC 2397 默认媒体类型与 `;base64` 判定**：
   - 省略 `<mediatype>`（如 `data:,Hello` 或 `data:;base64,SGVsbG8=`）时，`effectiveMimeType()` 严格遵循 RFC 2397 返回 `"text/plain;charset=US-ASCII"`；
   - `;base64` 标记仅在紧邻逗号 `,` 之前时生效，且解析器要求输入必须以 `data:` 开头并包含分隔逗号 `,`，否则返回 `null`。
2. **最短编码自动择优（`encodeShortestDataUrl`）**：
   - 纯 ASCII 文本/SVG 等数据采用 percent-encoding 通常比 Base64（膨胀 ~33%）更短，而二进制数据（如 PNG/字体）则 Base64 更短；`encodeShortestDataUrl` 在零堆分配前提下自动对比两者长度并输出更紧凑的编码。
3. **非法转义与缓冲边界防护**：
   - `percentDecode` 严格校验 `%XX` 两位十六进制字符，`base64Decode` 严格拦截非法字符与截断填充，任何畸形载荷或输出缓冲不足均安全返回 `null`。
