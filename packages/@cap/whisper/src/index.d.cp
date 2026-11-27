// Type definitions for CP

import {  } from "std";
export fn cp_whisper_version(): [*:0]&u8!;
export fn cp_whisper_print_system_info(): [*:0]&u8!;
export fn cp_whisper_lang_max_id(): i32;
export fn cp_whisper_lang_id(lang: [*:0]&u8!): i32;
export fn cp_whisper_lang_str(id: i32): [*:0]&u8!;
export fn cp_whisper_init_from_file(path: [*:0]&u8!): *auto!?;
export fn cp_whisper_free(ctx: *auto!?);
export fn cp_whisper_n_vocab(ctx: *auto!?): i32;
export error WhisperError { InitFailed, OutOfMemory }

export fn versionString(): string;
export fn systemInfo(): string;
export fn langMaxId(): i32;
export fn langId(code: string): i32;
export fn langStr(id: i32): string;
export class WhisperContext {
    ctx_ptr: *auto!?,
    fn initFromFile(path: string): !WhisperContext;
    fn isValid(): bool;
    fn nVocab(): i32;
    fn __cp_deinit(_: &mut WhisperContext);
}

