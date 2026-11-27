[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# node-pid-controller

纯 CP 实现的零堆分配工业级 PID 控制器，内置抗积分饱和限幅、微分低通滤波与测量值微分模式。

- **源包对齐**：对标 `npm` `node-pid-controller` 与 `crates.io` `pid_control`（MIT）
- **包版本**：`1.0.0`
- **内存模型**：全程零堆分配，纯栈上 `f64` 状态机，无 GC 抖动。

## 安装与导入

```bash
cap add node-pid-controller
```

```cp
import { PidConfig, PidTerms, PidController } from "node-pid-controller";
```

## 核心 API

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `PidConfig` | `class PidConfig { kp, ki, kd, dt, setpoint, i_min, i_max, out_min, out_max, d_filter_alpha: f64, derivative_on_measurement: bool }` | 完整配置结构，`PidConfig.defaults(kp, ki, kd)` 提供工业默认参数。 |
| `PidTerms` | `class PidTerms { p: f64, i: f64, d: f64, output: f64 }` | 最近一次更新的 P、I、D 分量与限幅后输出快照。 |
| `PidController.new` | `static fn new(kp: f64, ki: f64, kd: f64, setpoint: f64): PidController` | 使用增益与目标设定值创建控制器。 |
| `PidController.fromConfig` | `static fn fromConfig(cfg: PidConfig): PidController` | 从 `PidConfig` 创建控制器。 |
| `setTunings` / `setDt` | `fn setTunings(*self, kp, ki, kd: f64)` / `fn setDt(*self, dt: f64)` | 动态调整 PID 增益或默认采样周期 `dt`。 |
| `setOutputLimits` / `setIntegralLimits` | `fn setOutputLimits(*self, lo, hi: f64)` / `fn setIntegralLimits(*self, lo, hi: f64)` | 设置输出限幅与抗饱和积分限幅。 |
| `setDerivativeFilter` / `setDerivativeMode` | `fn setDerivativeFilter(*self, alpha: f64)` / `fn setDerivativeMode(*self, on_measurement: bool)` | 配置一阶低通微分滤波系数 `alpha` 与测量值微分模式（避免设定值突变引起微分冲击）。 |
| `setTarget` | `fn setTarget(*self, target: f64)` | 更新目标设定值 `setpoint`。 |
| `getError` / `getLastTerms` | `fn getError(measurement: f64): f64` / `fn getLastTerms(): PidTerms` | 计算当前误差或获取上次 `PidTerms` 分量。 |
| `reset` / `clone` | `fn reset(*self)` / `fn clone(): PidController` | 清空积分/微分历史状态或克隆控制器实例。 |
| `updateWithDt` / `update` | `fn updateWithDt(*self, measurement: f64): f64` / `fn update(*self, measurement: f64, dt: f64): f64` | 按内部 `dt` 或显式步长 `dt` 执行一次 PID 计算并返回限幅输出。 |

## 使用示例

### 1. 带抗积分饱和的基础闭环控制

```cp
import { PidController } from "node-pid-controller";

fn controlLoop(): f64 {
    let mut pid = PidController.new(1.5, 0.2, 0.05, 100.0);
    pid.setOutputLimits(-50.0, 50.0);
    pid.setIntegralLimits(-20.0, 20.0);

    let mut temp: f64 = 25.0;
    const out = pid.update(temp, 0.1);
    return out;
}
```

### 2. 配置化控制器与分项诊断

```cp
import { PidConfig, PidController } from "node-pid-controller";

fn configControl(): void {
    let mut cfg = PidConfig.defaults(2.0, 0.5, 0.1);
    cfg.setpoint = 60.0;
    cfg.dt = 0.02;
    cfg.d_filter_alpha = 0.2;
    cfg.derivative_on_measurement = true;

    let mut pid = PidController.fromConfig(cfg);
    const u = pid.updateWithDt(52.5);
    const terms = pid.getLastTerms(); // terms.p, terms.i, terms.d, terms.output
}
```

## 与源包差异与安全亮点

- **确定性步长**：通过显式 `dt` 参数或固定周期 `updateWithDt` 取代 JS `Date.now()` 隐式时钟，适合实时控制与仿真复现。
- **工业级保护**：原生提供抗积分饱和限幅、输出限幅、微分低通滤波及测量值微分模式。
- **零堆分配**：控制器状态全部位于栈上，零 GC 停顿，适合机器人与高频闭环控制。
