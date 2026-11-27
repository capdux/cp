import { test, expect_eq } from "cap:test";
import {
    ValType,
    ExportKind,
    FuncType,
    MemType,
    ExportEntry,
    FuncBody,
    WasmModuleBuilder,
    encodeUleb128,
    encodeSleb128,
    SEC_CUSTOM,
    SEC_TYPE,
    SEC_FUNCTION,
    SEC_MEMORY,
    SEC_EXPORT,
    SEC_CODE,
    OP_LOCAL_GET,
    OP_I32_ADD,
    OP_END,
} from "../../packages/wasm-encoder/src/index.cp";

fn main() {
    test("1. @cap/wasm-encoder — unsigned LEB128 encoding", () => {
        let buf: [16]u8 = undefined;

        const n0 = encodeUleb128(0, buf[0..16]);
        expect_eq(n0, 1);
        expect_eq(buf[0], 0x00);

        const n1 = encodeUleb128(127, buf[0..16]);
        expect_eq(n1, 1);
        expect_eq(buf[0], 0x7F);

        const n2 = encodeUleb128(128, buf[0..16]);
        expect_eq(n2, 2);
        expect_eq(buf[0], 0x80);
        expect_eq(buf[1], 0x01);

        // 624485 -> 0xE5 0x8E 0x26
        const n3 = encodeUleb128(624485, buf[0..16]);
        expect_eq(n3, 3);
        expect_eq(buf[0], 0xE5);
        expect_eq(buf[1], 0x8E);
        expect_eq(buf[2], 0x26);
    });

    test("2. @cap/wasm-encoder — signed LEB128 encoding", () => {
        let buf: [16]u8 = undefined;

        const n0 = encodeSleb128(0, buf[0..16]);
        expect_eq(n0, 1);
        expect_eq(buf[0], 0x00);

        const n1 = encodeSleb128(-1, buf[0..16]);
        expect_eq(n1, 1);
        expect_eq(buf[0], 0x7F);

        const n2 = encodeSleb128(64, buf[0..16]);
        expect_eq(n2, 2);
        expect_eq(buf[0], 0xC0);
        expect_eq(buf[1], 0x00);

        const n3 = encodeSleb128(-64, buf[0..16]);
        expect_eq(n3, 1);
        expect_eq(buf[0], 0x40);

        // -123456 -> 0xC0 0xBB 0x78
        const n4 = encodeSleb128(-123456, buf[0..16]);
        expect_eq(n4, 3);
        expect_eq(buf[0], 0xC0);
        expect_eq(buf[1], 0xBB);
        expect_eq(buf[2], 0x78);
    });

    test("3. @cap/wasm-encoder — empty module header magic & version", () => {
        let m = WasmModuleBuilder.init();
        let out: [32]u8 = undefined;
        const n = m.finish(out[0..32]);
        expect_eq(n, 8);
        expect_eq(out[0], 0x00);
        expect_eq(out[1], 0x61);
        expect_eq(out[2], 0x73);
        expect_eq(out[3], 0x6D);
        expect_eq(out[4], 0x01);
        expect_eq(out[5], 0x00);
        expect_eq(out[6], 0x00);
        expect_eq(out[7], 0x00);
    });

    test("4. @cap/wasm-encoder — complete add(i32, i32) -> i32 .wasm byte-for-byte check", () => {
        let m = WasmModuleBuilder.init();

        const params: [2]ValType = [ValType.I32, ValType.I32];
        const results: [1]ValType = [ValType.I32];
        const types: [1]FuncType = [FuncType.create(params[0..2], results[0..1])];
        expect_eq(m.addTypeSection(types[0..1]), true);

        const funcs: [1]u32 = [0];
        expect_eq(m.addFunctionSection(funcs[0..1]), true);

        const exports: [1]ExportEntry = [
            ExportEntry{ name: "add", kind: ExportKind.Func, index: 0 },
        ];
        expect_eq(m.addExportSection(exports[0..1]), true);

        const code: [6]u8 = [OP_LOCAL_GET, 0x00, OP_LOCAL_GET, 0x01, OP_I32_ADD, OP_END];
        const bodies: [1]FuncBody = [FuncBody.simple(code[0..6])];
        expect_eq(m.addCodeSection(bodies[0..1]), true);

        let out: [64]u8 = undefined;
        const n = m.finish(out[0..64]);

        // Expected layout (41 bytes total):
        // [0..8]   Magic + Version: 00 61 73 6D 01 00 00 00
        // [8..17]  Type section (id=1, len=7): 01 07 | 01 60 02 7F 7F 01 7F
        // [17..21] Function section (id=3, len=2): 03 02 | 01 00
        // [21..30] Export section (id=7, len=7): 07 07 | 01 03 61 64 64 00 00
        // [30..41] Code section (id=10, len=9): 0A 09 | 01 07 00 20 00 20 01 6A 0B
        expect_eq(n, 41);

        // Header
        expect_eq(out[0], 0x00);
        expect_eq(out[1], 0x61);
        expect_eq(out[2], 0x73);
        expect_eq(out[3], 0x6D);
        expect_eq(out[4], 0x01);

        // Type section
        expect_eq(out[8], SEC_TYPE);
        expect_eq(out[9], 7);
        expect_eq(out[10], 1);
        expect_eq(out[11], 0x60);
        expect_eq(out[12], 2);
        expect_eq(out[13], 0x7F);
        expect_eq(out[14], 0x7F);
        expect_eq(out[15], 1);
        expect_eq(out[16], 0x7F);

        // Function section
        expect_eq(out[17], SEC_FUNCTION);
        expect_eq(out[18], 2);
        expect_eq(out[19], 1);
        expect_eq(out[20], 0);

        // Export section
        expect_eq(out[21], SEC_EXPORT);
        expect_eq(out[22], 7);
        expect_eq(out[23], 1);
        expect_eq(out[24], 3);
        expect_eq(out[25], 0x61); // 'a'
        expect_eq(out[26], 0x64); // 'd'
        expect_eq(out[27], 0x64); // 'd'
        expect_eq(out[28], 0x00); // Func
        expect_eq(out[29], 0x00); // idx 0

        // Code section
        expect_eq(out[30], SEC_CODE);
        expect_eq(out[31], 9);
        expect_eq(out[32], 1);    // 1 body
        expect_eq(out[33], 7);    // body len = 1 (locals=0) + 6 (code)
        expect_eq(out[34], 0);    // 0 local groups
        expect_eq(out[35], 0x20); // local.get
        expect_eq(out[36], 0x00); // 0
        expect_eq(out[37], 0x20); // local.get
        expect_eq(out[38], 0x01); // 1
        expect_eq(out[39], 0x6A); // i32.add
        expect_eq(out[40], 0x0B); // end
    });

    test("5. @cap/wasm-encoder — memory section (unbounded & bounded) and custom section", () => {
        let m = WasmModuleBuilder.init();
        const mems: [2]MemType = [MemType.unbounded(1), MemType.bounded(2, 16)];
        expect_eq(m.addMemorySection(mems[0..2]), true);

        const custom_data: [2]u8 = [0xCA, 0xFE];
        expect_eq(m.addCustomSection("meta", custom_data[0..2]), true);

        let out: [64]u8 = undefined;
        const n = m.finish(out[0..64]);
        // 8 (hdr) + 2 + 6 (mem: 01 02 -> cnt=2, 00 01, 01 02 10) + 2 + 7 (custom: 04 'm' 'e' 't' 'a' CA FE) = 25
        expect_eq(n, 25);
        expect_eq(out[8], SEC_MEMORY);
        expect_eq(out[9], 6);
        expect_eq(out[10], 2);
        expect_eq(out[11], 0x00);
        expect_eq(out[12], 1);
        expect_eq(out[13], 0x01);
        expect_eq(out[14], 2);
        expect_eq(out[15], 16);

        expect_eq(out[16], SEC_CUSTOM);
        expect_eq(out[17], 7);
        expect_eq(out[18], 4);
        expect_eq(out[23], 0xCA);
        expect_eq(out[24], 0xFE);
    });

    test("6. @cap/wasm-encoder — FuncBody locals encoding & finish buffer-too-small guard", () => {
        let m = WasmModuleBuilder.init();
        const code: [1]u8 = [OP_END];
        let body = FuncBody.simple(code[0..1]);
        expect_eq(body.addLocals(2, ValType.I64), true);
        expect_eq(body.addLocals(1, ValType.F32), true);
        const bodies: [1]FuncBody = [body];
        expect_eq(m.addCodeSection(bodies[0..1]), true);

        let tiny: [4]u8 = undefined;
        expect_eq(m.finish(tiny[0..4]), 0);

        let out: [32]u8 = undefined;
        const n = m.finish(out[0..32]);
        // 8 (hdr) + 2 (sec hdr) + 8 (cnt=1, body_len=6, local_groups=2, 2 i64, 1 f32, end) = 18
        expect_eq(n, 18);
        expect_eq(out[8], SEC_CODE);
        expect_eq(out[9], 8);
        expect_eq(out[12], 2);    // 2 local groups
        expect_eq(out[13], 2);    // count 2
        expect_eq(out[14], 0x7E); // I64
        expect_eq(out[15], 1);    // count 1
        expect_eq(out[16], 0x7D); // F32
        expect_eq(out[17], 0x0B); // end
    });
}
