# @cap/harfbuzz@15.2.0 (Agent Quickref)

> HarfBuzz (v15.2.0) industrial-standard OpenType text shaping engine (CJK/Arabic/Latin/Indic) for CP | License: MIT
> Import: `import { ... } from "@cap/harfbuzz";` | Full types: `src/index.d.cp`

## Functions
- `export fn cp_hb_version_string(): [*:0]&u8!;`
- `export fn cp_hb_buffer_create(): *auto!?;`
- `export fn cp_hb_buffer_destroy(buf: *auto!?);`
- `export fn cp_hb_buffer_reset(buf: *auto!?);`
- `export fn cp_hb_buffer_add_utf8(buf: *auto!?, text: [*]&u8!, len: i32);`
- `export fn cp_hb_buffer_guess_segment_properties(buf: *auto!?);`
- `export fn cp_hb_buffer_get_length(buf: *auto!?): u32;`
- `export fn cp_hb_buffer_get_direction(buf: *auto!?): i32;`
- `export fn versionString(): string;`
