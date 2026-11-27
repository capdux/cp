# @cap/onnx@1.32.0 (Agent Quickref)

> ONNX Runtime (v1.32.0) high-performance ML inference engine for CP via official prebuilt dynamic library | License: MIT
> Import: `import { ... } from "@cap/onnx";` | Full types: `src/index.d.cp`

## Functions
- `export fn cp_ort_version(): [*:0]&u8!;`
- `export fn cp_ort_create_env(log_id: [*:0]&u8!): *auto!?;`
- `export fn cp_ort_release_env(env: *auto!?);`
- `export fn cp_ort_create_session_options(): *auto!?;`
- `export fn cp_ort_set_intra_op_threads(opts: *auto!?, n: i32): i32;`
- `export fn cp_ort_release_session_options(opts: *auto!?);`
- `export fn versionString(): string;`
