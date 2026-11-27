[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# read-pkg

Zero-allocation package.json / cap.json parser and normalizer for CP, reusing normalize-package-data.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:read-pkg` ([https://github.com/sindresorhus/read-pkg](https://github.com/sindresorhus/read-pkg))

| 元信息 | 值 |
| --- | --- |
| 包名 | `read-pkg` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add read-pkg
```

```cp
import {
    ReadPkgResult,
    rpkgIsSpace,
    rpkgStrEq,
    rpkgStartsWithAt,
    rpkgIsValueBoundary,
    rpkgSkipWs,
    rpkgFindStringEnd,
    rpkgSkipValue,
} from "read-pkg";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ReadPkgResult` | `class` | 字段: `meta: PackageMetadata, normalized: NormalizedPackageResult, parsed: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `rpkgIsSpace` | `(ch: u8): bool` | 调用 `rpkgIsSpace` 并返回 `bool`。 |
| `rpkgStrEq` | `(a: string, b: string): bool` | 调用 `rpkgStrEq` 并返回 `bool`。 |
| `rpkgStartsWithAt` | `(s: string, pos: usize, kw: string): bool` | 调用 `rpkgStartsWithAt` 并返回 `bool`。 |
| `rpkgIsValueBoundary` | `(s: string, pos: usize): bool` | 调用 `rpkgIsValueBoundary` 并返回 `bool`。 |
| `rpkgSkipWs` | `(s: string, pos: usize): usize` | 调用 `rpkgSkipWs` 并返回 `usize`。 |
| `rpkgFindStringEnd` | `(s: string, open_quote: usize): usize?` | 调用 `rpkgFindStringEnd` 并返回 `usize?`。 |
| `rpkgSkipValue` | `(s: string, pos: usize): usize?` | 调用 `rpkgSkipValue` 并返回 `usize?`。 |
| `rpkgValidateTopObject` | `(json_text: string): bool` | 调用 `rpkgValidateTopObject` 并返回 `bool`。 |
| `rpkgFindTopLevelValue` | `(json_text: string, key: string): usize?` | 调用 `rpkgFindTopLevelValue` 并返回 `usize?`。 |
| `extractJsonStringField` | `(json_text: string, key: string): string?` | 调用 `extractJsonStringField` 并返回 `string?`。 |
| `extractJsonBoolField` | `(json_text: string, key: string): bool?` | 调用 `extractJsonBoolField` 并返回 `bool?`。 |
| `rpkgExtractRepository` | `(json_text: string): string` | 调用 `rpkgExtractRepository` 并返回 `string`。 |
| `parsePackageJson` | `(json_text: string, normalize: bool): ReadPkgResult` | 通过 `parsePackageJson` 解析输入数据，返回 `ReadPkgResult`。 |

## 最小调用示例

```cp
import { parsePackageJson } from "read-pkg";

pub fn main() {
    let result = parsePackageJson("example", false);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
