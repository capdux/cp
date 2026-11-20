// Source: cp/lib/wasm/builder.cp (Minimal WebAssembly 1.0 Binary Module Builder)
// Pure CP WebAssembly 1.0 binary module encoder producing spec-compliant `.wasm`
// binaries with Type, Import, Function, Memory, Export, Start, Code & Custom sections.

const std = @import("std");
import { WasmError, Leb128 } from "./leb128.cp";
import {
    WASM_MAGIC,
    WASM_VERSION,
    WasmValType,
    WasmExternKind,
    WasmLimits,
    WasmFuncType,
    WasmImport,
    WasmExport,
    WasmCustomSection,
    WasmLocalDecl,
} from "./module.cp";

struct BuilderCodeEntry {
    locals: []const WasmLocalDecl = [],
    body: []const u8 = "",
}

fn appendName(out: *std.ArrayList(u8), alloc: std.mem.Allocator, name: []const u8): void ! WasmError {
    Leb128.writeU32(out, alloc, name.len as u32)?;
    if (name.len > 0) {
        out.appendSlice(alloc, name) catch return WasmError.OutOfMemory;
    }
}

fn appendSection(out: *std.ArrayList(u8), alloc: std.mem.Allocator, sec_id: u8, payload: []const u8): void ! WasmError {
    out.append(alloc, sec_id) catch return WasmError.OutOfMemory;
    Leb128.writeU32(out, alloc, payload.len as u32)?;
    if (payload.len > 0) {
        out.appendSlice(alloc, payload) catch return WasmError.OutOfMemory;
    }
}

fn appendLimits(out: *std.ArrayList(u8), alloc: std.mem.Allocator, lim: WasmLimits): void ! WasmError {
    if (max_p := lim.max) {
        out.append(alloc, 0x01) catch return WasmError.OutOfMemory;
        Leb128.writeU32(out, alloc, lim.min)?;
        Leb128.writeU32(out, alloc, max_p)?;
    } else {
        out.append(alloc, 0x00) catch return WasmError.OutOfMemory;
        Leb128.writeU32(out, alloc, lim.min)?;
    }
}

/// Incremental WebAssembly 1.0 binary module builder backed by an internal Arena.
pub struct WasmBuilder {
    owned_arena: (*std.heap.ArenaAllocator)? = null,
    parent_alloc: std.mem.Allocator? = null,
    types: std.ArrayList(WasmFuncType) = .empty,
    imports: std.ArrayList(WasmImport) = .empty,
    functions: std.ArrayList(u32) = .empty,
    memories: std.ArrayList(WasmLimits) = .empty,
    exports: std.ArrayList(WasmExport) = .empty,
    codes: std.ArrayList(BuilderCodeEntry) = .empty,
    custom_sections: std.ArrayList(WasmCustomSection) = .empty,
    start_func: u32? = null,
    import_func_count: u32 = 0,

    /// Initializes a new `WasmBuilder` whose temporary section data is managed by an internal Arena.
    pub static fn init(alloc: std.mem.Allocator): WasmBuilder {
        let arena_ptr = alloc.create(std.heap.ArenaAllocator) catch {
            return WasmBuilder{
                owned_arena: null,
                parent_alloc: alloc,
            };
        };
        unsafe {
            arena_ptr.* = std.heap.ArenaAllocator.init(alloc);
        }
        return WasmBuilder{
            owned_arena: arena_ptr,
            parent_alloc: alloc,
        };
    }

    /// Releases all memory owned by this `WasmBuilder`.
    pub fn deinit(*_) {
        if (owned := _.owned_arena) {
            owned.deinit();
            if (p := _.parent_alloc) {
                p.destroy(owned);
            }
            _.owned_arena = null;
            _.parent_alloc = null;
        }
        _.types = .empty;
        _.imports = .empty;
        _.functions = .empty;
        _.memories = .empty;
        _.exports = .empty;
        _.codes = .empty;
        _.custom_sections = .empty;
        _.start_func = null;
        _.import_func_count = 0;
    }

    fn arenaAlloc(): (std.mem.Allocator) ! WasmError {
        if (owned := _.owned_arena) {
            return owned.allocator();
        }
        return WasmError.OutOfMemory;
    }

    /// Adds a function signature `(params) -> (results)` to the Type section and returns its type index.
    pub fn addType(*_, params: []const WasmValType, results: []const WasmValType): u32 ! WasmError {
        const arena_alloc = _.arenaAlloc()?;
        const idx = _.types.items.len as u32;
        const p_dup = arena_alloc.dupe(WasmValType, params) catch return WasmError.OutOfMemory;
        const r_dup = arena_alloc.dupe(WasmValType, results) catch return WasmError.OutOfMemory;
        _.types.append(arena_alloc, WasmFuncType{
            params: p_dup,
            results: r_dup,
        }) catch return WasmError.OutOfMemory;
        return idx;
    }

    /// Adds an imported function `(module, name, type_idx)` to the Import section and returns its function index.
    /// Note: per WebAssembly 1.0 index space ordering, call `addImportFunc` before `addFunction`.
    pub fn addImportFunc(*_, module: []const u8, name: []const u8, type_idx: u32): u32 ! WasmError {
        const arena_alloc = _.arenaAlloc()?;
        const idx = _.import_func_count;
        const m_dup = arena_alloc.dupe(u8, module) catch return WasmError.OutOfMemory;
        const n_dup = arena_alloc.dupe(u8, name) catch return WasmError.OutOfMemory;
        _.imports.append(arena_alloc, WasmImport{
            module: m_dup,
            name: n_dup,
            kind: .Func,
            type_idx: type_idx,
        }) catch return WasmError.OutOfMemory;
        _.import_func_count += 1;
        return idx;
    }

    /// Adds a locally defined function with signature `type_idx` and instruction `body_bytes` (0 locals).
    /// Returns the global function index (`import_func_count + local_index`).
    pub fn addFunction(*_, type_idx: u32, body_bytes: []const u8): u32 ! WasmError {
        return _.addFunctionWithLocals(type_idx, [], body_bytes);
    }

    /// Adds a locally defined function with explicit local declarations and instruction `body_bytes`.
    /// Returns the global function index (`import_func_count + local_index`).
    pub fn addFunctionWithLocals(
        *_,
        type_idx: u32,
        locals: []const WasmLocalDecl,
        body_bytes: []const u8,
    ): u32 ! WasmError {
        const arena_alloc = _.arenaAlloc()?;
        const local_idx = _.functions.items.len as u32;
        const l_dup = arena_alloc.dupe(WasmLocalDecl, locals) catch return WasmError.OutOfMemory;
        const b_dup = arena_alloc.dupe(u8, body_bytes) catch return WasmError.OutOfMemory;
        _.functions.append(arena_alloc, type_idx) catch return WasmError.OutOfMemory;
        _.codes.append(arena_alloc, BuilderCodeEntry{
            locals: l_dup,
            body: b_dup,
        }) catch return WasmError.OutOfMemory;
        return _.import_func_count + local_idx;
    }

    /// Adds a linear memory definition `(min_pages, max_pages)` to the Memory section and returns its memory index.
    pub fn addMemory(*_, min_pages: u32, max_pages: u32?): u32 ! WasmError {
        const arena_alloc = _.arenaAlloc()?;
        if (max_p := max_pages) {
            if (max_p < min_pages) {
                return WasmError.MalformedSection;
            }
        }
        const idx = _.memories.items.len as u32;
        _.memories.append(arena_alloc, WasmLimits{
            min: min_pages,
            max: max_pages,
        }) catch return WasmError.OutOfMemory;
        return idx;
    }

    /// Adds an export entry `(name, kind, index)` to the Export section.
    pub fn addExport(*_, name: []const u8, kind: WasmExternKind, index: u32): void ! WasmError {
        const arena_alloc = _.arenaAlloc()?;
        const n_dup = arena_alloc.dupe(u8, name) catch return WasmError.OutOfMemory;
        _.exports.append(arena_alloc, WasmExport{
            name: n_dup,
            kind: kind,
            index: index,
        }) catch return WasmError.OutOfMemory;
    }

    /// Sets the optional Start section entry function index.
    pub fn setStart(*_, func_idx: u32) {
        _.start_func = func_idx;
    }

    /// Adds a Custom section (`id = 0`) with `name` and raw `data` payload.
    pub fn addCustomSection(*_, name: []const u8, data: []const u8): void ! WasmError {
        const arena_alloc = _.arenaAlloc()?;
        const n_dup = arena_alloc.dupe(u8, name) catch return WasmError.OutOfMemory;
        const d_dup = arena_alloc.dupe(u8, data) catch return WasmError.OutOfMemory;
        _.custom_sections.append(arena_alloc, WasmCustomSection{
            name: n_dup,
            data: d_dup,
        }) catch return WasmError.OutOfMemory;
    }

    /// Serializes all configured sections into a spec-compliant WebAssembly 1.0 binary slice.
    /// Caller owns the returned slice and must free it with `alloc.free(...)`.
    pub fn build(alloc: std.mem.Allocator): ([]u8) ! WasmError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        // 1. Magic + Version header
        out.appendSlice(alloc, WASM_MAGIC[0..]) catch return WasmError.OutOfMemory;
        const ver_bytes: [4]u8 = [
            (WASM_VERSION & 0xFF) as u8,
            ((WASM_VERSION >> 8) & 0xFF) as u8,
            ((WASM_VERSION >> 16) & 0xFF) as u8,
            ((WASM_VERSION >> 24) & 0xFF) as u8,
        ];
        out.appendSlice(alloc, &ver_bytes) catch return WasmError.OutOfMemory;

        let sec: std.ArrayList(u8) = .empty;
        defer sec.deinit(alloc);

        // 2. Type Section (id = 1)
        if (_.types.items.len > 0) {
            sec.clearRetainingCapacity();
            Leb128.writeU32(&sec, alloc, _.types.items.len as u32)?;
            for (ft of _.types.items) {
                sec.append(alloc, 0x60) catch return WasmError.OutOfMemory;
                Leb128.writeU32(&sec, alloc, ft.params.len as u32)?;
                for (pt of ft.params) {
                    sec.append(alloc, pt.toByte()) catch return WasmError.OutOfMemory;
                }
                Leb128.writeU32(&sec, alloc, ft.results.len as u32)?;
                for (rt of ft.results) {
                    sec.append(alloc, rt.toByte()) catch return WasmError.OutOfMemory;
                }
            }
            appendSection(&out, alloc, 1, sec.items)?;
        }

        // 3. Import Section (id = 2)
        if (_.imports.items.len > 0) {
            sec.clearRetainingCapacity();
            Leb128.writeU32(&sec, alloc, _.imports.items.len as u32)?;
            for (imp of _.imports.items) {
                appendName(&sec, alloc, imp.module)?;
                appendName(&sec, alloc, imp.name)?;
                sec.append(alloc, imp.kind.toByte()) catch return WasmError.OutOfMemory;
                Leb128.writeU32(&sec, alloc, imp.type_idx)?;
            }
            appendSection(&out, alloc, 2, sec.items)?;
        }

        // 4. Function Section (id = 3)
        if (_.functions.items.len > 0) {
            sec.clearRetainingCapacity();
            Leb128.writeU32(&sec, alloc, _.functions.items.len as u32)?;
            for (tidx of _.functions.items) {
                Leb128.writeU32(&sec, alloc, tidx)?;
            }
            appendSection(&out, alloc, 3, sec.items)?;
        }

        // 5. Memory Section (id = 5)
        if (_.memories.items.len > 0) {
            sec.clearRetainingCapacity();
            Leb128.writeU32(&sec, alloc, _.memories.items.len as u32)?;
            for (lim of _.memories.items) {
                appendLimits(&sec, alloc, lim)?;
            }
            appendSection(&out, alloc, 5, sec.items)?;
        }

        // 6. Export Section (id = 7)
        if (_.exports.items.len > 0) {
            sec.clearRetainingCapacity();
            Leb128.writeU32(&sec, alloc, _.exports.items.len as u32)?;
            for (exp of _.exports.items) {
                appendName(&sec, alloc, exp.name)?;
                sec.append(alloc, exp.kind.toByte()) catch return WasmError.OutOfMemory;
                Leb128.writeU32(&sec, alloc, exp.index)?;
            }
            appendSection(&out, alloc, 7, sec.items)?;
        }

        // 7. Start Section (id = 8)
        if (start_idx := _.start_func) {
            sec.clearRetainingCapacity();
            Leb128.writeU32(&sec, alloc, start_idx)?;
            appendSection(&out, alloc, 8, sec.items)?;
        }

        // 8. Code Section (id = 10)
        if (_.codes.items.len > 0) {
            sec.clearRetainingCapacity();
            Leb128.writeU32(&sec, alloc, _.codes.items.len as u32)?;
            let body_buf: std.ArrayList(u8) = .empty;
            defer body_buf.deinit(alloc);
            for (entry of _.codes.items) {
                body_buf.clearRetainingCapacity();
                Leb128.writeU32(&body_buf, alloc, entry.locals.len as u32)?;
                for (loc of entry.locals) {
                    Leb128.writeU32(&body_buf, alloc, loc.count)?;
                    body_buf.append(alloc, loc.val_type.toByte()) catch return WasmError.OutOfMemory;
                }
                if (entry.body.len > 0) {
                    body_buf.appendSlice(alloc, entry.body) catch return WasmError.OutOfMemory;
                }
                Leb128.writeU32(&sec, alloc, body_buf.items.len as u32)?;
                sec.appendSlice(alloc, body_buf.items) catch return WasmError.OutOfMemory;
            }
            appendSection(&out, alloc, 10, sec.items)?;
        }

        // 9. Custom Sections (id = 0)
        for (cs of _.custom_sections.items) {
            sec.clearRetainingCapacity();
            appendName(&sec, alloc, cs.name)?;
            if (cs.data.len > 0) {
                sec.appendSlice(alloc, cs.data) catch return WasmError.OutOfMemory;
            }
            appendSection(&out, alloc, 0, sec.items)?;
        }

        return out.toOwnedSlice(alloc) catch return WasmError.OutOfMemory;
    }
}
