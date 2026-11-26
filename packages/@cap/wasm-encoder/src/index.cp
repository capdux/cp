// @cap/wasm-encoder — Zero-allocation WebAssembly 1.0 binary module encoder
// Clean-room parity with crates.io:wasm-encoder (Apache-2.0 WITH LLVM-exception)

pub enum ValType: u8 {
    I32 = 0x7F,
    I64 = 0x7E,
    F32 = 0x7D,
    F64 = 0x7C,
}

pub enum ExportKind: u8 {
    Func = 0,
    Table = 1,
    Mem = 2,
    Global = 3,
}

pub const SEC_CUSTOM: u8 = 0;
pub const SEC_TYPE: u8 = 1;
pub const SEC_FUNCTION: u8 = 3;
pub const SEC_MEMORY: u8 = 5;
pub const SEC_EXPORT: u8 = 7;
pub const SEC_CODE: u8 = 10;

pub const OP_NOP: u8 = 0x01;
pub const OP_END: u8 = 0x0B;
pub const OP_RETURN: u8 = 0x0F;
pub const OP_CALL: u8 = 0x10;
pub const OP_LOCAL_GET: u8 = 0x20;
pub const OP_LOCAL_SET: u8 = 0x21;
pub const OP_I32_CONST: u8 = 0x41;
pub const OP_I64_CONST: u8 = 0x42;
pub const OP_I32_ADD: u8 = 0x6A;
pub const OP_I32_SUB: u8 = 0x6B;
pub const OP_I32_MUL: u8 = 0x6C;

fn wencValTypeByte(vt: ValType): u8 {
    switch (vt) {
        ValType.I32 => return 0x7F,
        ValType.I64 => return 0x7E,
        ValType.F32 => return 0x7D,
        ValType.F64 => return 0x7C,
    }
}

fn wencExportKindByte(kind: ExportKind): u8 {
    switch (kind) {
        ExportKind.Func => return 0,
        ExportKind.Table => return 1,
        ExportKind.Mem => return 2,
        ExportKind.Global => return 3,
    }
}

fn wencLow7Signed(v: i64): u8 {
    const bits = @as(u64, @bitCast(v));
    return u8(bits & 0x7F);
}

pub fn encodeUleb128(val: u64, out: []u8): usize {
    let v: u64 = val;
    let pos: usize = 0;
    while (true) {
        if (pos >= out.len) {
            return 0;
        }
        let b: u8 = u8(v & 0x7F);
        v = v >> 7;
        if (v != 0) {
            b = b | 0x80;
        }
        out[pos] = b;
        pos += 1;
        if (v == 0) {
            break;
        }
    }
    return pos;
}

pub fn encodeSleb128(val: i64, out: []u8): usize {
    let v: i64 = val;
    let pos: usize = 0;
    while (true) {
        if (pos >= out.len) {
            return 0;
        }
        let b: u8 = wencLow7Signed(v);
        v = v >> 7;
        const sign_set: bool = (b & 0x40) != 0;
        const done: bool = (v == 0 && !sign_set) || (v == -1 && sign_set);
        if (!done) {
            b = b | 0x80;
        }
        out[pos] = b;
        pos += 1;
        if (done) {
            break;
        }
    }
    return pos;
}

pub struct FuncType {
    params: [8]ValType,
    param_count: usize,
    results: [4]ValType,
    result_count: usize,

    static fn create(params: []const ValType, results: []const ValType): FuncType {
        let p: [8]ValType = undefined;
        let r: [4]ValType = undefined;
        let i: usize = 0;
        while (i < 8) : (i += 1) {
            p[i] = ValType.I32;
        }
        let j: usize = 0;
        while (j < 4) : (j += 1) {
            r[j] = ValType.I32;
        }
        const pc: usize = if (params.len > 8) 8 else params.len;
        let pi: usize = 0;
        while (pi < pc) : (pi += 1) {
            p[pi] = params[pi];
        }
        const rc: usize = if (results.len > 4) 4 else results.len;
        let ri: usize = 0;
        while (ri < rc) : (ri += 1) {
            r[ri] = results[ri];
        }
        return FuncType{
            params: p,
            param_count: pc,
            results: r,
            result_count: rc,
        };
    }
}

pub struct MemType {
    min_pages: u32,
    has_max: bool,
    max_pages: u32,

    static fn unbounded(min_pages: u32): MemType {
        return MemType{
            min_pages: min_pages,
            has_max: false,
            max_pages: 0,
        };
    }

    static fn bounded(min_pages: u32, max_pages: u32): MemType {
        return MemType{
            min_pages: min_pages,
            has_max: true,
            max_pages: max_pages,
        };
    }
}

pub struct ExportEntry {
    name: string,
    kind: ExportKind,
    index: u32,
}

pub struct FuncBody {
    local_counts: [8]u32,
    local_types: [8]ValType,
    local_group_count: usize,
    code: []const u8,

    static fn simple(code: []const u8): FuncBody {
        let lc: [8]u32 = undefined;
        let lt: [8]ValType = undefined;
        let i: usize = 0;
        while (i < 8) : (i += 1) {
            lc[i] = 0;
            lt[i] = ValType.I32;
        }
        return FuncBody{
            local_counts: lc,
            local_types: lt,
            local_group_count: 0,
            code: code,
        };
    }

    fn addLocals(*_, count: u32, vt: ValType): bool {
        if (_.local_group_count >= 8) {
            return false;
        }
        _.local_counts[_.local_group_count] = count;
        _.local_types[_.local_group_count] = vt;
        _.local_group_count += 1;
        return true;
    }
}

fn wencAppendByte(dst: []u8, pos: usize, b: u8): usize? {
    if (pos >= dst.len) {
        return null;
    }
    dst[pos] = b;
    return pos + 1;
}

fn wencAppendBytes(dst: []u8, pos: usize, src: []const u8): usize? {
    if (pos + src.len > dst.len) {
        return null;
    }
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[pos + i] = src[i];
    }
    return pos + src.len;
}

fn wencAppendUleb(dst: []u8, pos: usize, val: u64): usize? {
    if (pos >= dst.len) {
        return null;
    }
    const n = encodeUleb128(val, dst[pos..dst.len]);
    if (n == 0) {
        return null;
    }
    return pos + n;
}

pub struct WasmModuleBuilder {
    buf: [4096]u8,
    len: usize,
    overflow: bool,

    static fn init(): WasmModuleBuilder {
        let b: [4096]u8 = undefined;
        b[0] = 0x00;
        b[1] = 0x61; // 'a'
        b[2] = 0x73; // 's'
        b[3] = 0x6D; // 'm'
        b[4] = 0x01; // version 1
        b[5] = 0x00;
        b[6] = 0x00;
        b[7] = 0x00;
        return WasmModuleBuilder{
            buf: b,
            len: 8,
            overflow: false,
        };
    }

    fn writeSection(*_, section_id: u8, payload: []const u8): bool {
        if (_.overflow) {
            return false;
        }
        let leb: [10]u8 = undefined;
        const leb_len = encodeUleb128(u64(payload.len), leb[0..10]);
        if (leb_len == 0) {
            _.overflow = true;
            return false;
        }
        const needed: usize = usize(1) + leb_len + payload.len;
        if (_.len + needed > 4096) {
            _.overflow = true;
            return false;
        }
        _.buf[_.len] = section_id;
        _.len += 1;
        let i: usize = 0;
        while (i < leb_len) : (i += 1) {
            _.buf[_.len] = leb[i];
            _.len += 1;
        }
        let j: usize = 0;
        while (j < payload.len) : (j += 1) {
            _.buf[_.len] = payload[j];
            _.len += 1;
        }
        return true;
    }

    fn addTypeSection(*_, types: []const FuncType): bool {
        let sec: [1024]u8 = undefined;
        let p: usize = 0;
        if (next_p := wencAppendUleb(sec[0..1024], p, u64(types.len))) {
            p = next_p;
        } else {
            _.overflow = true;
            return false;
        }
        let i: usize = 0;
        while (i < types.len) : (i += 1) {
            const ft = types[i];
            if (p0 := wencAppendByte(sec[0..1024], p, 0x60)) {
                p = p0;
            } else {
                _.overflow = true;
                return false;
            }
            if (p1 := wencAppendUleb(sec[0..1024], p, u64(ft.param_count))) {
                p = p1;
            } else {
                _.overflow = true;
                return false;
            }
            let pi: usize = 0;
            while (pi < ft.param_count) : (pi += 1) {
                if (pp := wencAppendByte(sec[0..1024], p, wencValTypeByte(ft.params[pi]))) {
                    p = pp;
                } else {
                    _.overflow = true;
                    return false;
                }
            }
            if (p2 := wencAppendUleb(sec[0..1024], p, u64(ft.result_count))) {
                p = p2;
            } else {
                _.overflow = true;
                return false;
            }
            let ri: usize = 0;
            while (ri < ft.result_count) : (ri += 1) {
                if (pr := wencAppendByte(sec[0..1024], p, wencValTypeByte(ft.results[ri]))) {
                    p = pr;
                } else {
                    _.overflow = true;
                    return false;
                }
            }
        }
        return _.writeSection(SEC_TYPE, sec[0..p]);
    }

    fn addFunctionSection(*_, type_indices: []const u32): bool {
        let sec: [1024]u8 = undefined;
        let p: usize = 0;
        if (next_p := wencAppendUleb(sec[0..1024], p, u64(type_indices.len))) {
            p = next_p;
        } else {
            _.overflow = true;
            return false;
        }
        let i: usize = 0;
        while (i < type_indices.len) : (i += 1) {
            if (np := wencAppendUleb(sec[0..1024], p, u64(type_indices[i]))) {
                p = np;
            } else {
                _.overflow = true;
                return false;
            }
        }
        return _.writeSection(SEC_FUNCTION, sec[0..p]);
    }

    fn addMemorySection(*_, memories: []const MemType): bool {
        let sec: [512]u8 = undefined;
        let p: usize = 0;
        if (next_p := wencAppendUleb(sec[0..512], p, u64(memories.len))) {
            p = next_p;
        } else {
            _.overflow = true;
            return false;
        }
        let i: usize = 0;
        while (i < memories.len) : (i += 1) {
            const mem = memories[i];
            const flags: u8 = if (mem.has_max) 0x01 else 0x00;
            if (p0 := wencAppendByte(sec[0..512], p, flags)) {
                p = p0;
            } else {
                _.overflow = true;
                return false;
            }
            if (p1 := wencAppendUleb(sec[0..512], p, u64(mem.min_pages))) {
                p = p1;
            } else {
                _.overflow = true;
                return false;
            }
            if (mem.has_max) {
                if (p2 := wencAppendUleb(sec[0..512], p, u64(mem.max_pages))) {
                    p = p2;
                } else {
                    _.overflow = true;
                    return false;
                }
            }
        }
        return _.writeSection(SEC_MEMORY, sec[0..p]);
    }

    fn addExportSection(*_, exports: []const ExportEntry): bool {
        let sec: [1024]u8 = undefined;
        let p: usize = 0;
        if (next_p := wencAppendUleb(sec[0..1024], p, u64(exports.len))) {
            p = next_p;
        } else {
            _.overflow = true;
            return false;
        }
        let i: usize = 0;
        while (i < exports.len) : (i += 1) {
            const ex = exports[i];
            if (p0 := wencAppendUleb(sec[0..1024], p, u64(ex.name.len))) {
                p = p0;
            } else {
                _.overflow = true;
                return false;
            }
            if (p1 := wencAppendBytes(sec[0..1024], p, ex.name)) {
                p = p1;
            } else {
                _.overflow = true;
                return false;
            }
            if (p2 := wencAppendByte(sec[0..1024], p, wencExportKindByte(ex.kind))) {
                p = p2;
            } else {
                _.overflow = true;
                return false;
            }
            if (p3 := wencAppendUleb(sec[0..1024], p, u64(ex.index))) {
                p = p3;
            } else {
                _.overflow = true;
                return false;
            }
        }
        return _.writeSection(SEC_EXPORT, sec[0..p]);
    }

    fn addCodeSection(*_, bodies: []const FuncBody): bool {
        let sec: [2048]u8 = undefined;
        let p: usize = 0;
        if (next_p := wencAppendUleb(sec[0..2048], p, u64(bodies.len))) {
            p = next_p;
        } else {
            _.overflow = true;
            return false;
        }
        let i: usize = 0;
        while (i < bodies.len) : (i += 1) {
            const body = bodies[i];
            let fbuf: [1024]u8 = undefined;
            let fp: usize = 0;
            if (fp0 := wencAppendUleb(fbuf[0..1024], fp, u64(body.local_group_count))) {
                fp = fp0;
            } else {
                _.overflow = true;
                return false;
            }
            let li: usize = 0;
            while (li < body.local_group_count) : (li += 1) {
                if (fp1 := wencAppendUleb(fbuf[0..1024], fp, u64(body.local_counts[li]))) {
                    fp = fp1;
                } else {
                    _.overflow = true;
                    return false;
                }
                if (fp2 := wencAppendByte(fbuf[0..1024], fp, wencValTypeByte(body.local_types[li]))) {
                    fp = fp2;
                } else {
                    _.overflow = true;
                    return false;
                }
            }
            if (fp3 := wencAppendBytes(fbuf[0..1024], fp, body.code)) {
                fp = fp3;
            } else {
                _.overflow = true;
                return false;
            }
            if (p0 := wencAppendUleb(sec[0..2048], p, u64(fp))) {
                p = p0;
            } else {
                _.overflow = true;
                return false;
            }
            if (p1 := wencAppendBytes(sec[0..2048], p, fbuf[0..fp])) {
                p = p1;
            } else {
                _.overflow = true;
                return false;
            }
        }
        return _.writeSection(SEC_CODE, sec[0..p]);
    }

    fn addCustomSection(*_, name: string, data: []const u8): bool {
        let sec: [1024]u8 = undefined;
        let p: usize = 0;
        if (p0 := wencAppendUleb(sec[0..1024], p, u64(name.len))) {
            p = p0;
        } else {
            _.overflow = true;
            return false;
        }
        if (p1 := wencAppendBytes(sec[0..1024], p, name)) {
            p = p1;
        } else {
            _.overflow = true;
            return false;
        }
        if (p2 := wencAppendBytes(sec[0..1024], p, data)) {
            p = p2;
        } else {
            _.overflow = true;
            return false;
        }
        return _.writeSection(SEC_CUSTOM, sec[0..p]);
    }

    fn byteLen(): usize {
        return _.len;
    }

    fn hasOverflow(): bool {
        return _.overflow;
    }

    fn finish(out: []u8): usize {
        if (_.overflow || out.len < _.len) {
            return 0;
        }
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            out[i] = _.buf[i];
        }
        return _.len;
    }
}
