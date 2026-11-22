# `cap:content_range`

> 参考来源：/data/dev/bun/src/http/SendFile.cp | 吸收日期：2026-11-22

纯 CP 零堆分配 RFC 9110 §14 HTTP `Range` 请求头与 `Content-Range` 响应头解析/格式化库（用于 206 Partial Content 与 416 Range Not Satisfiable）。

## 导出符号

- `ByteRange`：`{ start, end }` 闭区间与 `length()`
- `RangeStatus`：`ok | unsatisfiable | invalid`
- `ParsedRangeHeader` / `parseRangeHeader(header, total_size)`：支持 `bytes=0-499`、开放尾 `bytes=500-`、后缀 `bytes=-500` 与多区间列表
- `formatContentRange` / `formatUnsatisfiedRange` / `ParsedContentRange` / `parseContentRange`

## 上游行为契约与断点续传边界

1. **三种 RFC 9110 字节区间归一化**：
   - 闭区间 `bytes=first-last`：若 `last >= total_size`，按规范自动钳制为 `total_size - 1`；若 `first >= total_size` 或 `first > last` 视为不可满足；
   - 开放尾 `bytes=first-`：映射为 `[first, total_size - 1]`；
   - 后缀区间 `bytes=-suffix`：当 `suffix >= total_size` 时映射为整个文件 `[0, total_size - 1]`，`suffix == 0` 视为不可满足。
2. **206 与 416 响应状态区分**：
   - `RangeStatus.ok` 时用 `formatContentRange` 输出 `bytes start-end/total` 并返回 `206 Partial Content`；
   - `RangeStatus.unsatisfiable`（含 `total_size == 0`）时用 `formatUnsatisfiedRange` 输出 `bytes */total` 并返回 `416 Range Not Satisfiable`；
   - `RangeStatus.invalid`（如非 `bytes=` 单位或畸形语法）按规范忽略 `Range` 头并回退返回完整 `200 OK` 响应。
