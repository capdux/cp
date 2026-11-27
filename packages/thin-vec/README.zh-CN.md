[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# thin-vec

Zero-heap compact ThinVec<i64> with push/pop/insert/remove/swapRemove/dedup/truncate and packed metadata header (crates.io thin-vec equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:thin-vec` ([https://github.com/gankra/thin-vec](https://github.com/gankra/thin-vec))

| 元信息 | 值 |
| --- | --- |
| 包名 | `thin-vec` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add thin-vec
```

```cp
import { ThinVecI64 } from "thin-vec";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ThinVecI64` | `class` | 字段: `len_val: usize, data: [64]i64` · 方法: `new, fromSlice, len, capacity, isEmpty, get, +11 项` |

## 最小调用示例

```cp
import { ThinVecI64 } from "thin-vec";

pub fn main() {
    // Refer to src/index.d.cp for ThinVecI64 fields and methods
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
