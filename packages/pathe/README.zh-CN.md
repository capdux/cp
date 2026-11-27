[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# pathe

Zero-allocation universal POSIX-normalized path utilities preserving Windows drive letters and URL protocols (clean-room MIT implementation of npm pathe, #219 / 207M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pathe` ([https://github.com/unjs/pathe](https://github.com/unjs/pathe)) | 许可：`MIT`

| 元信息 | 值 |
| --- | --- |
| 包名 | `pathe` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add pathe
```

```cp
import {
    pthIsSep,
    pthIsAlpha,
    pthIsSchemeChar,
    pthHasDrivePrefix,
    pthProtocolPrefixLen,
    pthPush,
    pthPushSlice,
    pthStrEq,
} from "pathe";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pthIsSep` | `(b: u8): bool` | 调用 `pthIsSep` 并返回 `bool`。 |
| `pthIsAlpha` | `(b: u8): bool` | 调用 `pthIsAlpha` 并返回 `bool`。 |
| `pthIsSchemeChar` | `(b: u8): bool` | 调用 `pthIsSchemeChar` 并返回 `bool`。 |
| `pthHasDrivePrefix` | `(s: string): bool` | 调用 `pthHasDrivePrefix` 并返回 `bool`。 |
| `pthProtocolPrefixLen` | `(s: string): usize` | 调用 `pthProtocolPrefixLen` 并返回 `usize`。 |
| `pthPush` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `pthPush` 并返回 `usize`。 |
| `pthPushSlice` | `(out: []u8, pos: usize, s: string): usize` | 调用 `pthPushSlice` 并返回 `usize`。 |
| `pthStrEq` | `(a: string, b: string): bool` | 调用 `pthStrEq` 并返回 `bool`。 |
| `isAbsolute` | `(path: string): bool` | 执行 `isAbsolute` 条件判定，返回 `bool`。 |
| `normalize` | `(path: string, out: []u8): usize` | 调用 `normalize` 并返回 `usize`。 |
| `join` | `(a: string, b: string, out: []u8): usize` | 调用 `join` 并返回 `usize`。 |
| `joinPath` | `(a: string, b: string, out: []u8): usize` | 调用 `joinPath` 并返回 `usize`。 |
| `pthStripTrailingSlash` | `(out: []u8, len: usize): usize` | 调用 `pthStripTrailingSlash` 并返回 `usize`。 |
| `resolve` | `(base: string, rel: string, out: []u8): usize` | 调用 `resolve` 并返回 `usize`。 |
| `dirname` | `(path: string): string` | 调用 `dirname` 并返回 `string`。 |
| `basename` | `(path: string): string` | 调用 `basename` 并返回 `string`。 |
| `extname` | `(path: string): string` | 调用 `extname` 并返回 `string`。 |
| `pthSplitSegments` | `(s: string, starts: []usize, lens: []usize): usize` | 调用 `pthSplitSegments` 并返回 `usize`。 |
| `relative` | `(from_path: string, to_path: string, out: []u8): usize` | 调用 `relative` 并返回 `usize`。 |

## 最小调用示例

```cp
import { isAbsolute } from "pathe";

pub fn main() {
    let result = isAbsolute("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
