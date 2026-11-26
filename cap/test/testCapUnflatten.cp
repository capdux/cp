// cap/test/testCapUnflatten.cp
// Comprehensive unit tests for @cap/unflatten (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    FlatOptions,
    FlatTable,
    flattenJson,
    flattenToJson,
    unflattenJson,
} from "../../packages/@cap/unflatten/src/index.cp";

fn main() {
    test("1. @cap/unflatten — nested object flattenJson & flattenToJson", () => {
        const input = "{\"user\":{\"name\":\"alice\",\"meta\":{\"age\":30,\"active\":true}},\"tag\":null}";
        const opts = FlatOptions.defaults();
        let table = FlatTable.new();
        const cnt = flattenJson(input, opts, &table);
        expect_eq(cnt, 4 as i64);
        expect_eq(table.count(), 4 as usize);

        let vbuf: [128]u8 = undefined;
        const n1 = table.get("user.name", vbuf[0..128]);
        expect_eq(vbuf[0..(n1 as usize)], "\"alice\"");
        const n2 = table.get("user.meta.age", vbuf[0..128]);
        expect_eq(vbuf[0..(n2 as usize)], "30");
        const n3 = table.get("user.meta.active", vbuf[0..128]);
        expect_eq(vbuf[0..(n3 as usize)], "true");
        const n4 = table.get("tag", vbuf[0..128]);
        expect_eq(vbuf[0..(n4 as usize)], "null");
        expect_eq(table.get("missing.key", vbuf[0..128]), -1 as i64);

        let outBuf: [256]u8 = undefined;
        const jlen = flattenToJson(input, opts, outBuf[0..256]);
        expect_eq(
            outBuf[0..(jlen as usize)],
            "{\"user.name\":\"alice\",\"user.meta.age\":30,\"user.meta.active\":true,\"tag\":null}",
        );
    });

    test("2. @cap/unflatten — array index flattening and safe:true array preservation", () => {
        const input = "{\"a\":{\"b\":[10,20],\"c\":\"hi\"}}";
        const defOpts = FlatOptions.defaults();
        let table = FlatTable.new();
        const cnt = flattenJson(input, defOpts, &table);
        expect_eq(cnt, 3 as i64);

        let vbuf: [128]u8 = undefined;
        const v0 = table.get("a.b.0", vbuf[0..128]);
        expect_eq(vbuf[0..(v0 as usize)], "10");
        const v1 = table.get("a.b.1", vbuf[0..128]);
        expect_eq(vbuf[0..(v1 as usize)], "20");
        const v2 = table.get("a.c", vbuf[0..128]);
        expect_eq(vbuf[0..(v2 as usize)], "\"hi\"");

        // safe: true preserves arrays intact
        const safeOpts = FlatOptions {
            delimiter: 46,
            max_depth: 0,
            safe: true,
        };
        let safeBuf: [256]u8 = undefined;
        const slen = flattenToJson(input, safeOpts, safeBuf[0..256]);
        expect_eq(safeBuf[0..(slen as usize)], "{\"a.b\":[10,20],\"a.c\":\"hi\"}");
    });

    test("3. @cap/unflatten — custom delimiter ('_' / '/') and max_depth truncation", () => {
        const input = "{\"a\":{\"b\":{\"c\":42}}}";

        // Custom delimiter '/'
        const slashOpts = FlatOptions {
            delimiter: 47, // '/'
            max_depth: 0,
            safe: false,
        };
        let outBuf: [256]u8 = undefined;
        const n1 = flattenToJson(input, slashOpts, outBuf[0..256]);
        expect_eq(outBuf[0..(n1 as usize)], "{\"a/b/c\":42}");

        // Custom delimiter '_' + max_depth = 2 (stops at a_b)
        const depthOpts = FlatOptions {
            delimiter: 95, // '_'
            max_depth: 2,
            safe: false,
        };
        const n2 = flattenToJson(input, depthOpts, outBuf[0..256]);
        expect_eq(outBuf[0..(n2 as usize)], "{\"a_b\":{\"c\":42}}");
    });

    test("4. @cap/unflatten — unflattenJson reconstructs nested JSON merging shared prefixes", () => {
        const flat = "{\"a.b.c\":1,\"a.b.d\":2,\"a.e\":true,\"x\":\"ok\"}";
        const opts = FlatOptions.defaults();
        let outBuf: [256]u8 = undefined;
        const n = unflattenJson(flat, opts, outBuf[0..256]);
        expect_eq(n > 0, true);
        expect_eq(
            outBuf[0..(n as usize)],
            "{\"a\":{\"b\":{\"c\":1,\"d\":2},\"e\":true},\"x\":\"ok\"}",
        );
    });

    test("5. @cap/unflatten — flattenToJson <-> unflattenJson bidirectional round-trip", () => {
        const original = "{\"cfg\":{\"ports\":[80,443],\"tls\":{\"enabled\":true,\"min\":\"1.3\"}},\"name\":\"edge\"}";
        const opts = FlatOptions.defaults();
        let flatBuf: [512]u8 = undefined;
        let restoredBuf: [512]u8 = undefined;

        const flen = flattenToJson(original, opts, flatBuf[0..512]);
        expect_eq(flen > 0, true);
        expect_eq(
            flatBuf[0..(flen as usize)],
            "{\"cfg.ports.0\":80,\"cfg.ports.1\":443,\"cfg.tls.enabled\":true,\"cfg.tls.min\":\"1.3\",\"name\":\"edge\"}",
        );

        const rlen = unflattenJson(flatBuf[0..(flen as usize)], opts, restoredBuf[0..512]);
        expect_eq(rlen > 0, true);
        expect_eq(restoredBuf[0..(rlen as usize)], original);
    });

    test("6. @cap/unflatten — empty object {}, malformed JSON, and small buffer safety", () => {
        const opts = FlatOptions.defaults();
        let outBuf: [128]u8 = undefined;

        // Empty object round-trip
        const e1 = flattenToJson("{}", opts, outBuf[0..128]);
        expect_eq(outBuf[0..(e1 as usize)], "{}");
        const e2 = unflattenJson("{}", opts, outBuf[0..128]);
        expect_eq(outBuf[0..(e2 as usize)], "{}");

        // Malformed JSON rejected with -1
        expect_eq(flattenToJson("{invalid", opts, outBuf[0..128]), -1 as i64);
        expect_eq(flattenToJson("{\"a\":1} trailing", opts, outBuf[0..128]), -1 as i64);
        expect_eq(unflattenJson("not-json", opts, outBuf[0..128]), -1 as i64);
        expect_eq(unflattenJson("{\"a.b\":}", opts, outBuf[0..128]), -1 as i64);

        // Small output buffer returns -1 safely without overflow
        let tiny: [4]u8 = undefined;
        expect_eq(flattenToJson("{\"a\":{\"b\":123}}", opts, tiny[0..4]), -1 as i64);
        expect_eq(unflattenJson("{\"a.b\":123}", opts, tiny[0..4]), -1 as i64);
    });
}
