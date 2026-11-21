// Source: Absorbed from /data/dev/bun/src/runtime/ffi/ffi.cp | Synced: 2026-11-21T17:29:09Z
// lib/ffi/dlopen.cp
// POSIX Dynamic Library Loader (dlopen/dlsym/dlclose/dlerror), C-ABI Type Matrix & Callback Descriptors

const std = @import("std");

zig {
    extern "c" fn dlopen(filename: ?[*:0]const u8, flags: c_int) ?*anyopaque;
    extern "c" fn dlsym(handle: ?*anyopaque, symbol: [*:0]const u8) ?*anyopaque;
    extern "c" fn dlclose(handle: *anyopaque) c_int;
    extern "c" fn dlerror() ?[*:0]u8;
}

pub enum FFIType: u8 {
    Void = 0,
    Bool = 1,
    I8 = 2,
    U8 = 3,
    I16 = 4,
    U16 = 5,
    I32 = 6,
    U32 = 7,
    I64 = 8,
    U64 = 9,
    F32 = 10,
    F64 = 11,
    Ptr = 12,
    CString = 13,
    Buffer = 14,
    Function = 15,

    /// Returns the C-ABI size in bytes of `ty` on 64-bit LP64/LLP64 targets.
    pub static fn sizeOf(ty: FFIType): usize {
        if (ty == FFIType.Void) { return 0; }
        if (ty == FFIType.Bool || ty == FFIType.I8 || ty == FFIType.U8) { return 1; }
        if (ty == FFIType.I16 || ty == FFIType.U16) { return 2; }
        if (ty == FFIType.I32 || ty == FFIType.U32 || ty == FFIType.F32) { return 4; }
        return 8;
    }

    /// Backward-compatible alias for `sizeOf`.
    pub static fn byteSize(ty: FFIType): usize {
        return FFIType.sizeOf(ty);
    }

    /// Returns the natural C-ABI alignment in bytes of `ty`.
    pub static fn alignOf(ty: FFIType): usize {
        if (ty == FFIType.Void) { return 1; }
        if (ty == FFIType.Bool || ty == FFIType.I8 || ty == FFIType.U8) { return 1; }
        if (ty == FFIType.I16 || ty == FFIType.U16) { return 2; }
        if (ty == FFIType.I32 || ty == FFIType.U32 || ty == FFIType.F32) { return 4; }
        return 8;
    }

    /// Returns `true` if `ty` is a signed or unsigned integer C-ABI type (`i8`..`u64`).
    pub static fn isInteger(ty: FFIType): bool {
        return ty == FFIType.I8 || ty == FFIType.U8 ||
            ty == FFIType.I16 || ty == FFIType.U16 ||
            ty == FFIType.I32 || ty == FFIType.U32 ||
            ty == FFIType.I64 || ty == FFIType.U64;
    }

    /// Returns `true` if `ty` is an IEEE-754 floating-point C-ABI type (`f32`/`f64`).
    pub static fn isFloat(ty: FFIType): bool {
        return ty == FFIType.F32 || ty == FFIType.F64;
    }

    /// Backward-compatible alias for `isFloat`.
    pub static fn isFloatingPoint(ty: FFIType): bool {
        return FFIType.isFloat(ty);
    }

    /// Returns `true` if `ty` is a signed numeric C-ABI type (`i8`/`i16`/`i32`/`i64`).
    pub static fn isSigned(ty: FFIType): bool {
        return ty == FFIType.I8 || ty == FFIType.I16 ||
            ty == FFIType.I32 || ty == FFIType.I64;
    }

    /// Returns `true` if `ty` is an unsigned integer or boolean C-ABI type.
    pub static fn isUnsigned(ty: FFIType): bool {
        return ty == FFIType.Bool || ty == FFIType.U8 ||
            ty == FFIType.U16 || ty == FFIType.U32 || ty == FFIType.U64;
    }

    /// Returns `true` if `ty` is passed as a pointer at the C-ABI boundary.
    pub static fn isPointer(ty: FFIType): bool {
        return ty == FFIType.Ptr || ty == FFIType.CString ||
            ty == FFIType.Buffer || ty == FFIType.Function;
    }

    /// Returns the canonical lowercase C-ABI type name string.
    pub static fn typeName(ty: FFIType): []const u8 {
        if (ty == FFIType.Void) { return "void"; }
        if (ty == FFIType.Bool) { return "bool"; }
        if (ty == FFIType.I8) { return "i8"; }
        if (ty == FFIType.U8) { return "u8"; }
        if (ty == FFIType.I16) { return "i16"; }
        if (ty == FFIType.U16) { return "u16"; }
        if (ty == FFIType.I32) { return "i32"; }
        if (ty == FFIType.U32) { return "u32"; }
        if (ty == FFIType.I64) { return "i64"; }
        if (ty == FFIType.U64) { return "u64"; }
        if (ty == FFIType.F32) { return "f32"; }
        if (ty == FFIType.F64) { return "f64"; }
        if (ty == FFIType.Ptr) { return "ptr"; }
        if (ty == FFIType.CString) { return "cstring"; }
        if (ty == FFIType.Buffer) { return "buffer"; }
        return "function";
    }

    /// Parses a Bun/CP FFI type name string into `FFIType`.
    pub static fn fromName(type_name: []const u8): FFIType? {
        if (std.mem.eql(u8, type_name, "void")) { return FFIType.Void; }
        if (std.mem.eql(u8, type_name, "bool")) { return FFIType.Bool; }
        if (std.mem.eql(u8, type_name, "i8") || std.mem.eql(u8, type_name, "int8_t") || std.mem.eql(u8, type_name, "char")) { return FFIType.I8; }
        if (std.mem.eql(u8, type_name, "u8") || std.mem.eql(u8, type_name, "uint8_t")) { return FFIType.U8; }
        if (std.mem.eql(u8, type_name, "i16") || std.mem.eql(u8, type_name, "int16_t")) { return FFIType.I16; }
        if (std.mem.eql(u8, type_name, "u16") || std.mem.eql(u8, type_name, "uint16_t")) { return FFIType.U16; }
        if (std.mem.eql(u8, type_name, "i32") || std.mem.eql(u8, type_name, "int32_t") || std.mem.eql(u8, type_name, "int") || std.mem.eql(u8, type_name, "c_int")) { return FFIType.I32; }
        if (std.mem.eql(u8, type_name, "u32") || std.mem.eql(u8, type_name, "uint32_t") || std.mem.eql(u8, type_name, "c_uint")) { return FFIType.U32; }
        if (std.mem.eql(u8, type_name, "i64") || std.mem.eql(u8, type_name, "int64_t") || std.mem.eql(u8, type_name, "isize")) { return FFIType.I64; }
        if (std.mem.eql(u8, type_name, "u64") || std.mem.eql(u8, type_name, "uint64_t") || std.mem.eql(u8, type_name, "usize") || std.mem.eql(u8, type_name, "size_t")) { return FFIType.U64; }
        if (std.mem.eql(u8, type_name, "f32") || std.mem.eql(u8, type_name, "float")) { return FFIType.F32; }
        if (std.mem.eql(u8, type_name, "f64") || std.mem.eql(u8, type_name, "double")) { return FFIType.F64; }
        if (std.mem.eql(u8, type_name, "ptr") || std.mem.eql(u8, type_name, "pointer") || std.mem.eql(u8, type_name, "void*")) { return FFIType.Ptr; }
        if (std.mem.eql(u8, type_name, "cstring") || std.mem.eql(u8, type_name, "char*")) { return FFIType.CString; }
        if (std.mem.eql(u8, type_name, "buffer")) { return FFIType.Buffer; }
        if (std.mem.eql(u8, type_name, "function") || std.mem.eql(u8, type_name, "callback") || std.mem.eql(u8, type_name, "fn")) { return FFIType.Function; }
        return null;
    }
}

pub struct DlFlags {
    pub const RTLD_LAZY: i32 = 0x00001;
    pub const RTLD_NOW: i32 = 0x00002;
    pub const RTLD_NOLOAD: i32 = 0x00004;
    pub const RTLD_GLOBAL: i32 = 0x00100;
    pub const RTLD_LOCAL: i32 = 0x00000;
    pub const RTLD_NODELETE: i32 = 0x01000;
}

pub struct FFISignature {
    pub args: [16]FFIType = [FFIType.Void; 16];
    pub arg_count: usize = 0;
    pub ret_type: FFIType = FFIType.Void;

    pub static fn init(ret: FFIType): FFISignature {
        return FFISignature {
            args: [FFIType.Void; 16],
            arg_count: 0,
            ret_type: ret,
        };
    }

    pub fn addArg(*_, arg_ty: FFIType): bool {
        if (_.arg_count >= 16) {
            return false;
        }
        _.args[_.arg_count] = arg_ty;
        _.arg_count += 1;
        return true;
    }

    pub fn argAt(index: usize): FFIType? {
        if (index >= _.arg_count) {
            return null;
        }
        return _.args[index];
    }

    pub fn totalArgBytes(): usize {
        let sum: usize = 0;
        let i: usize = 0;
        while (i < _.arg_count) {
            sum += FFIType.sizeOf(_.args[i]);
            i += 1;
        }
        return sum;
    }
}

pub struct FfiCallbackDesc {
    pub signature: FFISignature;
    pub fn_ptr: *!? = null;
    pub user_data: *!? = null;
    pub threadsafe: bool = false;
    pub active: bool = false;

    pub static fn init(
        signature: FFISignature,
        fn_ptr: *!?,
        user_data: *!? = null,
        threadsafe: bool = false
    ): FfiCallbackDesc {
        return FfiCallbackDesc {
            signature: signature,
            fn_ptr: fn_ptr,
            user_data: user_data,
            threadsafe: threadsafe,
            active: fn_ptr != null,
        };
    }

    pub fn isValid(): bool {
        return _.active && _.fn_ptr != null;
    }

    pub fn close(*_) {
        _.fn_ptr = null;
        _.user_data = null;
        _.active = false;
    }
}

pub struct DynLib {
    pub path: []const u8 = "";
    pub flags: i32 = 1;
    pub is_open: bool = false;
    pub handle: *!? = null;
    pub err_buf: [256]u8 = [0; 256];
    pub err_len: usize = 0;

    fn copyDlError(*_) {
        _.err_len = 0;
        zig {
            if (dlerror()) |err_ptr| {
                const msg = std.mem.span(err_ptr);
                const copy_len = @min(msg.len, 255);
                @memcpy(self.err_buf[0..copy_len], msg[0..copy_len]);
                self.err_buf[copy_len] = 0;
                self.err_len = copy_len;
            }
        }
    }

    /// Returns the latest dynamic linker error message from POSIX `dlerror()`, if any.
    pub static fn dlError(): ([]const u8)? {
        zig {
            if (dlerror()) |err_ptr| {
                return std.mem.span(err_ptr);
            }
        }
        return null;
    }

    /// Opens a shared library via POSIX `dlopen`. Pass empty `""` or use `openSelf()` to open the current process.
    pub static fn open(path: []const u8, flags: i32): DynLib {
        let lib = DynLib {
            path: path,
            flags: flags,
            is_open: false,
            handle: null,
            err_buf: [0; 256],
            err_len: 0,
        };
        if (path.len >= 1024) {
            return lib;
        }
        let path_buf: [1024]u8 = undefined;
        let has_path = path.len > 0;
        if (has_path) {
            let i: usize = 0;
            while (i < path.len) {
                path_buf[i] = path[i];
                i += 1;
            }
            path_buf[path.len] = 0;
        }
        zig {
            _ = dlerror();
            const c_path: ?[*:0]const u8 = if (has_path) @ptrCast(&path_buf) else null;
            const h = dlopen(c_path, @intCast(flags));
            if (h != null) {
                lib.handle = h;
                lib.is_open = true;
            } else {
                lib.copyDlError();
            }
        }
        return lib;
    }

    /// Opens the current process global symbol table via `dlopen(null, flags)`.
    pub static fn openSelf(flags: i32 = 1): DynLib {
        return DynLib.open("", flags);
    }

    /// Opens a shared library or returns `error.DlOpenFailed` if `dlopen` fails.
    pub static fn tryOpen(path: ([]const u8)?, flags: i32 = 1): !DynLib {
        const target = path ?? "";
        const lib = DynLib.open(target, flags);
        if (!lib.is_open) {
            return error.DlOpenFailed;
        }
        return lib;
    }

    /// Resolves a symbol address using POSIX `dlsym`. Returns `null` if not found or closed.
    pub fn symbol(*_, name: []const u8): *!? {
        if (!_.is_open || name.len == 0 || name.len >= 512) {
            return null;
        }
        let name_buf: [512]u8 = undefined;
        let i: usize = 0;
        while (i < name.len) {
            name_buf[i] = name[i];
            i += 1;
        }
        name_buf[name.len] = 0;
        _.err_len = 0;
        zig {
            _ = dlerror();
            const c_sym: [*:0]const u8 = @ptrCast(&name_buf);
            const ptr = dlsym(self.handle, c_sym);
            if (ptr == null) {
                self.copyDlError();
            }
            return ptr;
        }
    }

    /// Read-only symbol lookup via POSIX `dlsym`.
    pub fn lookup(name: []const u8): *!? {
        if (!_.is_open || name.len == 0 || name.len >= 512) {
            return null;
        }
        let name_buf: [512]u8 = undefined;
        let i: usize = 0;
        while (i < name.len) {
            name_buf[i] = name[i];
            i += 1;
        }
        name_buf[name.len] = 0;
        zig {
            const c_sym: [*:0]const u8 = @ptrCast(&name_buf);
            return dlsym(self.handle, c_sym);
        }
    }

    pub fn hasSymbol(name: []const u8): bool {
        return _.lookup(name) != null;
    }

    pub fn lastError(): ([]const u8)? {
        if (_.err_len == 0) {
            return null;
        }
        return _.err_buf[0.._.err_len];
    }

    /// Closes the shared library handle via POSIX `dlclose`.
    pub fn close(*_) {
        if (!_.is_open) {
            return;
        }
        zig {
            if (self.handle) |h| {
                _ = dlclose(h);
            }
        }
        _.handle = null;
        _.is_open = false;
    }
}
