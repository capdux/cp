# `cap:etag`

纯 CP 零堆分配 RFC 9110 / RFC 7232 HTTP ETag 解析、强/弱比较、`If-None-Match` / `If-Match` 条件请求求值与 FNV-1a 64-bit 内容 ETag 生成库，吸收自 `/data/dev/bun/src/http_types/ETag.cp`。

## 导出符号

- `ParsedETag`：`parse(input)`、`weakMatch(other)`、`strongMatch(other)`、`toHeader(out)`
- `ifNoneMatch(etag, header_val)`：支持 `*`、多值列表（含引号内逗号保护）与 RFC 9110 弱比较
- `ifMatch(etag, header_val)`：支持 `*`、多值列表与 RFC 9110 强比较
- `generateStrong(content, out)` / `generateWeak(content, out)`：生成 64-bit 十六进制强/弱 ETag
