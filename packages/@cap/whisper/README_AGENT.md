# @cap/whisper@1.9.6 (Agent Quickref)

> whisper.cpp (v1.9.6) local speech-to-text (ASR) engine for CP via prebuilt dynamic library | License: MIT
> Import: `import { ... } from "@cap/whisper";` | Full types: `src/index.d.cp`

## Functions
- `export fn cp_whisper_version(): [*:0]&u8!;`
- `export fn cp_whisper_print_system_info(): [*:0]&u8!;`
- `export fn cp_whisper_lang_max_id(): i32;`
- `export fn cp_whisper_lang_id(lang: [*:0]&u8!): i32;`
- `export fn cp_whisper_lang_str(id: i32): [*:0]&u8!;`
- `export fn cp_whisper_init_from_file(path: [*:0]&u8!): *auto!?;`
- `export fn cp_whisper_free(ctx: *auto!?);`
- `export fn cp_whisper_n_vocab(ctx: *auto!?): i32;`
- `export fn versionString(): string;`
- `export fn systemInfo(): string;`
- `export fn langMaxId(): i32;`
- `export fn langId(code: string): i32;`
- `export fn langStr(id: i32): string;`
