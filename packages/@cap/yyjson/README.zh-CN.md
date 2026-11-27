[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/yyjson

> 版本：`0.10.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

High performance JSON library (yyjson v0.10.0) native C binding for CP

## 安装与导入

```bash
cap add @cap/yyjson
```

```cp
import { cp_yyjson_read } from "@cap/yyjson";
```

## 核心导出 API

- `export fn cp_yyjson_read(dat: [*]&u8!, len: usize, flg: u32): *auto!?;`
- `export fn cp_yyjson_doc_free(doc: *auto!?);`
- `export fn cp_yyjson_doc_get_root(doc: *auto!?): *auto!?;`
- `export fn cp_yyjson_doc_get_read_size(doc: *auto!?): usize;`
- `export fn cp_yyjson_doc_get_val_count(doc: *auto!?): usize;`
- `export fn cp_yyjson_write(doc: *auto!?, flg: u32, len: *usize!?): [*]u8!?;`
- `export fn cp_yyjson_is_null(val: *auto!?): bool;`
- `export fn cp_yyjson_is_bool(val: *auto!?): bool;`
- `export fn cp_yyjson_is_int(val: *auto!?): bool;`
- `export fn cp_yyjson_is_real(val: *auto!?): bool;`
- `export fn cp_yyjson_is_str(val: *auto!?): bool;`
- `export fn cp_yyjson_is_arr(val: *auto!?): bool;`
- `export fn cp_yyjson_is_obj(val: *auto!?): bool;`
- `export fn cp_yyjson_get_bool(val: *auto!?): bool;`
- `export fn cp_yyjson_get_sint(val: *auto!?): i64;`
- `export fn cp_yyjson_get_real(val: *auto!?): f64;`
- `export fn cp_yyjson_get_str(val: *auto!?): [*:0]&u8!?;`
- `export fn cp_yyjson_get_len(val: *auto!?): usize;`
- `export fn cp_yyjson_obj_size(obj: *auto!?): usize;`
- `export fn cp_yyjson_obj_getn(obj: *auto!?, key: [*]&u8!, key_len: usize): *auto!?;`
- `export fn cp_yyjson_arr_size(arr: *auto!?): usize;`
- `export fn cp_yyjson_arr_get(arr: *auto!?, idx: usize): *auto!?;`
- `export fn cp_yyjson_free_str(ptr: *auto!?);`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`yyjson@0.10.0`)，许可证：MIT
