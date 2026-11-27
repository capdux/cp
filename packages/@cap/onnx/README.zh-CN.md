[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/onnx

> 版本：`1.32.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

ONNX Runtime (v1.32.0) high-performance ML inference engine for CP via official prebuilt dynamic library

## 安装与导入

```bash
cap add @cap/onnx
```

```cp
import { cp_ort_version } from "@cap/onnx";
```

## 核心导出 API

- `export fn cp_ort_version(): [*:0]&u8!;`
- `export fn cp_ort_create_env(log_id: [*:0]&u8!): *auto!?;`
- `export fn cp_ort_release_env(env: *auto!?);`
- `export fn cp_ort_create_session_options(): *auto!?;`
- `export fn cp_ort_set_intra_op_threads(opts: *auto!?, n: i32): i32;`
- `export fn cp_ort_release_session_options(opts: *auto!?);`
- `export fn versionString(): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c++` (`onnxruntime@1.32.0`)，许可证：MIT
