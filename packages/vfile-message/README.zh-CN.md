[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# vfile-message

Zero-allocation structured lint/AST diagnostic message with unist Point/Position formatting

| 元信息 | 值 |
| --- | --- |
| 包名 | `vfile-message` |
| 版本 | `4.0.2` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add vfile-message
```

```cp
import {
    Severity,
    Point,
    Position,
    VFileMessage,
    makePoint,
    makePosition,
    pointAtOffset,
    createMessage,
} from "vfile-message";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `Severity` | `Info, Warning, Error` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Point` | `class` | 字段: `line: u32, column: u32, offset: u32` |
| `Position` | `class` | 字段: `start: Point, end: Point` |
| `VFileMessage` | `class` | 字段: `reason: string, source: string, rule_id: string, file_path: string, severity: Severity, position: Position` · 方法: `isFatal, hasPosition` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `makePoint` | `(line: u32, column: u32, offset: u32): Point` | 调用 `makePoint` 并返回 `Point`。 |
| `makePosition` | `(start: Point, end: Point): Position` | 调用 `makePosition` 并返回 `Position`。 |
| `pointAtOffset` | `(text: string, offset: usize): Point` | 调用 `pointAtOffset` 并返回 `Point`。 |
| `createMessage` | `(reason: string, file_path: string, source: string, rule_id: string, severity: Severity, at: Point): VFileMessage` | 创建或初始化实例，返回 `VFileMessage`。 |
| `stringifyPosition` | `(pos: Position, out: []u8): usize` | 通过 `stringifyPosition` 编码或格式化输出，返回 `usize`。 |
| `formatMessage` | `(msg: VFileMessage, out: []u8): usize` | 通过 `formatMessage` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { pointAtOffset } from "vfile-message";

pub fn main() {
    let result = pointAtOffset("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
