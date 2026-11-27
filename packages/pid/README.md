[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/pid`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:pid` ([https://github.com/braincore/pid-rs](https://github.com/braincore/pid-rs))

Zero-heap-allocation industrial PID closed-loop feedback controller in pure CP.

## Features

- **Independent P/I/D Term Limits**: Symmetric clamping per term (`setTermLimits`) with integral anti-windup.
- **Derivative-on-Measurement**: Computes `-kd * (measurement - prev_measurement) / dt` so setpoint changes never cause derivative kick spikes.
- **Output Saturation**: Optional `[out_min, out_max]` clamping via `setLimits`.
- **Safe `dt <= 0` Guard**: Prevents division-by-zero and spurious windup when `dt <= 0.0`.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { PidController, PidOutput } from "@cap/pid";
```

## License

MIT
