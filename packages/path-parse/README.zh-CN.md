[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# path-parse

Zero-allocation POSIX and Win32 path.parse() splitter into { root, dir, base, ext, name } (clean-room MIT implementation of npm path-parse, #238 / 196M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:path-parse` ([https://github.com/jbgutierrez/path-parse](https://github.com/jbgutierrez/path-parse)) | 许可：`MIT`

| 元信息 | 值 |
| --- | --- |
| 包名 | `path-parse` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add path-parse
```

```cp
import {
    ParsedPath,
    PprNameExt,
    pprIsWinSep,
    pprIsAlpha,
    pprSplitBaseExt,
    parsePosix,
    parseWin32,
} from "path-parse";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ParsedPath` | `class` | 字段: `root: string, dir: string, base: string, ext: string, name: string` |
| `PprNameExt` | `class` | 字段: `name: string, ext: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pprIsWinSep` | `(b: u8): bool` | 调用 `pprIsWinSep` 并返回 `bool`。 |
| `pprIsAlpha` | `(b: u8): bool` | 调用 `pprIsAlpha` 并返回 `bool`。 |
| `pprSplitBaseExt` | `(base: string): PprNameExt` | 调用 `pprSplitBaseExt` 并返回 `PprNameExt`。 |
| `parsePosix` | `(path: string): ParsedPath` | 通过 `parsePosix` 解析输入数据，返回 `ParsedPath`。 |
| `parseWin32` | `(path: string): ParsedPath` | 通过 `parseWin32` 解析输入数据，返回 `ParsedPath`。 |

## 最小调用示例

```cp
import { parsePosix } from "path-parse";

pub fn main() {
    let result = parsePosix("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
