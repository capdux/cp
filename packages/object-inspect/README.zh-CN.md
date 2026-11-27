[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# object-inspect

Zero-heap debug value inspector and string/bytes/number pretty-formatter with escape and truncation support

| 元信息 | 值 |
| --- | --- |
| 包名 | `object-inspect` |
| 版本 | `1.13.4` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add object-inspect
```

```cp
import {
    InspectOptions,
    defaultInspectOptions,
    inspectString,
    inspectBool,
    inspectInt,
    inspectBytes,
    inspectStringArray,
} from "object-inspect";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `InspectOptions` | `class` | 字段: `quote_style_double: bool, max_string_len: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `defaultInspectOptions` | `(): InspectOptions` | 调用 `defaultInspectOptions` 并返回 `InspectOptions`。 |
| `inspectString` | `(s: string, opts: InspectOptions, out: []u8): usize` | 调用 `inspectString` 并返回 `usize`。 |
| `inspectBool` | `(v: bool, out: []u8): usize` | 调用 `inspectBool` 并返回 `usize`。 |
| `inspectInt` | `(val: i64, out: []u8): usize` | 调用 `inspectInt` 并返回 `usize`。 |
| `inspectBytes` | `(data: &[]u8, max_bytes: usize, out: []u8): usize` | 调用 `inspectBytes` 并返回 `usize`。 |
| `inspectStringArray` | `(items: &[]string, opts: InspectOptions, out: []u8): usize` | 调用 `inspectStringArray` 并返回 `usize`。 |

## 最小调用示例

```cp
import { inspectString } from "object-inspect";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = inspectString("example", /* opts: InspectOptions */, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
