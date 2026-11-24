// @cap/llama — llama.cpp (b12676) Local LLM Inference Engine for CP
// License: MIT (Upstream: ggml-org/llama.cpp)

import std from "std";

pub @c {
    fn cp_llama_backend_init();
    fn cp_llama_backend_free();
    fn cp_llama_max_devices(): usize;
    fn cp_llama_supports_mmap(): i32;
    fn cp_llama_supports_mlock(): i32;
    fn cp_llama_supports_gpu_offload(): i32;
    fn cp_llama_time_us(): i64;
    fn cp_llama_print_system_info(): [*:0]const u8;
    fn cp_llama_load_model(path: [*:0]const u8!, n_gpu_layers: i32): *!?;
    fn cp_llama_free_model(m: *!?);
}

pub err LlamaError {
    LoadModelFailed,
    OutOfMemory,
}

/// Returns llama.cpp / ggml hardware and SIMD feature summary string.
pub fn systemInfo(): string {
    unsafe {
        return std.mem.span(cp_llama_print_system_info());
    }
}

/// Returns true if memory-mapped model loading (mmap) is supported.
pub fn supportsMmap(): bool {
    unsafe {
        return cp_llama_supports_mmap() != 0;
    }
}

/// Returns true if memory-locking (mlock) is supported.
pub fn supportsMlock(): bool {
    unsafe {
        return cp_llama_supports_mlock() != 0;
    }
}

/// Returns true if GPU layer offloading is supported by the linked backend.
pub fn supportsGpuOffload(): bool {
    unsafe {
        return cp_llama_supports_gpu_offload() != 0;
    }
}

/// Returns the maximum number of compute devices supported by the backend.
pub fn maxDevices(): usize {
    unsafe {
        return cp_llama_max_devices();
    }
}

/// Returns monotonic timestamp in microseconds from llama.cpp runtime.
pub fn timeUs(): i64 {
    unsafe {
        return cp_llama_time_us();
    }
}

/// Owned RAII guard for the llama.cpp / ggml backend lifecycle.
/// Automatically calls `llama_backend_free()` on scope exit via `~init()`.
pub struct LlamaBackend {
    active: bool = false,

    /// Initializes the llama.cpp backend and returns an owned RAII guard.
    pub static fn init(): LlamaBackend {
        unsafe {
            cp_llama_backend_init();
        }
        return LlamaBackend{ .active = true };
    }

    /// Returns true if this backend guard is still active.
    pub fn isActive(): bool {
        return _.active;
    }

    ~init() {
        if (_.active) {
            _.active = false;
            unsafe {
                cp_llama_backend_free();
            }
        }
    }
}

/// Owned RAII wrapper around `struct llama_model *`.
/// Automatically releases the loaded GGUF model on scope exit via `~init()`.
pub struct LlamaModel {
    model_ptr: *!? = null,

    /// Loads a GGUF model from `path`, offloading `n_gpu_layers` layers to GPU (0 for CPU-only, -1 for all).
    pub static fn load(path: string, n_gpu_layers: i32): LlamaModel! {
        const alloc = std.heap.page_allocator;
        const path_z = alloc.allocSentinel(u8, path.len, 0) catch {
            return LlamaError.OutOfMemory;
        };
        defer alloc.free(path_z);
        @memcpy(path_z[0..path.len], path);

        let ptr: *!? = null;
        unsafe {
            ptr = cp_llama_load_model(path_z.ptr, n_gpu_layers);
        }
        if (ptr == null) {
            return LlamaError.LoadModelFailed;
        }
        return LlamaModel{ .model_ptr = ptr };
    }

    /// Returns true if the underlying `llama_model*` handle is valid.
    pub fn isValid(): bool {
        return _.model_ptr != null;
    }

    ~init() {
        if (p := _.model_ptr) {
            _.model_ptr = null;
            unsafe {
                cp_llama_free_model(p);
            }
        }
    }
}
