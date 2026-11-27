[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# clsx

Zero-heap conditional CSS class name builder with whitespace normalization, deduplication, and word-boundary checks (npm clsx/classnames parity).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:clsx` ([https://github.com/lukeed/clsx](https://github.com/lukeed/clsx))

| 元信息 | 值 |
| --- | --- |
| 包名 | `clsx` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add clsx
```

```cp
import { CondClass, ClassBuilder, joinClasses, joinCondClasses } from "clsx";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CondClass` | `class` | 字段: `name: &[]u8, enabled: bool` |
| `ClassBuilder` | `class` | 字段: `buf: []u8, pos: usize, overflow: bool` · 方法: `init, reset, has, appendToken, add, addIf, +5 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `joinClasses` | `(classes: &[]&[]u8, dst: []u8): i64` | 调用 `joinClasses` 并返回 `i64`。 |
| `joinCondClasses` | `(items: &[]CondClass, dst: []u8): i64` | 调用 `joinCondClasses` 并返回 `i64`。 |

## 最小调用示例

```cp
import { joinClasses } from "clsx";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = joinClasses(/* classes: &[]&[]u8 */, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
