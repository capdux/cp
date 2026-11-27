[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# path-clean

Zero-heap lexical POSIX path cleaner following Go filepath.Clean / Rust path-clean semantics in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:path-clean` ([https://github.com/danreeves/path-clean](https://github.com/danreeves/path-clean))（原库名 `path-clean` → CP 包名 `path-clean`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `path-clean` |
| 版本 | `1.0.0` |
| 许可证 | `MIT OR Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add path-clean
```

```cp
import {
    pclByteAt,
    pclCleanTwo,
    cleanPath,
    cleanJoin,
    isClean,
} from "path-clean";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pclByteAt` | `(a: string, b: string, has_sep: bool, idx: usize): u8` | 调用 `pclByteAt` 并返回 `u8`。 |
| `pclCleanTwo` | `(a: string, b: string, has_sep: bool, out: []u8): usize` | 调用 `pclCleanTwo` 并返回 `usize`。 |
| `cleanPath` | `(path: string, out: []u8): usize` | 调用 `cleanPath` 并返回 `usize`。 |
| `cleanJoin` | `(a: string, b: string, out: []u8): usize` | 调用 `cleanJoin` 并返回 `usize`。 |
| `isClean` | `(path: string): bool` | 执行 `isClean` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isClean } from "path-clean";

pub fn main() {
    let result = isClean("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
