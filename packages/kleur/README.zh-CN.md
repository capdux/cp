[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# kleur

Zero-allocation composable chainable ANSI terminal style builder with nested close/reset recovery for CP (clean-room parity with npm kleur v4)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:kleur` ([https://github.com/lukeed/kleur](https://github.com/lukeed/kleur))

| 元信息 | 值 |
| --- | --- |
| 包名 | `kleur` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add kleur
```

```cp
import { KleurChain } from "kleur";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `KleurChain` | `class` | 字段: `enabled: bool, stack: [8]StylePair, depth: usize` · 方法: `new, on, off, depthCount, withStyle, reset, +25 项` |

## 最小调用示例

```cp
import { KleurChain } from "kleur";

pub fn main() {
    // Refer to src/index.d.cp for KleurChain fields and methods
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
