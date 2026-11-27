[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# doctrine

Zero-allocation JSDoc comment parser extracting description and @param/@returns/@type/@deprecated/@throws/@template tags (npm doctrine parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:doctrine` ([https://github.com/eslint/doctrine](https://github.com/eslint/doctrine))（原库名 `doctrine` → CP 包名 `doctrine`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `doctrine` |
| 版本 | `1.0.0` |
| 许可证 | `Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add doctrine
```

```cp
import {
    JsDocTag,
    JsDocComment,
    docIsSpace,
    docStripLinePrefix,
    docTagHasName,
    docParseTagLine,
    parseJsDoc,
} from "doctrine";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `JsDocTag` | `class` | 字段: `title: string, type_expr: string, name: string, description: string, optional: bool` |
| `JsDocComment` | `class` | 字段: `description: string, tags: [16]JsDocTag, tag_count: usize` · 方法: `tagCount, getTag, findTag, isDeprecated` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `docIsSpace` | `(ch: u8): bool` | 调用 `docIsSpace` 并返回 `bool`。 |
| `docStripLinePrefix` | `(line: string): string` | 调用 `docStripLinePrefix` 并返回 `string`。 |
| `docTagHasName` | `(title: string): bool` | 调用 `docTagHasName` 并返回 `bool`。 |
| `docParseTagLine` | `(line: string): JsDocTag` | 调用 `docParseTagLine` 并返回 `JsDocTag`。 |
| `parseJsDoc` | `(comment: string): JsDocComment` | 通过 `parseJsDoc` 解析输入数据，返回 `JsDocComment`。 |

## 最小调用示例

```cp
import { parseJsDoc } from "doctrine";

pub fn main() {
    let result = parseJsDoc("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
