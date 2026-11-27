[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# scroll

Zero-allocation endian-aware offset-advancing binary reader/writer cursors (PreadCursor & PwriteCursor) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:scroll` ([https://github.com/m4b/scroll](https://github.com/m4b/scroll)) / `crates.io:endian-type` ([https://github.com/dpc/endian-type](https://github.com/dpc/endian-type))

| 元信息 | 值 |
| --- | --- |
| 包名 | `scroll` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add scroll
```

```cp
import { Endian, PreadCursor, PwriteCursor } from "scroll";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `Endian` | `Little, Big` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PreadCursor` | `class` | 字段: `buf: &[]u8, offset: usize` · 方法: `new, remaining, readU8, readU16, readU32, readU64, +2 项` |
| `PwriteCursor` | `class` | 字段: `buf: []u8, offset: usize` · 方法: `new, written, remaining, writeU8, writeU16, writeU32, +1 项` |

## 最小调用示例

```cp
import { PreadCursor } from "scroll";

pub fn main() {
    // Refer to src/index.d.cp for PreadCursor fields and methods
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
