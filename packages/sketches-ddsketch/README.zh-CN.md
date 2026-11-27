[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# sketches-ddsketch

Zero-heap DDSketch relative-accuracy quantile sketch for low-memory latency statistics

| 元信息 | 值 |
| --- | --- |
| 包名 | `sketches-ddsketch` |
| 版本 | `0.3.0` |
| 许可证 | `Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add sketches-ddsketch
```

```cp
import { DDSketch, ddskZeroBuckets, ddskClampAlpha, ddskAddKeyCount } from "sketches-ddsketch";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `DDSketch` | `class` | 字段: `buckets: [DDSK_BUCKETS]u64, alpha: f64, gamma: f64, log_gamma: f64, min_index: i32, has_anchor: bool, +5 项` · 方法: `new, add, count, sum, min, max, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ddskZeroBuckets` | `(): [DDSK_BUCKETS]u64` | 调用 `ddskZeroBuckets` 并返回 `[DDSK_BUCKETS]u64`。 |
| `ddskClampAlpha` | `(alpha: f64): f64` | 调用 `ddskClampAlpha` 并返回 `f64`。 |
| `ddskAddKeyCount` | `(sk: &mut DDSketch, key: i32, cnt: u64)` | 就地执行 `ddskAddKeyCount` 操作。 |

## 最小调用示例

```cp
import { ddskZeroBuckets } from "sketches-ddsketch";

pub fn main() {
    let result = ddskZeroBuckets();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
