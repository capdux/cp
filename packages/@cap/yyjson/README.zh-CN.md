[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/yyjson

High performance JSON library (yyjson v0.10.0) native C binding for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/yyjson` |
| 版本 | `0.10.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/yyjson
```

```cp
import {
    YyjsonError,
    JsonVal,
    JsonDoc,
    cp_yyjson_read,
    cp_yyjson_doc_free,
    cp_yyjson_doc_get_root,
    cp_yyjson_doc_get_read_size,
    cp_yyjson_doc_get_val_count,
} from "@cap/yyjson";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `YyjsonError` | `ParseFailed, WriteFailed, OutOfMemory` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `JsonVal` | `class` | 字段: `raw: *auto!?` · 方法: `isNull, isBool, isInt, isReal, isStr, isArr, +9 项` |
| `JsonDoc` | `class` | 字段: `doc_ptr: *auto!?` · 方法: `parse, parseWithFlags, root, valCount, readSize, stringify, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cp_yyjson_read` | `(dat: [*]&u8!, len: usize, flg: u32): *auto!?` | 调用 `cp_yyjson_read` 并返回 `*auto!?`。 |
| `cp_yyjson_doc_free` | `(doc: *auto!?)` | 就地执行 `cp_yyjson_doc_free` 操作。 |
| `cp_yyjson_doc_get_root` | `(doc: *auto!?): *auto!?` | 调用 `cp_yyjson_doc_get_root` 并返回 `*auto!?`。 |
| `cp_yyjson_doc_get_read_size` | `(doc: *auto!?): usize` | 调用 `cp_yyjson_doc_get_read_size` 并返回 `usize`。 |
| `cp_yyjson_doc_get_val_count` | `(doc: *auto!?): usize` | 调用 `cp_yyjson_doc_get_val_count` 并返回 `usize`。 |
| `cp_yyjson_write` | `(doc: *auto!?, flg: u32, len: *usize!?): [*]u8!?` | 调用 `cp_yyjson_write` 并返回 `[*]u8!?`。 |
| `cp_yyjson_is_null` | `(val: *auto!?): bool` | 调用 `cp_yyjson_is_null` 并返回 `bool`。 |
| `cp_yyjson_is_bool` | `(val: *auto!?): bool` | 调用 `cp_yyjson_is_bool` 并返回 `bool`。 |
| `cp_yyjson_is_int` | `(val: *auto!?): bool` | 调用 `cp_yyjson_is_int` 并返回 `bool`。 |
| `cp_yyjson_is_real` | `(val: *auto!?): bool` | 调用 `cp_yyjson_is_real` 并返回 `bool`。 |
| `cp_yyjson_is_str` | `(val: *auto!?): bool` | 调用 `cp_yyjson_is_str` 并返回 `bool`。 |
| `cp_yyjson_is_arr` | `(val: *auto!?): bool` | 调用 `cp_yyjson_is_arr` 并返回 `bool`。 |
| `cp_yyjson_is_obj` | `(val: *auto!?): bool` | 调用 `cp_yyjson_is_obj` 并返回 `bool`。 |
| `cp_yyjson_get_bool` | `(val: *auto!?): bool` | 调用 `cp_yyjson_get_bool` 并返回 `bool`。 |
| `cp_yyjson_get_sint` | `(val: *auto!?): i64` | 调用 `cp_yyjson_get_sint` 并返回 `i64`。 |
| `cp_yyjson_get_real` | `(val: *auto!?): f64` | 调用 `cp_yyjson_get_real` 并返回 `f64`。 |
| `cp_yyjson_get_str` | `(val: *auto!?): [*:0]&u8!?` | 调用 `cp_yyjson_get_str` 并返回 `[*:0]&u8!?`。 |
| `cp_yyjson_get_len` | `(val: *auto!?): usize` | 调用 `cp_yyjson_get_len` 并返回 `usize`。 |
| `cp_yyjson_obj_size` | `(obj: *auto!?): usize` | 调用 `cp_yyjson_obj_size` 并返回 `usize`。 |
| `cp_yyjson_obj_getn` | `(obj: *auto!?, key: [*]&u8!, key_len: usize): *auto!?` | 调用 `cp_yyjson_obj_getn` 并返回 `*auto!?`。 |
| `cp_yyjson_arr_size` | `(arr: *auto!?): usize` | 调用 `cp_yyjson_arr_size` 并返回 `usize`。 |
| `cp_yyjson_arr_get` | `(arr: *auto!?, idx: usize): *auto!?` | 调用 `cp_yyjson_arr_get` 并返回 `*auto!?`。 |
| `cp_yyjson_free_str` | `(ptr: *auto!?)` | 就地执行 `cp_yyjson_free_str` 操作。 |

## 最小调用示例

```cp
import { cp_yyjson_read } from "@cap/yyjson";

pub fn main() {
    let input = "example";
    let result = cp_yyjson_read(input.ptr, 0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
