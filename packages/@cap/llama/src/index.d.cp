// Type definitions for CP

import {  } from "std";
export fn cp_llama_backend_init();
export fn cp_llama_backend_free();
export fn cp_llama_max_devices(): usize;
export fn cp_llama_supports_mmap(): i32;
export fn cp_llama_supports_mlock(): i32;
export fn cp_llama_supports_gpu_offload(): i32;
export fn cp_llama_time_us(): i64;
export fn cp_llama_print_system_info(): [*:0]&u8!;
export fn cp_llama_load_model(path: [*:0]&u8!, n_gpu_layers: i32): *auto!?;
export fn cp_llama_free_model(m: *auto!?);
export error LlamaError { LoadModelFailed, OutOfMemory }

export fn systemInfo(): string;
export fn supportsMmap(): bool;
export fn supportsMlock(): bool;
export fn supportsGpuOffload(): bool;
export fn maxDevices(): usize;
export fn timeUs(): i64;
export class LlamaBackend {
    active: bool,
    fn init(): LlamaBackend;
    fn isActive(): bool;
    fn __cp_deinit(_: &mut LlamaBackend);
}

export class LlamaModel {
    model_ptr: *auto!?,
    fn load(path: string, n_gpu_layers: i32): !LlamaModel;
    fn isValid(): bool;
    fn __cp_deinit(_: &mut LlamaModel);
}

