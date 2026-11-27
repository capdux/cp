[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# is-plain-obj

Zero-heap plain object metadata checker, JSON object literal validator, and prototype-pollution key guard (clean-room port of npm is-plain-obj)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:is-plain-obj` ([https://github.com/sindresorhus/is-plain-obj](https://github.com/sindresorhus/is-plain-obj))

| 元信息 | 值 |
| --- | --- |
| 包名 | `is-plain-obj` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add is-plain-obj
```

```cp
import {
    ValueMeta,
    ipoIsSpace,
    ipoSliceEq,
    ipoIsDangerousSlice,
    isPlainObjectMeta,
    isPlainJsonObject,
    isPlainObjectKey,
    hasDangerousProtoKey,
} from "is-plain-obj";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ValueMeta` | `class` | 字段: `proto_kind: u8, has_to_string_tag: bool, is_iterator: bool, is_null: bool` · 方法: `plain, nullProto` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ipoIsSpace` | `(ch: u8): bool` | 调用 `ipoIsSpace` 并返回 `bool`。 |
| `ipoSliceEq` | `(s: string, start: usize, end: usize, target: string): bool` | 调用 `ipoSliceEq` 并返回 `bool`。 |
| `ipoIsDangerousSlice` | `(s: string, start: usize, end: usize): bool` | 调用 `ipoIsDangerousSlice` 并返回 `bool`。 |
| `isPlainObjectMeta` | `(m: ValueMeta): bool` | 执行 `isPlainObjectMeta` 条件判定，返回 `bool`。 |
| `isPlainJsonObject` | `(json_text: string): bool` | 执行 `isPlainJsonObject` 条件判定，返回 `bool`。 |
| `isPlainObjectKey` | `(key: string): bool` | 执行 `isPlainObjectKey` 条件判定，返回 `bool`。 |
| `hasDangerousProtoKey` | `(json_text: string): bool` | 执行 `hasDangerousProtoKey` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isPlainJsonObject } from "is-plain-obj";

pub fn main() {
    let result = isPlainJsonObject("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
