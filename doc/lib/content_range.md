# `cap:content_range`

纯 CP 零堆分配 RFC 9110 §14 HTTP `Range` 请求头与 `Content-Range` 响应头解析/格式化库（用于 206 Partial Content 与 416 Range Not Satisfiable），吸收自 `/data/dev/bun/src/http/SendFile.cp`。

## 导出符号

- `ByteRange`：`{ start, end }` 闭区间与 `length()`
- `RangeStatus`：`ok | unsatisfiable | invalid`
- `ParsedRangeHeader` / `parseRangeHeader(header, total_size)`：支持 `bytes=0-499`、开放尾 `bytes=500-`、后缀 `bytes=-500` 与多区间列表
- `formatContentRange` / `formatUnsatisfiedRange` / `ParsedContentRange` / `parseContentRange`
