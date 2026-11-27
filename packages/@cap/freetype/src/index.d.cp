// Type definitions for CP

export fn cp_ft_init(): *auto!?;
export fn cp_ft_done(lib: *auto!?);
export fn cp_ft_version(lib: *auto!?, maj: *i32!, min: *i32!, pat: *i32!);
export fn cp_ft_new_memory_face(lib: *auto!?, data: [*]&u8!, size: i64, face_idx: i64): *auto!?;
export fn cp_ft_done_face(face: *auto!?);
export fn cp_ft_face_num_glyphs(face: *auto!?): i64;
export error FreeTypeError { InitFailed, InvalidState, FaceLoadFailed }

export class FtVersion {
    major: i32,
    minor: i32,
    patch: i32,
}

export class FtFace {
    raw: *auto!?,
    fn numGlyphs(): i64;
    fn __cp_deinit(_: &mut FtFace);
}

export class FtLibrary {
    raw: *auto!?,
    fn init(): !FtLibrary;
    fn version(): !FtVersion;
    fn newMemoryFace(data: &[]u8, face_idx: i64): !FtFace;
    fn __cp_deinit(_: &mut FtLibrary);
}

