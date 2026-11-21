# `cap:sourcemap_index`

纯 CP 零堆分配 SourceMap v3 Base64-VLQ `mappings` 解码器与 O(log N) 二分查找定位器，吸收自 `/data/dev/bun/src/sourcemap/Mapping.cp`。

## 导出符号

- `MappingEntry`：`{ gen_line, gen_col, src_idx, orig_line, orig_col, name_idx }`（全 0-based，`name_idx == -1` 表示无符号名）
- `SourceMapIndex`：
  - `parseMappings(mappings)`：完整解码标准 SourceMap v3 `mappings` 字符串（支持 `;` 换行、`,` 分段、1/4/5 字段 VLQ 与负数相对偏移）
  - `find(line, col)`：按 `(gen_line, gen_col)` 二分查找同行内最近的原始源码位置
