// Source: cp/lib/wasm/module.cp (WebAssembly 1.0 Binary Module Parser & Validator)
// Pure CP Arena-backed WebAssembly 1.0 binary parser validating magic `\0asm`,
// version `1`, strictly increasing section order, Type/Import/Function/Memory/
// Export/Start/Code/Custom sections, and export/custom lookup helpers.

const std = @import("std");
import { WasmError, Leb128 } from "./leb128.cp";

/// WebAssembly 1.0 binary magic bytes (`\0asm`).
pub const WASM_MAGIC: [4]u8 = [0x00, 0x61, 0x73, 0x6D];

/// WebAssembly 1.0 binary format version (`1`).
pub const WASM_VERSION: u32 = 1;

/// WebAssembly value types (number types & reference types).
pub enum WasmValType: u8 {
    I32 = 0x7F,
    I64 = 0x7E,
    F32 = 0x7D,
    F64 = 0x7C,
    FuncRef = 0x70,
    ExternRef = 0x6F,

    static fn fromByte(b: u8): WasmValType ! WasmError {
        return switch (b) {
            0x7F => .I32,
            0x7E => .I64,
            0x7D => .F32,
            0x7C => .F64,
            0x70 => .FuncRef,
            0x6F => .ExternRef,
            else => return WasmError.InvalidValType,
        };
    }

    fn toByte(): u8 {
        return _ as u8;
    }
}

/// WebAssembly external import/export descriptor kind.
pub enum WasmExternKind: u8 {
    Func = 0,
    Table = 1,
    Memory = 2,
    Global = 3,

    static fn fromByte(b: u8): WasmExternKind ! WasmError {
        return switch (b) {
            0 => .Func,
            1 => .Table,
            2 => .Memory,
            3 => .Global,
            else => return WasmError.InvalidExportKind,
        };
    }

    fn toByte(): u8 {
        return _ as u8;
    }
}

/// WebAssembly 1.0 binary section identifiers.
pub enum WasmSectionId: u8 {
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
    DataCount = 12,
}

/// Memory or table size limits (`min` and optional `max` in pages/elements).
pub struct WasmLimits {
    min: u32 = 0,
    max: u32? = null,
}

/// Function signature (`params -> results`) in the Type section.
pub struct WasmFuncType {
    params: []const WasmValType = [],
    results: []const WasmValType = [],
}

/// Imported entity entry from the Import section.
pub struct WasmImport {
    module: []const u8 = "",
    name: []const u8 = "",
    kind: WasmExternKind = .Func,
    type_idx: u32 = 0,
}

/// Exported entity entry from the Export section.
pub struct WasmExport {
    name: []const u8 = "",
    kind: WasmExternKind = .Func,
    index: u32 = 0,
}

/// Custom section (`id = 0`) carrying a UTF-8 name and raw payload bytes.
pub struct WasmCustomSection {
    name: []const u8 = "",
    data: []const u8 = "",
}

/// Local variable declaration group inside a function code body.
pub struct WasmLocalDecl {
    count: u32 = 0,
    val_type: WasmValType = .I32,
}

/// Decoded function body from the Code section (`locals` declarations + `code` instructions).
pub struct WasmCodeBody {
    locals: []const WasmLocalDecl = [],
    code: []const u8 = "",
    raw: []const u8 = "",
}

fn readByte(bytes: []const u8, pos: *usize): u8 ! WasmError {
    if (pos.* >= bytes.len) {
        return WasmError.UnexpectedEof;
    }
    const b = bytes[pos.*];
    pos.* += 1;
    return b;
}

fn readSlice(bytes: []const u8, pos: *usize, len: usize): ([]const u8) ! WasmError {
    if (len > bytes.len - pos.*) {
        return WasmError.UnexpectedEof;
    }
    const s = bytes[pos.* .. pos.* + len];
    pos.* += len;
    return s;
}

fn readName(alloc: std.mem.Allocator, bytes: []const u8, pos: *usize): ([]const u8) ! WasmError {
    const len_u32 = Leb128.decodeU32(bytes, pos)?;
    const raw = readSlice(bytes, pos, len_u32 as usize)?;
    if (!std.unicode.utf8ValidateSlice(raw)) {
        return WasmError.MalformedSection;
    }
    return alloc.dupe(u8, raw) catch return WasmError.OutOfMemory;
}

fn readLimits(bytes: []const u8, pos: *usize): WasmLimits ! WasmError {
    const flags = readByte(bytes, pos)?;
    if (flags == 0x00) {
        const min_val = Leb128.decodeU32(bytes, pos)?;
        return WasmLimits{ min: min_val, max: null };
    }
    if (flags == 0x01) {
        const min_val = Leb128.decodeU32(bytes, pos)?;
        const max_val = Leb128.decodeU32(bytes, pos)?;
        if (max_val < min_val) {
            return WasmError.MalformedSection;
        }
        return WasmLimits{ min: min_val, max: max_val };
    }
    return WasmError.MalformedSection;
}

/// Parsed WebAssembly 1.0 binary module backed by an `ArenaAllocator`.
/// Call `mod.deinit()` to release all decoded slices and metadata in O(1).
pub struct WasmModule {
    version: u32 = WASM_VERSION,
    types: []const WasmFuncType = [],
    imports: []const WasmImport = [],
    functions: []const u32 = [],
    memories: []const WasmLimits = [],
    exports: []const WasmExport = [],
    start_func: u32? = null,
    codes: []const WasmCodeBody = [],
    custom_sections: []const WasmCustomSection = [],
    owned_arena: (*std.heap.ArenaAllocator)? = null,
    parent_alloc: std.mem.Allocator? = null,

    /// Releases all Arena-allocated metadata owned by this `WasmModule`.
    pub fn deinit(*_) {
        if (owned := _.owned_arena) {
            owned.deinit();
            if (p := _.parent_alloc) {
                p.destroy(owned);
            }
            _.owned_arena = null;
            _.parent_alloc = null;
        }
        _.types = [];
        _.imports = [];
        _.functions = [];
        _.memories = [];
        _.exports = [];
        _.start_func = null;
        _.codes = [];
        _.custom_sections = [];
    }

    /// Parses and validates a WebAssembly 1.0 binary buffer into an Arena-owned `WasmModule`.
    pub static fn parse(alloc: std.mem.Allocator, bytes: []const u8): WasmModule ! WasmError {
        if (bytes.len < 8) {
            return WasmError.UnexpectedEof;
        }
        if (bytes[0] != 0x00 || bytes[1] != 0x61 || bytes[2] != 0x73 || bytes[3] != 0x6D) {
            return WasmError.InvalidMagic;
        }
        const ver: u32 = (bytes[4] as u32) |
            ((bytes[5] as u32) << 8) |
            ((bytes[6] as u32) << 16) |
            ((bytes[7] as u32) << 24);
        if (ver != WASM_VERSION) {
            return WasmError.UnsupportedVersion;
        }

        let arena_ptr = alloc.create(std.heap.ArenaAllocator) catch return WasmError.OutOfMemory;
        unsafe {
            arena_ptr.* = std.heap.ArenaAllocator.init(alloc);
        }
        let mod = WasmModule{
            version: ver,
            owned_arena: arena_ptr,
            parent_alloc: alloc,
        };
        errdefer mod.deinit();

        const arena_alloc = arena_ptr.allocator();

        let custom_list: std.ArrayList(WasmCustomSection) = .empty;
        let pos: usize = 8;
        let last_known_sec: u8 = 0;

        while (pos < bytes.len) {
            const sec_id = readByte(bytes, &pos)?;
            const sec_len_u32 = Leb128.decodeU32(bytes, &pos)?;
            const sec_len: usize = sec_len_u32 as usize;
            if (sec_len > bytes.len - pos) {
                return WasmError.UnexpectedEof;
            }
            const sec_bytes = bytes[pos .. pos + sec_len];
            pos += sec_len;

            if (sec_id == 0) {
                let spos: usize = 0;
                const cname = readName(arena_alloc, sec_bytes, &spos)?;
                const payload = arena_alloc.dupe(u8, sec_bytes[spos..]) catch return WasmError.OutOfMemory;
                custom_list.append(arena_alloc, WasmCustomSection{
                    name: cname,
                    data: payload,
                }) catch return WasmError.OutOfMemory;
                continue;
            }

            if (sec_id > 12) {
                return WasmError.MalformedSection;
            }
            if (sec_id <= last_known_sec) {
                return WasmError.InvalidSectionOrder;
            }
            last_known_sec = sec_id;

            let spos: usize = 0;
            switch (sec_id) {
                1 => {
                    const count = Leb128.decodeU32(sec_bytes, &spos)? as usize;
                    let items = arena_alloc.alloc(WasmFuncType, count) catch return WasmError.OutOfMemory;
                    let i: usize = 0;
                    while (i < count) : (i += 1) {
                        const tag = readByte(sec_bytes, &spos)?;
                        if (tag != 0x60) {
                            return WasmError.MalformedSection;
                        }
                        const p_count = Leb128.decodeU32(sec_bytes, &spos)? as usize;
                        let params = arena_alloc.alloc(WasmValType, p_count) catch return WasmError.OutOfMemory;
                        let pi: usize = 0;
                        while (pi < p_count) : (pi += 1) {
                            params[pi] = WasmValType.fromByte(readByte(sec_bytes, &spos)?)?;
                        }
                        const r_count = Leb128.decodeU32(sec_bytes, &spos)? as usize;
                        let results = arena_alloc.alloc(WasmValType, r_count) catch return WasmError.OutOfMemory;
                        let ri: usize = 0;
                        while (ri < r_count) : (ri += 1) {
                            results[ri] = WasmValType.fromByte(readByte(sec_bytes, &spos)?)?;
                        }
                        items[i] = WasmFuncType{
                            params: params,
                            results: results,
                        };
                    }
                    if (spos != sec_bytes.len) {
                        return WasmError.MalformedSection;
                    }
                    mod.types = items;
                },
                2 => {
                    const count = Leb128.decodeU32(sec_bytes, &spos)? as usize;
                    let items = arena_alloc.alloc(WasmImport, count) catch return WasmError.OutOfMemory;
                    let i: usize = 0;
                    while (i < count) : (i += 1) {
                        const mod_name = readName(arena_alloc, sec_bytes, &spos)?;
                        const field_name = readName(arena_alloc, sec_bytes, &spos)?;
                        const kind_b = readByte(sec_bytes, &spos)?;
                        const kind = WasmExternKind.fromByte(kind_b)?;
                        let tidx: u32 = 0;
                        switch (kind) {
                            .Func => {
                                tidx = Leb128.decodeU32(sec_bytes, &spos)?;
                            },
                            .Table => {
                                const elem_b = readByte(sec_bytes, &spos)?;
                                const elem_t = WasmValType.fromByte(elem_b)?;
                                if (elem_t != .FuncRef && elem_t != .ExternRef) {
                                    return WasmError.InvalidValType;
                                }
                                * = readLimits(sec_bytes, &spos)?;
                            },
                            .Memory => {
                                * = readLimits(sec_bytes, &spos)?;
                            },
                            .Global => {
                                * = WasmValType.fromByte(readByte(sec_bytes, &spos)?)?;
                                const mut_flag = readByte(sec_bytes, &spos)?;
                                if (mut_flag > 1) {
                                    return WasmError.MalformedSection;
                                }
                            },
                        }
                        items[i] = WasmImport{
                            module: mod_name,
                            name: field_name,
                            kind: kind,
                            type_idx: tidx,
                        };
                    }
                    if (spos != sec_bytes.len) {
                        return WasmError.MalformedSection;
                    }
                    mod.imports = items;
                },
                3 => {
                    const count = Leb128.decodeU32(sec_bytes, &spos)? as usize;
                    let items = arena_alloc.alloc(u32, count) catch return WasmError.OutOfMemory;
                    let i: usize = 0;
                    while (i < count) : (i += 1) {
                        items[i] = Leb128.decodeU32(sec_bytes, &spos)?;
                    }
                    if (spos != sec_bytes.len) {
                        return WasmError.MalformedSection;
                    }
                    mod.functions = items;
                },
                5 => {
                    const count = Leb128.decodeU32(sec_bytes, &spos)? as usize;
                    let items = arena_alloc.alloc(WasmLimits, count) catch return WasmError.OutOfMemory;
                    let i: usize = 0;
                    while (i < count) : (i += 1) {
                        items[i] = readLimits(sec_bytes, &spos)?;
                    }
                    if (spos != sec_bytes.len) {
                        return WasmError.MalformedSection;
                    }
                    mod.memories = items;
                },
                7 => {
                    const count = Leb128.decodeU32(sec_bytes, &spos)? as usize;
                    let items = arena_alloc.alloc(WasmExport, count) catch return WasmError.OutOfMemory;
                    let i: usize = 0;
                    while (i < count) : (i += 1) {
                        const ename = readName(arena_alloc, sec_bytes, &spos)?;
                        const kind_b = readByte(sec_bytes, &spos)?;
                        const kind = WasmExternKind.fromByte(kind_b)?;
                        const idx = Leb128.decodeU32(sec_bytes, &spos)?;
                        items[i] = WasmExport{
                            name: ename,
                            kind: kind,
                            index: idx,
                        };
                    }
                    if (spos != sec_bytes.len) {
                        return WasmError.MalformedSection;
                    }
                    mod.exports = items;
                },
                8 => {
                    const start_idx = Leb128.decodeU32(sec_bytes, &spos)?;
                    if (spos != sec_bytes.len) {
                        return WasmError.MalformedSection;
                    }
                    mod.start_func = start_idx;
                },
                10 => {
                    const count = Leb128.decodeU32(sec_bytes, &spos)? as usize;
                    if (count != mod.functions.len) {
                        return WasmError.MalformedSection;
                    }
                    let items = arena_alloc.alloc(WasmCodeBody, count) catch return WasmError.OutOfMemory;
                    let i: usize = 0;
                    while (i < count) : (i += 1) {
                        const body_size = Leb128.decodeU32(sec_bytes, &spos)? as usize;
                        if (body_size > sec_bytes.len - spos) {
                            return WasmError.MalformedSection;
                        }
                        const raw_slice = sec_bytes[spos .. spos + body_size];
                        spos += body_size;

                        const raw_duped = arena_alloc.dupe(u8, raw_slice) catch return WasmError.OutOfMemory;
                        let bpos: usize = 0;
                        const local_decl_count = Leb128.decodeU32(raw_duped, &bpos)? as usize;
                        let locals = arena_alloc.alloc(WasmLocalDecl, local_decl_count) catch return WasmError.OutOfMemory;
                        let li: usize = 0;
                        while (li < local_decl_count) : (li += 1) {
                            const lcount = Leb128.decodeU32(raw_duped, &bpos)?;
                            const ltype = WasmValType.fromByte(readByte(raw_duped, &bpos)?)?;
                            locals[li] = WasmLocalDecl{
                                count: lcount,
                                val_type: ltype,
                            };
                        }
                        items[i] = WasmCodeBody{
                            locals: locals,
                            code: raw_duped[bpos..],
                            raw: raw_duped,
                        };
                    }
                    if (spos != sec_bytes.len) {
                        return WasmError.MalformedSection;
                    }
                    mod.codes = items;
                },
                else => {
                    // Validated section ordering for Table(4), Global(6), Element(9), Data(11), DataCount(12)
                },
            }
        }

        if (mod.functions.len != mod.codes.len) {
            return WasmError.MalformedSection;
        }

        mod.custom_sections = custom_list.toOwnedSlice(arena_alloc) catch return WasmError.OutOfMemory;
        return mod;
    }

    /// Finds an export entry by exact UTF-8 `name`.
    pub fn findExport(name: []const u8): WasmExport? {
        for (exp of _.exports) {
            if (std.mem.eql(u8, exp.name, name)) {
                return exp;
            }
        }
        return null;
    }

    /// Finds the first custom section matching `name`.
    pub fn findCustomSection(name: []const u8): WasmCustomSection? {
        for (cs of _.custom_sections) {
            if (std.mem.eql(u8, cs.name, name)) {
                return cs;
            }
        }
        return null;
    }

    /// Returns the number of imported functions (which precede local functions in index space).
    pub fn importedFuncCount(): u32 {
        let count: u32 = 0;
        for (imp of _.imports) {
            if (imp.kind == .Func) {
                count += 1;
            }
        }
        return count;
    }

    /// Resolves the `WasmFuncType` of a function by its global function index (imports + locals).
    pub fn getFuncType(func_idx: u32): WasmFuncType? {
        let cur: u32 = 0;
        for (imp of _.imports) {
            if (imp.kind == .Func) {
                if (cur == func_idx) {
                    if ((imp.type_idx as usize) < _.types.len) {
                        return _.types[imp.type_idx as usize];
                    }
                    return null;
                }
                cur += 1;
            }
        }
        const local_idx = func_idx - cur;
        if ((local_idx as usize) < _.functions.len) {
            const tidx = _.functions[local_idx as usize];
            if ((tidx as usize) < _.types.len) {
                return _.types[tidx as usize];
            }
        }
        return null;
    }

    /// Returns the decoded `WasmCodeBody` for a global function index (if it is a locally defined function).
    pub fn getCodeBody(func_idx: u32): WasmCodeBody? {
        const imp_count = _.importedFuncCount();
        if (func_idx < imp_count) {
            return null;
        }
        const local_idx = (func_idx - imp_count) as usize;
        if (local_idx < _.codes.len) {
            return _.codes[local_idx];
        }
        return null;
    }
}
