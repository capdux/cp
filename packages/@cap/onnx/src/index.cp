// @cap/onnx — ONNX Runtime (v1.32.0) ML Inference Engine for CP
// License: MIT (Upstream: Microsoft ONNX Runtime)

import std from "std";

pub @c {
    fn cp_ort_version(): [*:0]const u8;
    fn cp_ort_create_env(log_id: [*:0]const u8!): *!?;
    fn cp_ort_release_env(env: *!?);
    fn cp_ort_create_session_options(): *!?;
    fn cp_ort_set_intra_op_threads(opts: *!?, n: i32): i32;
    fn cp_ort_release_session_options(opts: *!?);
}

pub err OrtError {
    CreateEnvFailed,
    CreateSessionOptionsFailed,
    SetThreadsFailed,
    ClosedHandle,
    OutOfMemory,
}

/// Returns the linked ONNX Runtime shared library version string (e.g. "1.32.0").
pub fn versionString(): string {
    unsafe {
        return std.mem.span(cp_ort_version());
    }
}

/// Owned RAII wrapper around `OrtEnv*`.
/// Automatically releases the ONNX Runtime environment on scope exit via `~init()`.
pub struct OrtEnv {
    env_ptr: *!? = null,

    /// Creates a new ONNX Runtime environment with the given `log_id`.
    pub static fn create(log_id: string): OrtEnv! {
        const alloc = std.heap.page_allocator;
        const id_z = alloc.allocSentinel(u8, log_id.len, 0) catch {
            return OrtError.OutOfMemory;
        };
        defer alloc.free(id_z);
        @memcpy(id_z[0..log_id.len], log_id);

        let ptr: *!? = null;
        unsafe {
            ptr = cp_ort_create_env(id_z.ptr);
        }
        if (ptr == null) {
            return OrtError.CreateEnvFailed;
        }
        return OrtEnv{ .env_ptr = ptr };
    }

    /// Returns true if the underlying `OrtEnv*` handle is valid.
    pub fn isValid(): bool {
        return _.env_ptr != null;
    }

    ~init() {
        if (p := _.env_ptr) {
            _.env_ptr = null;
            unsafe {
                cp_ort_release_env(p);
            }
        }
    }
}

/// Owned RAII wrapper around `OrtSessionOptions*`.
/// Automatically releases the session options on scope exit via `~init()`.
pub struct OrtSessionOptions {
    opts_ptr: *!? = null,

    /// Creates a new `OrtSessionOptions` handle.
    pub static fn create(): OrtSessionOptions! {
        let ptr: *!? = null;
        unsafe {
            ptr = cp_ort_create_session_options();
        }
        if (ptr == null) {
            return OrtError.CreateSessionOptionsFailed;
        }
        return OrtSessionOptions{ .opts_ptr = ptr };
    }

    /// Configures the number of intra-op execution threads.
    pub fn setIntraOpThreads(n: i32): void! {
        if (p := _.opts_ptr) {
            let rc: i32 = -1;
            unsafe {
                rc = cp_ort_set_intra_op_threads(p, n);
            }
            if (rc != 0) {
                return OrtError.SetThreadsFailed;
            }
            return;
        }
        return OrtError.ClosedHandle;
    }

    /// Returns true if the underlying `OrtSessionOptions*` handle is valid.
    pub fn isValid(): bool {
        return _.opts_ptr != null;
    }

    ~init() {
        if (p := _.opts_ptr) {
            _.opts_ptr = null;
            unsafe {
                cp_ort_release_session_options(p);
            }
        }
    }
}
