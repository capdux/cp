[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# debugid

Zero-heap Sentry/Breakpad/PDB DebugId (128-bit UUID + appendix age) parser and formatter

| 元信息 | 值 |
| --- | --- |
| 包名 | `debugid` |
| 版本 | `0.8.0` |
| 许可证 | `Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add debugid
```

```cp
import { DebugId, parseDebugId, formatBreakpad, formatCanonical } from "debugid";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `DebugId` | `class` | 字段: `bytes: [16]u8, appendix: u32, valid: bool` · 方法: `isNil` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `parseDebugId` | `(s: string): DebugId` | 通过 `parseDebugId` 解析输入数据，返回 `DebugId`。 |
| `formatBreakpad` | `(id: DebugId, out: []u8): usize` | 通过 `formatBreakpad` 编码或格式化输出，返回 `usize`。 |
| `formatCanonical` | `(id: DebugId, out: []u8): usize` | 通过 `formatCanonical` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parseDebugId } from "debugid";

pub fn main() {
    let result = parseDebugId("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
