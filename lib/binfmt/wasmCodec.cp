// Source: cp/lib/wasm_fmt/codec.cp (Pure CP WebAssembly binary parser, encoder & helpers)
// Reference-hardened against bytecodealliance/wasm-tools (Apache-2.0)
const std = @import("std");
import { Simd } from "cap:simd";
import {
    WASM_MAGIC,
    WASM_VERSION,
    WASM_MAX_SECTION_ITEMS,
    WASM_MAX_NAME_LEN,
    WasmError,
    ValType,
    SectionId,
    ExternalKind,
    FuncType,
    WasmLimits,
    WasmImport,
    WasmExport,
    WasmMemory,
    WasmGlobal,
    CustomSection,
    WasmModule,
} from "./wasmTypes.cp";
import { readUleb128, writeUleb128 } from "./wasmLeb128.cp";

type WasmVec16 = @Vector(16, u8);

/// SIMD-accelerated UTF-8 validator for WebAssembly Custom/Import/Export names:
/// fast-paths pure ASCII via `cap:simd` `Simd.isAscii`, falling back to strict RFC 3629 UTF-8 check.
pub fn simdIsValidWasmUtf8(s: []const u8): bool {
    if (Simd.isAscii(s)) return true;
    return std.unicode.utf8ValidateSlice(s);
}

/// 16-lane `@Vector(16, u8)` SIMD payload copy for WebAssembly section and code bodies.
pub fn simdWasmCopyPayload(dst: []u8, src: []const u8): usize {
    const n = if (dst.len < src.len) dst.len else src.len;
    let i: usize = 0;
    while (i + 16 <= n) {
        const chunk: WasmVec16 = src[i..][0..16].*;
        dst[i..][0..16].* = chunk;
        i += 16;
    }
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
    return n;
}

fn readByte(data: []const u8, offset: *usize): u8 ! WasmError {
    if (offset.* >= data.len) {
        return WasmError.UnexpectedEof;
    }
    const b = data[offset.*];
    offset.* += 1;
    return b;
}

fn readBytesSlice(data: []const u8, offset: *usize, len: usize): ([]const u8) ! WasmError {
    if (len > data.len - offset.*) {
        return WasmError.UnexpectedEof;
    }
    const s = data[offset.* .. offset.* + len];
    offset.* += len;
    return s;
}

fn readNameSlice(data: []const u8, offset: *usize): ([]const u8) ! WasmError {
    const len = try readUleb128(data, offset);
    if (len > WASM_MAX_NAME_LEN) {
        return WasmError.TooManyItems;
    }
    const raw = try readBytesSlice(data, offset, @as(usize, len));
    if (!simdIsValidWasmUtf8(raw)) {
        return WasmError.InvalidUtf8;
    }
    return raw;
}

fn readName(alloc: std.mem.Allocator, data: []const u8, offset: *usize): ([]u8) ! WasmError {
    const raw = try readNameSlice(data, offset);
    let dup = try alloc.alloc(u8, raw.len);
    const copied = simdWasmCopyPayload(dup, raw);
    if (copied != raw.len) return WasmError.UnexpectedEof;
    return dup;
}

fn readLimits(data: []const u8, offset: *usize): WasmLimits ! WasmError {
    const flags = try readByte(data, offset);
    if (flags == 0x00) {
        const min_val = try readUleb128(data, offset);
        return WasmLimits{ .min = min_val, .max = null };
    } else if (flags == 0x01) {
        const min_val = try readUleb128(data, offset);
        const max_val = try readUleb128(data, offset);
        if (max_val < min_val) {
            return WasmError.InvalidLimitsRange;
        }
        return WasmLimits{ .min = min_val, .max = max_val };
    }
    return WasmError.InvalidLimitFlags;
}

fn checkVectorCount(count: u32, rem_bytes: usize): u32 ! WasmError {
    if (count > WASM_MAX_SECTION_ITEMS) {
        return WasmError.TooManyItems;
    }
    if (@as(usize, count) > rem_bytes) {
        return WasmError.UnexpectedEof;
    }
    return count;
}

fn parseTypeSection(alloc: std.mem.Allocator, payload: []const u8): ([]FuncType) ! WasmError {
    let off: usize = 0;
    const count = try readUleb128(payload, &off);
    try checkVectorCount(count, payload.len - off);
    let list: std.ArrayList(FuncType) = .empty;
    errdefer {
        let j: usize = 0;
        while (j < list.items.len) {
            alloc.free(list.items[j].params);
            alloc.free(list.items[j].results);
            j += 1;
        }
        list.deinit(alloc);
    }

    let i: u32 = 0;
    while (i < count) {
        const tag = try readByte(payload, &off);
        if (tag != 0x60) {
            return WasmError.InvalidFuncTypeTag;
        }

        const p_count = try readUleb128(payload, &off);
        try checkVectorCount(p_count, payload.len - off);
        let params = try alloc.alloc(ValType, @as(usize, p_count));
        errdefer alloc.free(params);
        let pi: u32 = 0;
        while (pi < p_count) {
            const pb = try readByte(payload, &off);
            params[@as(usize, pi)] = try ValType.fromByte(pb);
            pi += 1;
        }

        const r_count = try readUleb128(payload, &off);
        try checkVectorCount(r_count, payload.len - off);
        let results = try alloc.alloc(ValType, @as(usize, r_count));
        errdefer alloc.free(results);
        let ri: u32 = 0;
        while (ri < r_count) {
            const rb = try readByte(payload, &off);
            results[@as(usize, ri)] = try ValType.fromByte(rb);
            ri += 1;
        }

        try list.append(alloc, FuncType{ .params = params, .results = results });
        i += 1;
    }

    if (off != payload.len) {
        return WasmError.SectionLengthMismatch;
    }
    return try list.toOwnedSlice(alloc);
}

fn parseImportSection(alloc: std.mem.Allocator, payload: []const u8): ([]WasmImport) ! WasmError {
    let off: usize = 0;
    const count = try readUleb128(payload, &off);
    try checkVectorCount(count, payload.len - off);
    let list: std.ArrayList(WasmImport) = .empty;
    errdefer {
        let j: usize = 0;
        while (j < list.items.len) {
            alloc.free(list.items[j].module);
            alloc.free(list.items[j].name);
            j += 1;
        }
        list.deinit(alloc);
    }

    let i: u32 = 0;
    while (i < count) {
        const mod_name = try readName(alloc, payload, &off);
        errdefer alloc.free(mod_name);
        const item_name = try readName(alloc, payload, &off);
        errdefer alloc.free(item_name);

        const kind_b = try readByte(payload, &off);
        const kind = try ExternalKind.fromByte(kind_b);

        let imp = WasmImport{
            .module = mod_name,
            .name = item_name,
            .kind = kind,
            .type_index = 0,
            .memory_limits = null,
        };

        if (kind == .Func) {
            imp.type_index = try readUleb128(payload, &off);
        } else if (kind == .Memory) {
            imp.memory_limits = try readLimits(payload, &off);
        } else if (kind == .Table) {
            * = try readByte(payload, &off);
            * = try readLimits(payload, &off);
        } else if (kind == .Global) {
            * = try ValType.fromByte(try readByte(payload, &off));
            const mut_b = try readByte(payload, &off);
            if (mut_b > 1) {
                return WasmError.InvalidMutFlag;
            }
        }

        try list.append(alloc, imp);
        i += 1;
    }

    if (off != payload.len) {
        return WasmError.SectionLengthMismatch;
    }
    return try list.toOwnedSlice(alloc);
}

fn parseFunctionSection(alloc: std.mem.Allocator, payload: []const u8): ([]u32) ! WasmError {
    let off: usize = 0;
    const count = try readUleb128(payload, &off);
    try checkVectorCount(count, payload.len - off);
    let funcs = try alloc.alloc(u32, @as(usize, count));
    errdefer alloc.free(funcs);

    let i: u32 = 0;
    while (i < count) {
        funcs[@as(usize, i)] = try readUleb128(payload, &off);
        i += 1;
    }

    if (off != payload.len) {
        return WasmError.SectionLengthMismatch;
    }
    return funcs;
}

fn parseMemorySection(alloc: std.mem.Allocator, payload: []const u8): ([]WasmMemory) ! WasmError {
    let off: usize = 0;
    const count = try readUleb128(payload, &off);
    try checkVectorCount(count, payload.len - off);
    let mems = try alloc.alloc(WasmMemory, @as(usize, count));
    errdefer alloc.free(mems);

    let i: u32 = 0;
    while (i < count) {
        const lim = try readLimits(payload, &off);
        mems[@as(usize, i)] = WasmMemory{ .limits = lim };
        i += 1;
    }

    if (off != payload.len) {
        return WasmError.SectionLengthMismatch;
    }
    return mems;
}

fn parseGlobalSection(alloc: std.mem.Allocator, payload: []const u8): ([]WasmGlobal) ! WasmError {
    let off: usize = 0;
    const count = try readUleb128(payload, &off);
    try checkVectorCount(count, payload.len - off);
    let list: std.ArrayList(WasmGlobal) = .empty;
    errdefer {
        let j: usize = 0;
        while (j < list.items.len) {
            alloc.free(list.items[j].init_expr);
            j += 1;
        }
        list.deinit(alloc);
    }

    let i: u32 = 0;
    while (i < count) {
        const vt = try ValType.fromByte(try readByte(payload, &off));
        const mut_b = try readByte(payload, &off);
        if (mut_b > 1) {
            return WasmError.InvalidMutFlag;
        }
        const expr_start = off;
        while (true) {
            const op = try readByte(payload, &off);
            if (op == 0x0B) {
                break;
            }
        }
        const expr_dup = try alloc.dupe(u8, payload[expr_start..off]);
        errdefer alloc.free(expr_dup);

        try list.append(alloc, WasmGlobal{
            .val_type = vt,
            .mutable = mut_b == 1,
            .init_expr = expr_dup,
        });
        i += 1;
    }

    if (off != payload.len) {
        return WasmError.SectionLengthMismatch;
    }
    return try list.toOwnedSlice(alloc);
}

fn parseExportSection(alloc: std.mem.Allocator, payload: []const u8): ([]WasmExport) ! WasmError {
    let off: usize = 0;
    const count = try readUleb128(payload, &off);
    try checkVectorCount(count, payload.len - off);
    let list: std.ArrayList(WasmExport) = .empty;
    errdefer {
        let j: usize = 0;
        while (j < list.items.len) {
            alloc.free(list.items[j].name);
            j += 1;
        }
        list.deinit(alloc);
    }

    let i: u32 = 0;
    while (i < count) {
        const exp_name = try readName(alloc, payload, &off);
        errdefer alloc.free(exp_name);
        const kind = try ExternalKind.fromByte(try readByte(payload, &off));
        const idx = try readUleb128(payload, &off);

        try list.append(alloc, WasmExport{
            .name = exp_name,
            .kind = kind,
            .index = idx,
        });
        i += 1;
    }

    if (off != payload.len) {
        return WasmError.SectionLengthMismatch;
    }
    return try list.toOwnedSlice(alloc);
}

fn parseStartSection(payload: []const u8): u32 ! WasmError {
    let off: usize = 0;
    const idx = try readUleb128(payload, &off);
    if (off != payload.len) {
        return WasmError.SectionLengthMismatch;
    }
    return idx;
}

fn parseCodeSection(alloc: std.mem.Allocator, payload: []const u8): ([]const []const u8) ! WasmError {
    let off: usize = 0;
    const count = try readUleb128(payload, &off);
    try checkVectorCount(count, payload.len - off);
    const empty_code: []const u8 = "";
    let codes = try alloc.alloc(@TypeOf(empty_code), @as(usize, count));
    let parsed_count: usize = 0;
    errdefer {
        let j: usize = 0;
        while (j < parsed_count) {
            alloc.free(codes[j]);
            j += 1;
        }
        alloc.free(codes);
    }

    let i: u32 = 0;
    while (i < count) {
        const body_len = try readUleb128(payload, &off);
        const body_raw = try readBytesSlice(payload, &off, @as(usize, body_len));
        let body_dup = try alloc.alloc(u8, body_raw.len);
        const copied = simdWasmCopyPayload(body_dup, body_raw);
        if (copied != body_raw.len) return WasmError.UnexpectedEof;
        codes[@as(usize, i)] = body_dup;
        parsed_count += 1;
        i += 1;
    }

    if (off != payload.len) {
        return WasmError.SectionLengthMismatch;
    }
    return codes;
}

fn parseCustomSection(alloc: std.mem.Allocator, payload: []const u8): CustomSection ! WasmError {
    let off: usize = 0;
    const sec_name = try readName(alloc, payload, &off);
    errdefer alloc.free(sec_name);
    const tail = payload[off..];
    let sec_data = try alloc.alloc(u8, tail.len);
    const copied = simdWasmCopyPayload(sec_data, tail);
    if (copied != tail.len) return WasmError.UnexpectedEof;
    return CustomSection{
        .name = sec_name,
        .data = sec_data,
    };
}

pub fn parseWasm(alloc: std.mem.Allocator, bytes: []const u8): WasmModule ! WasmError {
    if (bytes.len < 8) {
        return WasmError.UnexpectedEof;
    }
    if (!std.mem.eql(u8, bytes[0..4], &WASM_MAGIC)) {
        return WasmError.InvalidMagic;
    }
    if (!std.mem.eql(u8, bytes[4..8], &WASM_VERSION)) {
        return WasmError.UnsupportedVersion;
    }

    let mod = WasmModule{};
    errdefer mod.deinit(alloc);

    let customs: std.ArrayList(CustomSection) = .empty;
    errdefer {
        let j: usize = 0;
        while (j < customs.items.len) {
            alloc.free(customs.items[j].name);
            alloc.free(customs.items[j].data);
            j += 1;
        }
        customs.deinit(alloc);
    }

    let seen_sections: [13]bool = .{ false, false, false, false, false, false, false, false, false, false, false, false, false };
    let last_std_id: u8 = 0;
    let off: usize = 8;

    while (off < bytes.len) {
        const id_byte = try readByte(bytes, &off);
        const sec_id = try SectionId.fromByte(id_byte);
        const payload_len = try readUleb128(bytes, &off);
        const payload = try readBytesSlice(bytes, &off, @as(usize, payload_len));

        if (sec_id != .Custom) {
            if (seen_sections[@as(usize, id_byte)]) {
                return WasmError.DuplicateSection;
            }
            if (id_byte <= last_std_id) {
                return WasmError.OutOfOrderSection;
            }
            seen_sections[@as(usize, id_byte)] = true;
            last_std_id = id_byte;
        }

        switch (sec_id) {
            .Custom => {
                const cs = try parseCustomSection(alloc, payload);
                try customs.append(alloc, cs);
            },
            .Type => {
                mod.types = try parseTypeSection(alloc, payload);
            },
            .Import => {
                mod.imports = try parseImportSection(alloc, payload);
            },
            .Function => {
                mod.functions = try parseFunctionSection(alloc, payload);
            },
            .Memory => {
                mod.memories = try parseMemorySection(alloc, payload);
            },
            .Global => {
                mod.globals = try parseGlobalSection(alloc, payload);
            },
            .Export => {
                mod.exports = try parseExportSection(alloc, payload);
            },
            .Start => {
                mod.start = try parseStartSection(payload);
            },
            .Code => {
                mod.codes = try parseCodeSection(alloc, payload);
            },
            .DataCount => {
                let dc_off: usize = 0;
                * = try readUleb128(payload, &dc_off);
                if (dc_off != payload.len) {
                    return WasmError.SectionLengthMismatch;
                }
            },
            .Table, .Element, .Data => {},
        }
    }

    mod.custom_sections = try customs.toOwnedSlice(alloc);
    return mod;
}

/// Zero-allocation structural WebAssembly 2.0 binary module validator (aligned with `wasmparser`).
/// Enforces magic/version, strictly ascending non-duplicate standard section IDs (1..=12),
/// overflow-safe section lengths, UTF-8 names, exact section consumption, and vector item caps.
/// Returns the total number of validated sections on success.
pub fn validateWasmModule(bytes: []const u8): u32 ! WasmError {
    if (bytes.len < 8) {
        return WasmError.UnexpectedEof;
    }
    if (!std.mem.eql(u8, bytes[0..4], &WASM_MAGIC)) {
        return WasmError.InvalidMagic;
    }
    if (!std.mem.eql(u8, bytes[4..8], &WASM_VERSION)) {
        return WasmError.UnsupportedVersion;
    }

    let seen_sections: [13]bool = .{ false, false, false, false, false, false, false, false, false, false, false, false, false };
    let last_std_id: u8 = 0;
    let off: usize = 8;
    let sec_count: u32 = 0;

    while (off < bytes.len) {
        const id_byte = try readByte(bytes, &off);
        const sec_id = try SectionId.fromByte(id_byte);
        const payload_len = try readUleb128(bytes, &off);
        const payload = try readBytesSlice(bytes, &off, @as(usize, payload_len));

        if (sec_id != .Custom) {
            if (seen_sections[@as(usize, id_byte)]) {
                return WasmError.DuplicateSection;
            }
            if (id_byte <= last_std_id) {
                return WasmError.OutOfOrderSection;
            }
            seen_sections[@as(usize, id_byte)] = true;
            last_std_id = id_byte;
        }

        let p: usize = 0;
        switch (sec_id) {
            .Custom => {
                * = try readNameSlice(payload, &p);
            },
            .Type => {
                const cnt = try readUleb128(payload, &p);
                try checkVectorCount(cnt, payload.len - p);
                let i: u32 = 0;
                while (i < cnt) : (i += 1) {
                    const tag = try readByte(payload, &p);
                    if (tag != 0x60) return WasmError.InvalidFuncTypeTag;
                    const pc = try readUleb128(payload, &p);
                    try checkVectorCount(pc, payload.len - p);
                    let pi: u32 = 0;
                    while (pi < pc) : (pi += 1) {
                        * = try ValType.fromByte(try readByte(payload, &p));
                    }
                    const rc = try readUleb128(payload, &p);
                    try checkVectorCount(rc, payload.len - p);
                    let ri: u32 = 0;
                    while (ri < rc) : (ri += 1) {
                        * = try ValType.fromByte(try readByte(payload, &p));
                    }
                }
                if (p != payload.len) return WasmError.SectionLengthMismatch;
            },
            .Import => {
                const cnt = try readUleb128(payload, &p);
                try checkVectorCount(cnt, payload.len - p);
                let i: u32 = 0;
                while (i < cnt) : (i += 1) {
                    * = try readNameSlice(payload, &p);
                    * = try readNameSlice(payload, &p);
                    const kind = try ExternalKind.fromByte(try readByte(payload, &p));
                    if (kind == .Func) {
                        * = try readUleb128(payload, &p);
                    } else if (kind == .Memory) {
                        * = try readLimits(payload, &p);
                    } else if (kind == .Table) {
                        * = try readByte(payload, &p);
                        * = try readLimits(payload, &p);
                    } else if (kind == .Global) {
                        * = try ValType.fromByte(try readByte(payload, &p));
                        if ((try readByte(payload, &p)) > 1) return WasmError.InvalidMutFlag;
                    }
                }
                if (p != payload.len) return WasmError.SectionLengthMismatch;
            },
            .Function => {
                const cnt = try readUleb128(payload, &p);
                try checkVectorCount(cnt, payload.len - p);
                let i: u32 = 0;
                while (i < cnt) : (i += 1) {
                    * = try readUleb128(payload, &p);
                }
                if (p != payload.len) return WasmError.SectionLengthMismatch;
            },
            .Memory => {
                const cnt = try readUleb128(payload, &p);
                try checkVectorCount(cnt, payload.len - p);
                let i: u32 = 0;
                while (i < cnt) : (i += 1) {
                    * = try readLimits(payload, &p);
                }
                if (p != payload.len) return WasmError.SectionLengthMismatch;
            },
            .Global => {
                const cnt = try readUleb128(payload, &p);
                try checkVectorCount(cnt, payload.len - p);
                let i: u32 = 0;
                while (i < cnt) : (i += 1) {
                    * = try ValType.fromByte(try readByte(payload, &p));
                    if ((try readByte(payload, &p)) > 1) return WasmError.InvalidMutFlag;
                    while (true) {
                        if ((try readByte(payload, &p)) == 0x0B) break;
                    }
                }
                if (p != payload.len) return WasmError.SectionLengthMismatch;
            },
            .Export => {
                const cnt = try readUleb128(payload, &p);
                try checkVectorCount(cnt, payload.len - p);
                let i: u32 = 0;
                while (i < cnt) : (i += 1) {
                    * = try readNameSlice(payload, &p);
                    * = try ExternalKind.fromByte(try readByte(payload, &p));
                    * = try readUleb128(payload, &p);
                }
                if (p != payload.len) return WasmError.SectionLengthMismatch;
            },
            .Start, .DataCount => {
                * = try readUleb128(payload, &p);
                if (p != payload.len) return WasmError.SectionLengthMismatch;
            },
            .Code => {
                const cnt = try readUleb128(payload, &p);
                try checkVectorCount(cnt, payload.len - p);
                let i: u32 = 0;
                while (i < cnt) : (i += 1) {
                    const blen = try readUleb128(payload, &p);
                    * = try readBytesSlice(payload, &p, @as(usize, blen));
                }
                if (p != payload.len) return WasmError.SectionLengthMismatch;
            },
            .Table, .Element, .Data => {
                const cnt = try readUleb128(payload, &p);
                try checkVectorCount(cnt, payload.len - p);
            },
        }
        sec_count += 1;
    }

    return sec_count;
}

fn appendSection(out: *std.ArrayList(u8), alloc: std.mem.Allocator, sec_id: SectionId, payload: []const u8): ! {
    try out.append(alloc, sec_id.toByte());
    try writeUleb128(out, alloc, @as(u32, @intCast(payload.len)));
    try out.appendSlice(alloc, payload);
}

fn appendName(out: *std.ArrayList(u8), alloc: std.mem.Allocator, name: []const u8): ! {
    try writeUleb128(out, alloc, @as(u32, @intCast(name.len)));
    try out.appendSlice(alloc, name);
}

fn appendLimits(out: *std.ArrayList(u8), alloc: std.mem.Allocator, lim: WasmLimits): ! {
    if (m := lim.max) {
        try out.append(alloc, 0x01);
        try writeUleb128(out, alloc, lim.min);
        try writeUleb128(out, alloc, m);
    } else {
        try out.append(alloc, 0x00);
        try writeUleb128(out, alloc, lim.min);
    }
}

pub fn encodeWasm(alloc: std.mem.Allocator, mod: WasmModule): ([]u8) ! {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);

    try out.appendSlice(alloc, &WASM_MAGIC);
    try out.appendSlice(alloc, &WASM_VERSION);

    let sec: std.ArrayList(u8) = .empty;
    defer sec.deinit(alloc);

    if (mod.types.len > 0) {
        sec.clearRetainingCapacity();
        try writeUleb128(&sec, alloc, @as(u32, @intCast(mod.types.len)));
        let i: usize = 0;
        while (i < mod.types.len) {
            const ft = mod.types[i];
            try sec.append(alloc, 0x60);
            try writeUleb128(&sec, alloc, @as(u32, @intCast(ft.params.len)));
            let pi: usize = 0;
            while (pi < ft.params.len) {
                try sec.append(alloc, ft.params[pi].toByte());
                pi += 1;
            }
            try writeUleb128(&sec, alloc, @as(u32, @intCast(ft.results.len)));
            let ri: usize = 0;
            while (ri < ft.results.len) {
                try sec.append(alloc, ft.results[ri].toByte());
                ri += 1;
            }
            i += 1;
        }
        try appendSection(&out, alloc, .Type, sec.items);
    }

    if (mod.imports.len > 0) {
        sec.clearRetainingCapacity();
        try writeUleb128(&sec, alloc, @as(u32, @intCast(mod.imports.len)));
        let i: usize = 0;
        while (i < mod.imports.len) {
            const imp = mod.imports[i];
            try appendName(&sec, alloc, imp.module);
            try appendName(&sec, alloc, imp.name);
            try sec.append(alloc, imp.kind.toByte());
            if (imp.kind == .Func) {
                try writeUleb128(&sec, alloc, imp.type_index);
            } else if (imp.kind == .Memory) {
                const lim = imp.memory_limits orelse WasmLimits{};
                try appendLimits(&sec, alloc, lim);
            }
            i += 1;
        }
        try appendSection(&out, alloc, .Import, sec.items);
    }

    if (mod.functions.len > 0) {
        sec.clearRetainingCapacity();
        try writeUleb128(&sec, alloc, @as(u32, @intCast(mod.functions.len)));
        let i: usize = 0;
        while (i < mod.functions.len) {
            try writeUleb128(&sec, alloc, mod.functions[i]);
            i += 1;
        }
        try appendSection(&out, alloc, .Function, sec.items);
    }

    if (mod.memories.len > 0) {
        sec.clearRetainingCapacity();
        try writeUleb128(&sec, alloc, @as(u32, @intCast(mod.memories.len)));
        let i: usize = 0;
        while (i < mod.memories.len) {
            try appendLimits(&sec, alloc, mod.memories[i].limits);
            i += 1;
        }
        try appendSection(&out, alloc, .Memory, sec.items);
    }

    if (mod.globals.len > 0) {
        sec.clearRetainingCapacity();
        try writeUleb128(&sec, alloc, @as(u32, @intCast(mod.globals.len)));
        let i: usize = 0;
        while (i < mod.globals.len) {
            const g = mod.globals[i];
            try sec.append(alloc, g.val_type.toByte());
            try sec.append(alloc, if (g.mutable) @as(u8, 1) else @as(u8, 0));
            try sec.appendSlice(alloc, g.init_expr);
            i += 1;
        }
        try appendSection(&out, alloc, .Global, sec.items);
    }

    if (mod.exports.len > 0) {
        sec.clearRetainingCapacity();
        try writeUleb128(&sec, alloc, @as(u32, @intCast(mod.exports.len)));
        let i: usize = 0;
        while (i < mod.exports.len) {
            const exp = mod.exports[i];
            try appendName(&sec, alloc, exp.name);
            try sec.append(alloc, exp.kind.toByte());
            try writeUleb128(&sec, alloc, exp.index);
            i += 1;
        }
        try appendSection(&out, alloc, .Export, sec.items);
    }

    if (start_idx := mod.start) {
        sec.clearRetainingCapacity();
        try writeUleb128(&sec, alloc, start_idx);
        try appendSection(&out, alloc, .Start, sec.items);
    }

    if (mod.codes.len > 0) {
        sec.clearRetainingCapacity();
        try writeUleb128(&sec, alloc, @as(u32, @intCast(mod.codes.len)));
        let i: usize = 0;
        while (i < mod.codes.len) {
            const body = mod.codes[i];
            try writeUleb128(&sec, alloc, @as(u32, @intCast(body.len)));
            try sec.appendSlice(alloc, body);
            i += 1;
        }
        try appendSection(&out, alloc, .Code, sec.items);
    }

    let ci: usize = 0;
    while (ci < mod.custom_sections.len) {
        const cs = mod.custom_sections[ci];
        sec.clearRetainingCapacity();
        try appendName(&sec, alloc, cs.name);
        try sec.appendSlice(alloc, cs.data);
        try appendSection(&out, alloc, .Custom, sec.items);
        ci += 1;
    }

    return try out.toOwnedSlice(alloc);
}

pub fn findExport(mod: WasmModule, name: []const u8): WasmExport? {
    let i: usize = 0;
    while (i < mod.exports.len) {
        if (std.mem.eql(u8, mod.exports[i].name, name)) {
            return mod.exports[i];
        }
        i += 1;
    }
    return null;
}

pub fn findExportFuncType(mod: WasmModule, name: []const u8): FuncType? {
    const exp = findExport(mod, name) orelse return null;
    if (exp.kind != .Func) {
        return null;
    }

    let imported_funcs: u32 = 0;
    let i: usize = 0;
    while (i < mod.imports.len) {
        if (mod.imports[i].kind == .Func) {
            if (imported_funcs == exp.index) {
                const tid = @as(usize, mod.imports[i].type_index);
                if (tid < mod.types.len) {
                    return mod.types[tid];
                }
                return null;
            }
            imported_funcs += 1;
        }
        i += 1;
    }

    const local_idx = exp.index - imported_funcs;
    if (@as(usize, local_idx) < mod.functions.len) {
        const tid = @as(usize, mod.functions[@as(usize, local_idx)]);
        if (tid < mod.types.len) {
            return mod.types[tid];
        }
    }
    return null;
}

pub fn findCustomSection(mod: WasmModule, name: []const u8): CustomSection? {
    let i: usize = 0;
    while (i < mod.custom_sections.len) {
        if (std.mem.eql(u8, mod.custom_sections[i].name, name)) {
            return mod.custom_sections[i];
        }
        i += 1;
    }
    return null;
}

pub fn addCustomSection(alloc: std.mem.Allocator, wasm_bytes: []const u8, name: []const u8, data: []const u8): ([]u8) ! {
    if (wasm_bytes.len < 8) {
        return WasmError.UnexpectedEof;
    }
    if (!std.mem.eql(u8, wasm_bytes[0..4], &WASM_MAGIC)) {
        return WasmError.InvalidMagic;
    }
    if (!std.mem.eql(u8, wasm_bytes[4..8], &WASM_VERSION)) {
        return WasmError.UnsupportedVersion;
    }

    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(alloc);

    try out.appendSlice(alloc, wasm_bytes);

    let sec: std.ArrayList(u8) = .empty;
    defer sec.deinit(alloc);

    try appendName(&sec, alloc, name);
    try sec.appendSlice(alloc, data);
    try appendSection(&out, alloc, .Custom, sec.items);

    return try out.toOwnedSlice(alloc);
}
