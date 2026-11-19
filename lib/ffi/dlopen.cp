// Source: /data/dev/bun/src/runtime/ffi/
// lib/ffi/dlopen.cp
// Dynamic Library Loader (dlopen/dlsym/dlclose) & FFI Type Descriptors
// Absorbed from Bun src/runtime/ffi/ & tcc_sys/tcc.cp

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

    pub static fn byteSize(ty: FFIType): usize {
        if (ty == FFIType.Void) { return 0; }
        if (ty == FFIType.Bool || ty == FFIType.I8 || ty == FFIType.U8) { return 1; }
        if (ty == FFIType.I16 || ty == FFIType.U16) { return 2; }
        if (ty == FFIType.I32 || ty == FFIType.U32 || ty == FFIType.F32) { return 4; }
        return 8;
    }

    pub static fn isFloatingPoint(ty: FFIType): bool {
        return ty == FFIType.F32 || ty == FFIType.F64;
    }
}

pub struct DlFlags {
    pub const RTLD_LAZY: i32 = 0x00001;
    pub const RTLD_NOW: i32 = 0x00002;
    pub const RTLD_GLOBAL: i32 = 0x00100;
    pub const RTLD_LOCAL: i32 = 0x00000;
}

pub struct FFISignature {
    args: [16]FFIType = [FFIType.Void; 16];
    arg_count: usize = 0;
    ret_type: FFIType = FFIType.Void;

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

    pub fn totalArgBytes(): usize {
        let sum: usize = 0;
        let i: usize = 0;
        while (i < _.arg_count) {
            sum += FFIType.byteSize(_.args[i]);
            i += 1;
        }
        return sum;
    }
}

pub struct DynLib {
    path: []const u8 = "";
    flags: i32 = 1;
    is_open: bool = false;

    pub static fn open(path: []const u8, flags: i32): DynLib {
        return DynLib {
            path: path,
            flags: flags,
            is_open: true,
        };
    }

    pub fn close(*_) {
        _.is_open = false;
    }
}
