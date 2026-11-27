[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# binary-extensions

Zero-heap O(log n) lookup of 127 common binary file extensions and categories in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:binary-extensions` ([https://github.com/sindresorhus/binary-extensions](https://github.com/sindresorhus/binary-extensions))

| 元信息 | 值 |
| --- | --- |
| 包名 | `binary-extensions` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add binary-extensions
```

```cp
import { cmpLowerToAscii, extensionCategory, isBinaryExtension, isBinaryPath } from "binary-extensions";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cmpLowerToAscii` | `(a: string, b: string): i32` | 调用 `cmpLowerToAscii` 并返回 `i32`。 |
| `extensionCategory` | `(ext: string): u8` | 调用 `extensionCategory` 并返回 `u8`。 |
| `isBinaryExtension` | `(ext: string): bool` | 执行 `isBinaryExtension` 条件判定，返回 `bool`。 |
| `isBinaryPath` | `(path: string): bool` | 执行 `isBinaryPath` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isBinaryExtension } from "binary-extensions";

pub fn main() {
    let result = isBinaryExtension("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
