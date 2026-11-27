[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# type-check

Zero-allocation runtime type expression parser and validator supporting primitives, unions (A | B), Maybe T, arrays ([T]), tuples ((A, B)), and objects (clean-room MIT implementation of npm type-check, #241 / 195M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:type-check` ([https://github.com/gkz/type-check](https://github.com/gkz/type-check)) | 许可：`MIT`

| 元信息 | 值 |
| --- | --- |
| 包名 | `type-check` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add type-check
```

```cp
import {
    DynValueKind,
    DynValue,
    tckMatchAtom,
    typeCheck,
    isValidTypeExpr,
} from "type-check";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `DynValueKind` | `Undefined, Null, Boolean, Number, Int, Float, String, Array ...` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `DynValue` | `class` | 字段: `kind: DynValueKind, elem_kind: DynValueKind, array_len: usize` · 方法: `of, arrayOf` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `tckMatchAtom` | `(atom: string, val: DynValue): bool` | 调用 `tckMatchAtom` 并返回 `bool`。 |
| `typeCheck` | `(type_expr: string, val: DynValue): bool` | 调用 `typeCheck` 并返回 `bool`。 |
| `isValidTypeExpr` | `(type_expr: string): bool` | 执行 `isValidTypeExpr` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isValidTypeExpr } from "type-check";

pub fn main() {
    let result = isValidTypeExpr("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
