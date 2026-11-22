# `cap:sourcemap_index`

> 参考来源：/data/dev/bun/src/sourcemap/Mapping.cp | 吸收日期：2026-11-22

纯 CP 零堆分配 SourceMap v3 Base64-VLQ `mappings` 解码器与 O(log N) 二分查找定位器。

## 导出符号

- `MappingEntry`：`{ gen_line, gen_col, src_idx, orig_line, orig_col, name_idx }`（全 0-based，`name_idx == -1` 表示无符号名）
- `SourceMapIndex`：
  - `parseMappings(mappings)`：完整解码标准 SourceMap v3 `mappings` 字符串（支持 `;` 换行、`,` 分段、1/4/5 字段 VLQ 与负数相对偏移）
  - `find(line, col)`：按 `(gen_line, gen_col)` 二分查找同行内最近的原始源码位置

## 上游行为契约与堆栈还原边界

1. **0-based 坐标系与增量相对解码（SourceMap v3 规范）**：
   - 所有行列号均为 **0-based**（与堆栈帧常见 1-based 行列号转换时需减 1）；每遇到 `;` 仅重置 `gen_col = 0` 并递增 `gen_line`，而 `src_idx` / `orig_line` / `orig_col` / `name_idx` 跨行持续累积相对偏移。
2. **同行下界二分查找 (`find`)**：
   - `find(line, col)` 在 O(log N) 内定位同 `gen_line` 中满足 `entry.gen_col <= col` 的最大列映射项；若目标行无映射或所有列号均大于 `col` 则返回 `null`，不会跨行错误回退到上一行源码。
3. **1 字段段与无名符号**：
   - 仅含 1 个 VLQ 字段的生成列段无对应原始源码位置，4 字段段将 `name_idx` 置为 `-1`；畸形 Base64-VLQ 字符或超容量时安全拦截。
