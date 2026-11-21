# `cap:content_disposition`

纯 CP 零堆分配 RFC 6266 / RFC 5987 HTTP `Content-Disposition` 头部解析、UTF-8 `filename*` 解码、防路径穿越文件名净化与附件头格式化库，吸收自 `/data/dev/bun/src/http/HeaderValueIterator.cp`。

## 导出符号

- `DispositionType`：`unknown | inline_disp | attachment | form_data`
- `ContentDisposition`：`parse(header)`、`decodedFilename(out)`（RFC 6266 优先 `filename*`）
- `decodeRfc5987(ext_val, out)` / `sanitizeFilename(raw, out)` / `formatAttachment(filename, out)`
