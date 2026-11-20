// Source: cp/lib/wasm_fmt/index.cp (Pure CP WebAssembly binary format & LEB128 module entry)
export {
    WASM_MAGIC,
    WASM_VERSION,
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
} from "./types.cp";

export {
    readUleb128,
    writeUleb128,
    readSleb128,
    writeSleb128,
} from "./leb128.cp";

export {
    parseWasm,
    encodeWasm,
    findExport,
    findExportFuncType,
    findCustomSection,
    addCustomSection,
} from "./codec.cp";
