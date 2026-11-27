# @cap/dav1d@1.5.4 (Agent Quickref)

> VideoLAN dav1d (v1.5.4) high-performance royalty-free AV1 video decoder for CP via prebuilt dynamic library | License: BSD-2-Clause
> Import: `import { ... } from "@cap/dav1d";` | Full types: `src/index.d.cp`

## Functions
- `export fn cp_dav1d_version(): [*:0]&u8!;`
- `export fn cp_dav1d_version_api(): u32;`
- `export fn cp_dav1d_open(n_threads: i32, max_frame_delay: i32): *auto!?;`
- `export fn cp_dav1d_flush(ctx: *auto!?);`
- `export fn cp_dav1d_close(ctx: *auto!?);`
- `export fn version(): string;`
- `export fn versionApi(): u32;`
