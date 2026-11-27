// Type definitions for CP

import {  } from "std";
export fn cp_ort_version(): [*:0]&u8!;
export fn cp_ort_create_env(log_id: [*:0]&u8!): *auto!?;
export fn cp_ort_release_env(env: *auto!?);
export fn cp_ort_create_session_options(): *auto!?;
export fn cp_ort_set_intra_op_threads(opts: *auto!?, n: i32): i32;
export fn cp_ort_release_session_options(opts: *auto!?);
export error OrtError { CreateEnvFailed, CreateSessionOptionsFailed, SetThreadsFailed, ClosedHandle, OutOfMemory }

export fn versionString(): string;
export class OrtEnv {
    env_ptr: *auto!?,
    fn create(log_id: string): !OrtEnv;
    fn isValid(): bool;
    fn __cp_deinit(_: &mut OrtEnv);
}

export class OrtSessionOptions {
    opts_ptr: *auto!?,
    fn create(): !OrtSessionOptions;
    fn setIntraOpThreads(n: i32): !void;
    fn isValid(): bool;
    fn __cp_deinit(_: &mut OrtSessionOptions);
}

