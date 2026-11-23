// Source: cp/lib/wasm_fmt/types.cp (Pure CP WebAssembly binary format types & errors)
const std = @import("std");

pub const WASM_MAGIC: [4]u8 = .{ @as(u8, 0x00), @as(u8, 0x61), @as(u8, 0x73), @as(u8, 0x6D) };
pub const WASM_VERSION: [4]u8 = .{ @as(u8, 0x01), @as(u8, 0x00), @as(u8, 0x00), @as(u8, 0x00) };

pub const WasmError = error{
    InvalidMagic,
    UnsupportedVersion,
    UnexpectedEof,
    Leb128Overflow,
    InvalidSectionId,
    OutOfOrderSection,
    DuplicateSection,
    SectionLengthMismatch,
    InvalidValType,
    InvalidExternalKind,
    InvalidLimitFlags,
    InvalidFuncTypeTag,
    InvalidMutFlag,
    InvalidTypeIndex,
    OutOfMemory,
};

pub enum ValType: u8 {
    I32 = 0x7F,
    I64 = 0x7E,
    F32 = 0x7D,
    F64 = 0x7C,
    FuncRef = 0x70,
    ExternRef = 0x6F,

    pub static fn fromByte(b: u8): WasmError!ValType {
        return switch (b) {
            0x7F => .I32,
            0x7E => .I64,
            0x7D => .F32,
            0x7C => .F64,
            0x70 => .FuncRef,
            0x6F => .ExternRef,
            else => WasmError.InvalidValType,
        };
    }

    pub fn toByte(): u8 {
        return @intFromEnum(_);
    }
}

pub enum SectionId: u8 {
    Custom = 0,
    Type = 1,
    Import = 2,
    Function = 3,
    Table = 4,
    Memory = 5,
    Global = 6,
    Export = 7,
    Start = 8,
    Element = 9,
    Code = 10,
    Data = 11,

    pub static fn fromByte(b: u8): WasmError!SectionId {
        return switch (b) {
            0 => .Custom,
            1 => .Type,
            2 => .Import,
            3 => .Function,
            4 => .Table,
            5 => .Memory,
            6 => .Global,
            7 => .Export,
            8 => .Start,
            9 => .Element,
            10 => .Code,
            11 => .Data,
            else => WasmError.InvalidSectionId,
        };
    }

    pub fn toByte(): u8 {
        return @intFromEnum(_);
    }
}

pub enum ExternalKind: u8 {
    Func = 0x00,
    Table = 0x01,
    Memory = 0x02,
    Global = 0x03,

    pub static fn fromByte(b: u8): WasmError!ExternalKind {
        return switch (b) {
            0x00 => .Func,
            0x01 => .Table,
            0x02 => .Memory,
            0x03 => .Global,
            else => WasmError.InvalidExternalKind,
        };
    }

    pub fn toByte(): u8 {
        return @intFromEnum(_);
    }
}

pub struct FuncType {
    pub params: []const ValType = &.{},
    pub results: []const ValType = &.{},
}

pub struct WasmLimits {
    pub min: u32 = 0,
    pub max: u32? = null,
}

pub struct WasmImport {
    pub module: []const u8 = "",
    pub name: []const u8 = "",
    pub kind: ExternalKind = .Func,
    pub type_index: u32 = 0,
    pub memory_limits: WasmLimits? = null,
}

pub struct WasmExport {
    pub name: []const u8 = "",
    pub kind: ExternalKind = .Func,
    pub index: u32 = 0,
}

pub struct WasmMemory {
    pub limits: WasmLimits = .{},
}

pub struct WasmGlobal {
    pub val_type: ValType = .I32,
    pub mutable: bool = false,
    pub init_expr: []const u8 = &.{},
}

pub struct CustomSection {
    pub name: []const u8 = "",
    pub data: []const u8 = &.{},
}

pub struct WasmModule {
    pub types: []const FuncType = &.{},
    pub imports: []const WasmImport = &.{},
    pub functions: []const u32 = &.{},
    pub memories: []const WasmMemory = &.{},
    pub globals: []const WasmGlobal = &.{},
    pub exports: []const WasmExport = &.{},
    pub start: u32? = null,
    pub codes: []const []const u8 = &.{},
    pub custom_sections: []const CustomSection = &.{},

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        let i: usize = 0;
        while (i < _.types.len) {
            alloc.free(_.types[i].params);
            alloc.free(_.types[i].results);
            i += 1;
        }
        alloc.free(_.types);

        i = 0;
        while (i < _.imports.len) {
            alloc.free(_.imports[i].module);
            alloc.free(_.imports[i].name);
            i += 1;
        }
        alloc.free(_.imports);

        alloc.free(_.functions);
        alloc.free(_.memories);

        i = 0;
        while (i < _.globals.len) {
            alloc.free(_.globals[i].init_expr);
            i += 1;
        }
        alloc.free(_.globals);

        i = 0;
        while (i < _.exports.len) {
            alloc.free(_.exports[i].name);
            i += 1;
        }
        alloc.free(_.exports);

        i = 0;
        while (i < _.codes.len) {
            alloc.free(_.codes[i]);
            i += 1;
        }
        alloc.free(_.codes);

        i = 0;
        while (i < _.custom_sections.len) {
            alloc.free(_.custom_sections[i].name);
            alloc.free(_.custom_sections[i].data);
            i += 1;
        }
        alloc.free(_.custom_sections);

        _.* = .{};
    }
}
