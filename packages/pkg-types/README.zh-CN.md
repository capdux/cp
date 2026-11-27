[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# pkg-types

Zero-heap package.json and cap.json manifest parser and package name validator

| 元信息 | 值 |
| --- | --- |
| 包名 | `pkg-types` |
| 版本 | `1.3.1` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add pkg-types
```

```cp
import {
    PkgManifest,
    PkgNameParts,
    pkgtIsSpace,
    pkgtSkipSpace,
    pkgtStrEq,
    pkgtScanStringEnd,
    pkgtSkipValue,
    pkgtEmptyManifest,
} from "pkg-types";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PkgManifest` | `class` | 字段: `name: string, version: string, license: string, main: string, module: string, entry: string, +2 项` |
| `PkgNameParts` | `class` | 字段: `scope: string, pkg_name: string, has_scope: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pkgtIsSpace` | `(c: u8): bool` | 调用 `pkgtIsSpace` 并返回 `bool`。 |
| `pkgtSkipSpace` | `(s: string, start: usize): usize` | 调用 `pkgtSkipSpace` 并返回 `usize`。 |
| `pkgtStrEq` | `(a: string, b: string): bool` | 调用 `pkgtStrEq` 并返回 `bool`。 |
| `pkgtScanStringEnd` | `(s: string, open_quote: usize): usize` | 调用 `pkgtScanStringEnd` 并返回 `usize`。 |
| `pkgtSkipValue` | `(s: string, start: usize): usize` | 调用 `pkgtSkipValue` 并返回 `usize`。 |
| `pkgtEmptyManifest` | `(): PkgManifest` | 调用 `pkgtEmptyManifest` 并返回 `PkgManifest`。 |
| `parsePkgManifest` | `(json_text: string): PkgManifest` | 通过 `parsePkgManifest` 解析输入数据，返回 `PkgManifest`。 |
| `pkgtIsValidSegmentChar` | `(c: u8): bool` | 调用 `pkgtIsValidSegmentChar` 并返回 `bool`。 |
| `pkgtIsValidSegment` | `(seg: string): bool` | 调用 `pkgtIsValidSegment` 并返回 `bool`。 |
| `isValidPkgName` | `(name: string): bool` | 执行 `isValidPkgName` 条件判定，返回 `bool`。 |
| `splitPkgScope` | `(name: string): PkgNameParts` | 调用 `splitPkgScope` 并返回 `PkgNameParts`。 |

## 最小调用示例

```cp
import { parsePkgManifest } from "pkg-types";

pub fn main() {
    let result = parsePkgManifest("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
