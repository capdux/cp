[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# serialize-javascript

Zero-heap XSS-safe JSON/JavaScript literal serializer escaping HTML script breakouts and line separators

| 元信息 | 值 |
| --- | --- |
| 包名 | `serialize-javascript` |
| 版本 | `6.0.2` |
| 许可证 | `BSD-3-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add serialize-javascript
```

```cp
import {
    sjsWriteStr,
    sjsHex,
    escapeUnsafeJson,
    serializeString,
    hasUnsafeScriptChars,
} from "serialize-javascript";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `sjsWriteStr` | `(s: string, out: []u8, pos: usize): usize` | 调用 `sjsWriteStr` 并返回 `usize`。 |
| `sjsHex` | `(n: u8): u8` | 调用 `sjsHex` 并返回 `u8`。 |
| `escapeUnsafeJson` | `(json_text: string, out: []u8): usize` | 调用 `escapeUnsafeJson` 并返回 `usize`。 |
| `serializeString` | `(s: string, out: []u8): usize` | 调用 `serializeString` 并返回 `usize`。 |
| `hasUnsafeScriptChars` | `(s: string): bool` | 执行 `hasUnsafeScriptChars` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { sjsWriteStr } from "serialize-javascript";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = sjsWriteStr("example", out_buf[0..256], 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
