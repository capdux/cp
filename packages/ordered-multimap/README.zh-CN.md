[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ordered-multimap

Zero-heap insertion-ordered multimap supporting multiple values per key

| 元信息 | 值 |
| --- | --- |
| 包名 | `ordered-multimap` |
| 版本 | `0.7.3` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ordered-multimap
```

```cp
import { OrderedMultiMap, ommEmptyStrings, ommStrEq } from "ordered-multimap";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `OrderedMultiMap` | `class` | 字段: `keys: [OMM_CAP]string, vals: [OMM_CAP]string, count: usize` · 方法: `new, append, insert, getFirst, getAll, removeAll, +3 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ommEmptyStrings` | `(): [OMM_CAP]string` | 调用 `ommEmptyStrings` 并返回 `[OMM_CAP]string`。 |
| `ommStrEq` | `(a: string, b: string): bool` | 调用 `ommStrEq` 并返回 `bool`。 |

## 最小调用示例

```cp
import { ommStrEq } from "ordered-multimap";

pub fn main() {
    let result = ommStrEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
