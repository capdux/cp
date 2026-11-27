[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# pid

Zero-allocation industrial PID feedback controller with term limits, anti-windup, and derivative-on-measurement in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:pid` ([https://github.com/braincore/pid-rs](https://github.com/braincore/pid-rs))

| 元信息 | 值 |
| --- | --- |
| 包名 | `pid` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add pid
```

```cp
import { PidOutput, PidController, clampRange, clampSymmetric } from "pid";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PidOutput` | `class` | 字段: `output: f64, p: f64, i: f64, d: f64` |
| `PidController` | `class` | 字段: `setpoint: f64, kp: f64, ki: f64, kd: f64, p_limit: f64, i_limit: f64, +7 项` · 方法: `init, setLimits, setTermLimits, setSetpoint, setpointValue, integralValue, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `clampRange` | `(v: f64, lo: f64, hi: f64): f64` | 调用 `clampRange` 并返回 `f64`。 |
| `clampSymmetric` | `(v: f64, lim: f64): f64` | 调用 `clampSymmetric` 并返回 `f64`。 |

## 最小调用示例

```cp
import { clampRange } from "pid";

pub fn main() {
    let result = clampRange(0.0, 0.0, 0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
