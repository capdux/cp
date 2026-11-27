[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# compressible

Zero-allocation HTTP Content-Type compressibility checker

| 元信息 | 值 |
| --- | --- |
| 包名 | `compressible` |
| 版本 | `2.0.18` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add compressible
```

```cp
import {
    cmprIsSpace,
    cmprToLower,
    cmprEqlIgnoreCase,
    cmprHasPrefixIgnoreCase,
    cmprHasSuffixIgnoreCase,
    cmprFindSlash,
    stripContentTypeParams,
    isCompressible,
} from "compressible";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cmprIsSpace` | `(b: u8): bool` | 调用 `cmprIsSpace` 并返回 `bool`。 |
| `cmprToLower` | `(b: u8): u8` | 调用 `cmprToLower` 并返回 `u8`。 |
| `cmprEqlIgnoreCase` | `(a: string, b: string): bool` | 调用 `cmprEqlIgnoreCase` 并返回 `bool`。 |
| `cmprHasPrefixIgnoreCase` | `(s: string, prefix: string): bool` | 调用 `cmprHasPrefixIgnoreCase` 并返回 `bool`。 |
| `cmprHasSuffixIgnoreCase` | `(s: string, suffix: string): bool` | 调用 `cmprHasSuffixIgnoreCase` 并返回 `bool`。 |
| `cmprFindSlash` | `(s: string): usize` | 调用 `cmprFindSlash` 并返回 `usize`。 |
| `stripContentTypeParams` | `(ct: string): string` | 调用 `stripContentTypeParams` 并返回 `string`。 |
| `isCompressible` | `(content_type: string): bool` | 执行 `isCompressible` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isCompressible } from "compressible";

pub fn main() {
    let result = isCompressible("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
