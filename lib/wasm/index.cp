// Source: cp/lib/wasm/index.cp (Unified Exports for cap:wasm)
// Pure CP WebAssembly 1.0 binary module parser, validator, LEB128 codec & module builder.

export {
    WasmError,
    Leb128,
} from "./leb128.cp";

export {
    WASM_MAGIC,
    WASM_VERSION,
    WasmValType,
    WasmExternKind,
    WasmSectionId,
    WasmLimits,
    WasmFuncType,
    WasmImport,
    WasmExport,
    WasmCustomSection,
    WasmLocalDecl,
    WasmCodeBody,
    WasmModule,
} from "./module.cp";

export {
    WasmBuilder,
} from "./builder.cp";
