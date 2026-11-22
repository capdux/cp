import std from "std";
import { test, expect_eq } from "cap:test";
import {
    MAX_DEFINE_RULES,
    CallUnwrapKind,
    DefineValueKind,
    DefineFlags,
    DefineValue,
    DefineEntry,
    isValidDefineKey,
    DefineTable,
} from "cap:bundler";

fn main() {
    test("bundler_defines: isValidDefineKey validates identifiers & dotted chains", () => {
        expect_eq(isValidDefineKey("__DEV__"), true);
        expect_eq(isValidDefineKey("$app_1"), true);
        expect_eq(isValidDefineKey("process.env.NODE_ENV"), true);
        expect_eq(isValidDefineKey("import.meta.env.MODE"), true);

        expect_eq(isValidDefineKey(""), false);
        expect_eq(isValidDefineKey("123bad"), false);
        expect_eq(isValidDefineKey("process."), false);
        expect_eq(isValidDefineKey(".process"), false);
        expect_eq(isValidDefineKey("process..env"), false);
        expect_eq(isValidDefineKey("a-b"), false);
        expect_eq(isValidDefineKey("a b"), false);
    });

    test("bundler_defines: DefineValue.parse & formatJs for all literal kinds", () => {
        let buf: [64]u8 = [0; 64];

        const v_undef = DefineValue.parse("  undefined  ").?;
        expect_eq(v_undef.kind == DefineValueKind.undefined_val, true);
        expect_eq(std.mem.eql(u8, v_undef.formatJs(buf[0..]).?, "undefined"), true);

        const v_null = DefineValue.parse("null").?;
        expect_eq(v_null.kind == DefineValueKind.null_val, true);
        expect_eq(std.mem.eql(u8, v_null.formatJs(buf[0..]).?, "null"), true);

        const v_bool = DefineValue.parse("false").?;
        expect_eq(v_bool.kind == DefineValueKind.boolean, true);
        expect_eq(v_bool.bool_val, false);
        expect_eq(std.mem.eql(u8, v_bool.formatJs(buf[0..]).?, "false"), true);

        const v_num = DefineValue.parse("42.5").?;
        expect_eq(v_num.kind == DefineValueKind.number, true);
        expect_eq(std.mem.eql(u8, v_num.formatJs(buf[0..]).?, "42.5"), true);

        const v_inf = DefineValue.parse("-Infinity").?;
        expect_eq(v_inf.kind == DefineValueKind.number, true);
        expect_eq(std.mem.eql(u8, v_inf.formatJs(buf[0..]).?, "-Infinity"), true);

        const v_str = DefineValue.parse("'production'").?;
        expect_eq(v_str.kind == DefineValueKind.string_lit, true);
        expect_eq(std.mem.eql(u8, v_str.formatJs(buf[0..]).?, "\"production\""), true);

        const v_id = DefineValue.parse("globalThis.fetch").?;
        expect_eq(v_id.kind == DefineValueKind.identifier, true);
        expect_eq(std.mem.eql(u8, v_id.formatJs(buf[0..]).?, "globalThis.fetch"), true);

        // Invalid values rejected
        expect_eq(DefineValue.parse("") == null, true);
        expect_eq(DefineValue.parse("'unterminated") == null, true);
        expect_eq(DefineValue.parse("12abc") == null, true);
        // Tiny output buffer rejected safely
        let tiny: [2]u8 = [0; 2];
        expect_eq(v_undef.formatJs(tiny[0..]) == null, true);
    });

    test("bundler_defines: DefineTable lookup, lookupParts & hasDotPrefix pruning", () => {
        let tbl = DefineTable.init();
        expect_eq(tbl.defineRaw("process.env.NODE_ENV", "\"production\""), true);
        expect_eq(tbl.defineRaw("__DEV__", "false"), true);
        expect_eq(tbl.defineRaw("fetch", "globalThis.fetch"), true);
        expect_eq(tbl.count(), 3 as usize);

        // Exact lookup
        const env_entry: DefineEntry = tbl.lookup("process.env.NODE_ENV").?;
        expect_eq(env_entry.is_dot_chain, true);
        expect_eq(env_entry.canBeRemovedIfUnused(), true);
        expect_eq(std.mem.eql(u8, env_entry.value.str_val, "production"), true);

        // Identifier alias is not marked pure-literal by default
        const fetch_entry: DefineEntry = tbl.lookup("fetch").?;
        expect_eq(fetch_entry.canBeRemovedIfUnused(), false);

        // Zero-alloc segmented AST lookup
        const parts: [3][]const u8 = ["process", "env", "NODE_ENV"];
        const seg_hit: DefineEntry = tbl.lookupParts(parts[0..]).?;
        expect_eq(std.mem.eql(u8, seg_hit.value.str_val, "production"), true);

        const wrong_parts: [2][]const u8 = ["process", "env"];
        expect_eq(tbl.lookupParts(wrong_parts[0..]) == null, true);

        // Prefix pruning check
        expect_eq(tbl.hasDotPrefix("process"), true);
        expect_eq(tbl.hasDotPrefix("process.env"), true);
        expect_eq(tbl.hasDotPrefix("__DEV__"), false);
        expect_eq(tbl.hasDotPrefix("window"), false);
    });

    test("bundler_defines: --drop rules replace method calls with undefined", () => {
        let tbl = DefineTable.init();
        expect_eq(tbl.addDrop("console.log"), true);
        expect_eq(tbl.addDrop("console.debug"), true);

        const e: DefineEntry = tbl.lookup("console.log").?;
        expect_eq(e.dropsMethodCall(), true);
        expect_eq(e.canBeRemovedIfUnused(), true);
        expect_eq(e.flags.call_can_be_unwrapped_if_unused == CallUnwrapKind.always, true);
        expect_eq(e.value.kind == DefineValueKind.undefined_val, true);
        expect_eq(tbl.hasDotPrefix("console"), true);
    });

    test("bundler_defines: overwriting existing keys preserves slot count", () => {
        let tbl = DefineTable.init();
        expect_eq(tbl.defineRaw("__VERSION__", "\"1.0.0\""), true);
        expect_eq(tbl.count(), 1 as usize);

        expect_eq(tbl.defineRaw("__VERSION__", "\"2.0.0\""), true);
        expect_eq(tbl.count(), 1 as usize);

        const e: DefineEntry = tbl.lookup("__VERSION__").?;
        expect_eq(std.mem.eql(u8, e.value.str_val, "2.0.0"), true);

        // Invalid key/value rejected without mutating table
        expect_eq(tbl.defineRaw("bad..key", "1"), false);
        expect_eq(tbl.defineRaw("goodKey", ""), false);
        expect_eq(tbl.count(), 1 as usize);
    });

    test("bundler_defines: custom valueless DefineFlags & capacity limit", () => {
        let tbl = DefineTable.init();
        expect_eq(tbl.defineEntry("Object.create", DefineValue.makeUndefined(), DefineFlags{
            valueless: true,
            can_be_removed_if_unused: true,
            call_can_be_unwrapped_if_unused: CallUnwrapKind.if_unused,
        }), true);

        const e: DefineEntry = tbl.lookup("Object.create").?;
        expect_eq(e.isValueless(), true);
        expect_eq(e.canBeRemovedIfUnused(), true);
        expect_eq(e.flags.call_can_be_unwrapped_if_unused == CallUnwrapKind.if_unused, true);

        // Fill remaining 63 slots to verify MAX_DEFINE_RULES (64) backpressure
        let keys: [64][4]u8 = [[0; 4]; 64];
        let i: usize = 0;
        while (i < 63) : (i += 1) {
            keys[i][0] = 'k';
            keys[i][1] = @as(u8, 'a' + @as(u8, @intCast(i / 26)));
            keys[i][2] = @as(u8, 'a' + @as(u8, @intCast(i % 26)));
            keys[i][3] = 'x';
            expect_eq(tbl.defineRaw(keys[i][0..4], "1"), true);
        }
        expect_eq(tbl.count(), MAX_DEFINE_RULES);
        expect_eq(tbl.defineRaw("overflow_key", "1"), false);
    });
}
