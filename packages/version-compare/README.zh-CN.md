[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# version-compare

Zero-heap flexible version string comparison (multi-segment, v-prefix, prerelease ordering, operator matching) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:version-compare` ([https://github.com/timvisee/version-compare](https://github.com/timvisee/version-compare))

| 元信息 | 值 |
| --- | --- |
| 包名 | `version-compare` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add version-compare
```

```cp
import {
    VersionCmp,
    VcSeg,
    VcParsed,
    vcIsSep,
    vcTagRank,
    vcCmpLexLower,
    vcParse,
    compareVersions,
} from "version-compare";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `VersionCmp` | `Lt, Eq, Gt, Invalid` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `VcSeg` | `class` | 字段: `isNum: bool, numVal: u64, start: usize, len: usize` |
| `VcParsed` | `class` | 字段: `segs: [32]VcSeg, count: usize, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `vcIsSep` | `(b: u8): bool` | 调用 `vcIsSep` 并返回 `bool`。 |
| `vcTagRank` | `(s: string, start: usize, len: usize): i32` | 调用 `vcTagRank` 并返回 `i32`。 |
| `vcCmpLexLower` | `(a: string, aStart: usize, aLen: usize, b: string, bStart: usize, bLen: usize): i32` | 调用 `vcCmpLexLower` 并返回 `i32`。 |
| `vcParse` | `(raw: string): VcParsed` | 调用 `vcParse` 并返回 `VcParsed`。 |
| `compareVersions` | `(a: string, b: string): VersionCmp` | 调用 `compareVersions` 并返回 `VersionCmp`。 |
| `isLess` | `(a: string, b: string): bool` | 执行 `isLess` 条件判定，返回 `bool`。 |
| `isEqual` | `(a: string, b: string): bool` | 执行 `isEqual` 条件判定，返回 `bool`。 |
| `isGreater` | `(a: string, b: string): bool` | 执行 `isGreater` 条件判定，返回 `bool`。 |
| `satisfies` | `(ver: string, op: string, target: string): bool` | 调用 `satisfies` 并返回 `bool`。 |

## 最小调用示例

```cp
import { isLess } from "version-compare";

pub fn main() {
    let result = isLess("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
