// Type definitions for CP

export enum ValType {
    I32 = 127,
    I64 = 126,
    F32 = 125,
    F64 = 124
}

export enum ExportKind {
    Func = 0,
    Table = 1,
    Mem = 2,
    Global = 3
}

export fn wencValTypeByte(vt: ValType): u8;
export fn wencExportKindByte(kind: ExportKind): u8;
export fn wencLow7Signed(v: i64): u8;
export fn encodeUleb128(val: u64, out: []u8): usize;
export fn encodeSleb128(val: i64, out: []u8): usize;
export class FuncType {
    params: [8]ValType,
    param_count: usize,
    results: [4]ValType,
    result_count: usize,
    fn create(params: &[]ValType, results: &[]ValType): FuncType;
}

export class MemType {
    min_pages: u32,
    has_max: bool,
    max_pages: u32,
    fn unbounded(min_pages: u32): MemType;
    fn bounded(min_pages: u32, max_pages: u32): MemType;
}

export class ExportEntry {
    name: string,
    kind: ExportKind,
    index: u32,
}

export class FuncBody {
    local_counts: [8]u32,
    local_types: [8]ValType,
    local_group_count: usize,
    code: &[]u8,
    fn simple(code: &[]u8): FuncBody;
    fn addLocals(_: auto, count: u32, vt: ValType): bool;
}

export fn wencAppendByte(dst: []u8, pos: usize, b: u8): usize?;
export fn wencAppendBytes(dst: []u8, pos: usize, src: &[]u8): usize?;
export fn wencAppendUleb(dst: []u8, pos: usize, val: u64): usize?;
export class WasmModuleBuilder {
    buf: [4096]u8,
    len: usize,
    overflow: bool,
    fn init(): WasmModuleBuilder;
    fn writeSection(_: auto, section_id: u8, payload: &[]u8): bool;
    fn addTypeSection(_: auto, types: &[]FuncType): bool;
    fn addFunctionSection(_: auto, type_indices: &[]u32): bool;
    fn addMemorySection(_: auto, memories: &[]MemType): bool;
    fn addExportSection(_: auto, exports: &[]ExportEntry): bool;
    fn addCodeSection(_: auto, bodies: &[]FuncBody): bool;
    fn addCustomSection(_: auto, name: string, data: &[]u8): bool;
    fn byteLen(): usize;
    fn hasOverflow(): bool;
    fn finish(out: []u8): usize;
}

