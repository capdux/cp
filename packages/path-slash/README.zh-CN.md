[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# path-slash

Zero-heap POSIX '/' and Windows '\' path separator conversion (UNC, drive letters, lossy UTF-8) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:path-slash` ([https://github.com/rhysd/path-slash](https://github.com/rhysd/path-slash))

| 元信息 | 值 |
| --- | --- |
| 包名 | `path-slash` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add path-slash
```

```cp
import {
    pshIsAsciiAlpha,
    pshIsSep,
    pshWriteByte,
    pshValidUtf8Len,
    pshIsAbs,
    toSlash,
    toSlashLossy,
    fromSlash,
} from "path-slash";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pshIsAsciiAlpha` | `(b: u8): bool` | 调用 `pshIsAsciiAlpha` 并返回 `bool`。 |
| `pshIsSep` | `(b: u8): bool` | 调用 `pshIsSep` 并返回 `bool`。 |
| `pshWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `pshWriteByte` 并返回 `usize`。 |
| `pshValidUtf8Len` | `(s: string, offset: usize): usize` | 调用 `pshValidUtf8Len` 并返回 `usize`。 |
| `pshIsAbs` | `(path: string): bool` | 调用 `pshIsAbs` 并返回 `bool`。 |
| `toSlash` | `(path: string, out: []u8): usize` | 调用 `toSlash` 并返回 `usize`。 |
| `toSlashLossy` | `(path: string, out: []u8): usize` | 调用 `toSlashLossy` 并返回 `usize`。 |
| `fromSlash` | `(path: string, out: []u8): usize` | 调用 `fromSlash` 并返回 `usize`。 |
| `hasBackslash` | `(path: string): bool` | 执行 `hasBackslash` 条件判定，返回 `bool`。 |
| `hasForwardSlash` | `(path: string): bool` | 执行 `hasForwardSlash` 条件判定，返回 `bool`。 |
| `trimTrailingSlashes` | `(path: string): string` | 调用 `trimTrailingSlashes` 并返回 `string`。 |
| `joinSlash` | `(a: string, b: string, out: []u8): usize` | 调用 `joinSlash` 并返回 `usize`。 |

## 最小调用示例

```cp
import { pshValidUtf8Len } from "path-slash";

pub fn main() {
    let result = pshValidUtf8Len("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
