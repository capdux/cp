[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/llama

> 版本：`1.0.12676` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

llama.cpp (b12676) local LLM inference engine for CP via official prebuilt dynamic library

## 安装与导入

```bash
cap add @cap/llama
```

```cp
import { cp_llama_backend_init } from "@cap/llama";
```

## 核心导出 API

- `export fn cp_llama_backend_init();`
- `export fn cp_llama_backend_free();`
- `export fn cp_llama_max_devices(): usize;`
- `export fn cp_llama_supports_mmap(): i32;`
- `export fn cp_llama_supports_mlock(): i32;`
- `export fn cp_llama_supports_gpu_offload(): i32;`
- `export fn cp_llama_time_us(): i64;`
- `export fn cp_llama_print_system_info(): [*:0]&u8!;`
- `export fn cp_llama_load_model(path: [*:0]&u8!, n_gpu_layers: i32): *auto!?;`
- `export fn cp_llama_free_model(m: *auto!?);`
- `export fn systemInfo(): string;`
- `export fn supportsMmap(): bool;`
- `export fn supportsMlock(): bool;`
- `export fn supportsGpuOffload(): bool;`
- `export fn maxDevices(): usize;`
- `export fn timeUs(): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c++` (`llama.cpp@1.0.12676`)，许可证：MIT
