# @cap/llama@1.0.12676 (Agent Quickref)

> llama.cpp (b12676) local LLM inference engine for CP via official prebuilt dynamic library | License: MIT
> Import: `import { ... } from "@cap/llama";` | Full types: `src/index.d.cp`

## Functions
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
