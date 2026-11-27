# @cap/freetype@2.13.3 (Agent Quickref)

> FreeType 2.13.3 industrial-standard TrueType/SFNT font rasterization engine for CP | License: FTL
> Import: `import { ... } from "@cap/freetype";` | Full types: `src/index.d.cp`

## Functions
- `export fn cp_ft_init(): *auto!?;`
- `export fn cp_ft_done(lib: *auto!?);`
- `export fn cp_ft_version(lib: *auto!?, maj: *i32!, min: *i32!, pat: *i32!);`
- `export fn cp_ft_new_memory_face(lib: *auto!?, data: [*]&u8!, size: i64, face_idx: i64): *auto!?;`
- `export fn cp_ft_done_face(face: *auto!?);`
- `export fn cp_ft_face_num_glyphs(face: *auto!?): i64;`
