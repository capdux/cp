// @cap/freetype — FreeType 2.13.3 font rasterization engine for CP
// License: FTL (Upstream: FreeType 2.13.3)

pub @c {
    fn cp_ft_init(): *!?;
    fn cp_ft_done(lib: *!?);
    fn cp_ft_version(lib: *!?, maj: *i32!, min: *i32!, pat: *i32!);
    fn cp_ft_new_memory_face(lib: *!?, data: [*]const u8!, size: i64, face_idx: i64): *!?;
    fn cp_ft_done_face(face: *!?);
    fn cp_ft_face_num_glyphs(face: *!?): i64;
}

pub err FreeTypeError {
    InitFailed,
    InvalidState,
    FaceLoadFailed,
}

/// FreeType semantic version tuple (major, minor, patch).
pub struct FtVersion {
    pub major: i32 = 0,
    pub minor: i32 = 0,
    pub patch: i32 = 0,
}

/// Owned RAII FreeType font face. Automatically freed on scope exit.
pub struct FtFace {
    raw: *!? = null,

    /// Returns the number of glyphs in this font face.
    pub fn numGlyphs(): i64 {
        if (_.raw == null) return 0;
        unsafe { return cp_ft_face_num_glyphs(_.raw); }
    }

    ~init() {
        if (p := _.raw) {
            _.raw = null;
            unsafe { cp_ft_done_face(p); }
        }
    }
}

/// Owned RAII FreeType library instance. Automatically cleaned up on scope exit.
pub struct FtLibrary {
    raw: *!? = null,

    pub static fn init(): FtLibrary! {
        let handle: *!? = null;
        unsafe {
            handle = cp_ft_init();
        }
        if (handle == null) return FreeTypeError.InitFailed;
        return FtLibrary{ .raw = handle };
    }

    /// Queries the linked FreeType library version (major, minor, patch).
    pub fn version(): FtVersion! {
        if (_.raw == null) return FreeTypeError.InvalidState;
        let maj: i32 = 0;
        let min: i32 = 0;
        let pat: i32 = 0;
        unsafe {
            cp_ft_version(_.raw, &maj, &min, &pat);
        }
        return FtVersion{
            .major = maj,
            .minor = min,
            .patch = pat,
        };
    }

    /// Loads a font face from an in-memory buffer.
    /// Caller must ensure `data` outlives the returned `FtFace`.
    pub fn newMemoryFace(data: []const u8, face_idx: i64): FtFace! {
        if (_.raw == null) return FreeTypeError.InvalidState;
        if (data.len == 0) return FreeTypeError.FaceLoadFailed;
        let handle: *!? = null;
        unsafe {
            handle = cp_ft_new_memory_face(_.raw, data.ptr, @intCast(data.len), face_idx);
        }
        if (handle == null) return FreeTypeError.FaceLoadFailed;
        return FtFace{ .raw = handle };
    }

    ~init() {
        if (p := _.raw) {
            _.raw = null;
            unsafe { cp_ft_done(p); }
        }
    }
}
