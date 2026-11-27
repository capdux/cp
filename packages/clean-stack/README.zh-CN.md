[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# clean-stack

Zero-allocation stack trace cleaner removing internal runtime frames and normalizing base paths (clean-room MIT implementation of npm clean-stack v5)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:clean-stack` ([https://github.com/sindresorhus/clean-stack](https://github.com/sindresorhus/clean-stack)) | 许可：`MIT`

| 元信息 | 值 |
| --- | --- |
| 包名 | `clean-stack` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add clean-stack
```

```cp
import {
    CleanStackOptions,
    csContains,
    csStartsWith,
    csAppend,
    isInternalStackFrame,
    csWriteCleanedLine,
    cleanStackWithOptions,
    cleanStack,
} from "clean-stack";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CleanStackOptions` | `class` | 字段: `pretty: bool, base_path: string, home_dir: string` · 方法: `defaults` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `csContains` | `(hay: string, needle: string): bool` | 调用 `csContains` 并返回 `bool`。 |
| `csStartsWith` | `(s: string, prefix: string): bool` | 调用 `csStartsWith` 并返回 `bool`。 |
| `csAppend` | `(out: []u8, pos: usize, s: string): usize` | 调用 `csAppend` 并返回 `usize`。 |
| `isInternalStackFrame` | `(line: string): bool` | 执行 `isInternalStackFrame` 条件判定，返回 `bool`。 |
| `csWriteCleanedLine` | `(line: string, opts: CleanStackOptions, out: []u8, pos: usize): usize` | 调用 `csWriteCleanedLine` 并返回 `usize`。 |
| `cleanStackWithOptions` | `(stack: string, opts: CleanStackOptions, out: []u8): usize` | 调用 `cleanStackWithOptions` 并返回 `usize`。 |
| `cleanStack` | `(stack: string, out: []u8): usize` | 调用 `cleanStack` 并返回 `usize`。 |

## 最小调用示例

```cp
import { isInternalStackFrame } from "clean-stack";

pub fn main() {
    let result = isInternalStackFrame("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
