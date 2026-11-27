[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# property-information

Zero-allocation HTML/SVG attribute and DOM property name mapping table

| 元信息 | 值 |
| --- | --- |
| 包名 | `property-information` |
| 版本 | `6.5.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add property-information
```

```cp
import {
    PropInfo,
    pinfMake,
    pinfNotFound,
    pinfMatchPair,
    findHtmlProp,
    isBooleanAttr,
    isSpaceSeparatedAttr,
    isCommaSeparatedAttr,
} from "property-information";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PropInfo` | `class` | 字段: `property: string, attribute: string, is_boolean: bool, is_space_separated: bool, is_comma_separated: bool, is_numeric: bool, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pinfMake` | `(property: string, attribute: string, is_boolean: bool, is_space_separated: bool, is_comma_separated: bool, is_numeric: bool): PropInfo` | 调用 `pinfMake` 并返回 `PropInfo`。 |
| `pinfNotFound` | `(name: string): PropInfo` | 调用 `pinfNotFound` 并返回 `PropInfo`。 |
| `pinfMatchPair` | `(name: string, prop: string, attr: string): bool` | 调用 `pinfMatchPair` 并返回 `bool`。 |
| `findHtmlProp` | `(name: string): PropInfo` | 查询或读取目标数据，返回 `PropInfo`。 |
| `isBooleanAttr` | `(name: string): bool` | 执行 `isBooleanAttr` 条件判定，返回 `bool`。 |
| `isSpaceSeparatedAttr` | `(name: string): bool` | 执行 `isSpaceSeparatedAttr` 条件判定，返回 `bool`。 |
| `isCommaSeparatedAttr` | `(name: string): bool` | 执行 `isCommaSeparatedAttr` 条件判定，返回 `bool`。 |
| `attrToDomProp` | `(attr: string): string` | 调用 `attrToDomProp` 并返回 `string`。 |
| `domPropToAttr` | `(prop: string): string` | 调用 `domPropToAttr` 并返回 `string`。 |

## 最小调用示例

```cp
import { isBooleanAttr } from "property-information";

pub fn main() {
    let result = isBooleanAttr("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
