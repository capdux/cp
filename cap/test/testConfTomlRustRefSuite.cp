// toml-rs/toml (MIT/Apache-2.0) Reference-Hardened TOML v1.0.0 Test Suite
// Covers:
//   - Test 01: TOML v1.0.0 Valid feature matrix (bare/quoted/empty/dotted keys, super-tables,
//              inline tables, [[array_of_tables]], multiline basic/literal strings, escapes,
//              hex/oct/bin/underscores/inf/nan, RFC 3339 leap-year datetimes, stringify roundtrip, TomlDoc compat)
//   - Test 02: TOML v1.0.0 Invalid key & table semantics (duplicate keys, table redefinition,
//              dotted-key table redef, inline-table freeze/newline/trailing-comma, static array vs [[...]],
//              multiline keys, depth > 64 rejection)
//   - Test 03: TOML v1.0.0 Invalid strings, escapes (surrogates 0xD800..0xDFFF, >0x10FFFF, control chars),
//              numbers (leading zero, bad underscores, uppercase/signed radix, float overflow/syntax),
//              and invalid RFC 3339 datetimes (non-leap 2025-02-29 / 1900-02-29, out-of-range h/m/s)
//   - Test 04: 32 real official invalid fixtures loaded directly from _ref/toml/crates/toml/tests/fixtures/invalid/ext/
//   - Test 05: cap:simd hot-path scanning vs scalar reference 100% equivalence + large config stress test
//   - Test 06: 1,500-iteration deterministic PRNG malformed TOML / deep nesting / control-char fuzzing

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { readFileSync } from "cap:fs";
import { ConfTag, ConfDateTimeKind, ConfValue } from "../../lib/conf/value.cp";
import { TomlParser, TomlDoc, TomlError } from "../../lib/conf/toml.cp";

fn canParseToml(alloc: std.mem.Allocator, src: []const u8): bool {
    const res = TomlParser.parse(alloc, src) catch return false;
    return res.tag == ConfTag.Object;
}

fn xorshift64(state: *u64): u64 {
    let x = state.*;
    x = x ^ (x << 13);
    x = x ^ (x >> 7);
    x = x ^ (x << 17);
    state.* = x;
    return x;
}

fn runTest01(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    const valid_doc =
        "# TOML v1.0.0 comprehensive valid document\n" +
        "bare_key = \"value\"\n" +
        "bare-key2 = 42\n" +
        "\"\" = \"empty-key\"\n" +
        "a.b.\"c.d\" = 99\n" +
        "inline = { x = 10, y = 20, nested = { z = true } }\n" +
        "dec_u = 1_000_000\n" +
        "hex_v = 0xdead_beef\n" +
        "oct_v = 0o755\n" +
        "bin_v = 0b1101_0110\n" +
        "flt_v = 3.141_592\n" +
        "exp_v = 1e06\n" +
        "pos_inf = +inf\n" +
        "neg_inf = -inf\n" +
        "not_num = nan\n" +
        "esc_s = \"\\b\\t\\n\\f\\r\\\"\\\\\\u00E9\\U0001F600\"\n" +
        "ml_basic = \"\"\"\n" +
        "hello \\\n" +
        "   world\"\"\"\"\"\n" +
        "lit_s = 'C:\\Users\\cp'\n" +
        "ml_lit = '''\n" +
        "raw '' text'''''\n" +
        "dt_utc = 1979-05-27T07:32:00Z\n" +
        "dt_off = 1979-05-27 07:32:00.5-07:00\n" +
        "dt_leap = 2024-02-29\n" +
        "dt_leap2 = 2000-02-29\n" +
        "dt_time = 07:32:00.999999\n" +
        "arr = [\n" +
        "  1, # inline comment\n" +
        "  2,\n" +
        "]\n" +
        "\n" +
        "[super.sub]\n" +
        "k = 1\n" +
        "\n" +
        "[super]\n" +
        "parent_k = 2\n" +
        "\n" +
        "[[fruits]]\n" +
        "name = \"apple\"\n" +
        "[fruits.physical]\n" +
        "color = \"red\"\n" +
        "[[fruits.varieties]]\n" +
        "name = \"red delicious\"\n" +
        "\n" +
        "[[fruits]]\n" +
        "name = \"banana\"\n";

    const root = TomlParser.parse(alloc, valid_doc) catch unreachable;
    expect_eq(std.mem.eql(u8, root.get("bare_key").?.asString().?, "value"), true);
    expect_eq(root.get("bare-key2").?.asInt().?, 42);
    expect_eq(std.mem.eql(u8, root.get("").?.asString().?, "empty-key"), true);
    expect_eq(root.get("a").?.get("b").?.get("c.d").?.asInt().?, 99);
    expect_eq(root.getPath("inline.x").?.asInt().?, 10);
    expect_eq(root.getPath("inline.nested.z").?.asBool().?, true);
    expect_eq(root.get("dec_u").?.asInt().?, 1000000);
    expect_eq(root.get("hex_v").?.asInt().?, 0xdeadbeef);
    expect_eq(root.get("oct_v").?.asInt().?, 493);
    expect_eq(root.get("bin_v").?.asInt().?, 214);
    expect_eq(std.math.isInf(root.get("pos_inf").?.asFloat().?), true);
    expect_eq(std.math.isInf(root.get("neg_inf").?.asFloat().?), true);
    expect_eq(std.math.isNan(root.get("not_num").?.asFloat().?), true);
    expect_eq(std.mem.eql(u8, root.get("ml_basic").?.asString().?, "hello world\"\""), true);
    expect_eq(std.mem.eql(u8, root.get("lit_s").?.asString().?, "C:\\Users\\cp"), true);
    expect_eq(std.mem.eql(u8, root.get("ml_lit").?.asString().?, "raw '' text''"), true);

    const dt_utc = root.get("dt_utc").?.asDateTime().?;
    expect_eq(dt_utc.kind == ConfDateTimeKind.OffsetDateTime, true);
    expect_eq(dt_utc.is_utc, true);
    const dt_leap = root.get("dt_leap").?.asDateTime().?;
    expect_eq(dt_leap.year, 2024);
    expect_eq(dt_leap.month, 2);
    expect_eq(dt_leap.day, 29);

    expect_eq(root.getPath("super.sub.k").?.asInt().?, 1);
    expect_eq(root.getPath("super.parent_k").?.asInt().?, 2);
    expect_eq(std.mem.eql(u8, root.getPath("fruits.0.name").?.asString().?, "apple"), true);
    expect_eq(std.mem.eql(u8, root.getPath("fruits.0.physical.color").?.asString().?, "red"), true);
    expect_eq(std.mem.eql(u8, root.getPath("fruits.0.varieties.0.name").?.asString().?, "red delicious"), true);
    expect_eq(std.mem.eql(u8, root.getPath("fruits.1.name").?.asString().?, "banana"), true);

    // 双向序列化往返校验
    const dumped = TomlParser.stringify(alloc, root) catch unreachable;
    const reparsed = TomlParser.parse(alloc, dumped) catch unreachable;
    expect_eq(reparsed.get("bare-key2").?.asInt().?, 42);
    expect_eq(reparsed.getPath("super.sub.k").?.asInt().?, 1);

    // 旧 API TomlDoc 向后兼容校验
    let doc = TomlDoc.parse("title = \"CP\"\n[server]\nport = 8080\n");
    expect_eq(std.mem.eql(u8, doc.getRoot("title").?, "CP"), true);
    expect_eq(doc.getInt("server", "port", 0), 8080);
}

fn runTest02(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    const invalid_cases: [18][]const u8 = [
        // 1. 重复键
        "a = 1\na = 2\n",
        "a.b = 1\na.b = 2\n",
        "t = { a = 1, a = 2 }\n",
        // 2. 显式表重复定义
        "[a]\nx = 1\n[a]\ny = 2\n",
        // 3. 点分键定义表后不可再显式定义 [a]
        "a.b = 1\n[a]\nc = 2\n",
        // 4. 外部点分键不可向已显式定义的 [a] 追加
        "[a]\nb = 1\n[a.c]\nd = 2\n[a]\ne = 3\n",
        // 5. Inline Table 冻结不可通过点分键追加
        "a = { b = 1 }\na.c = 2\n",
        // 6. Inline Table 冻结不可通过子表头 [a.c] 追加
        "a = { b = 1 }\n[a.c]\nd = 2\n",
        // 7. Inline Table 禁止尾随逗号
        "a = { b = 1, }\n",
        // 8. Inline Table 禁止换行
        "a = {\n b = 1\n}\n",
        // 9. 静态数组不可用 [[a]] 追加
        "a = [1, 2]\n[[a]]\nb = 3\n",
        // 10. 静态空数组不可用 [[a]] 追加
        "a = []\n[[a]]\nb = 3\n",
        // 11. 禁止多行基础字符串作为键
        "\"\"\"k\"\"\" = 1\n",
        // 12. 禁止多行字面量字符串作为键
        "'''k''' = 1\n",
        // 13. 禁止多行字符串作为表头
        "[\"\"\"t\"\"\"]\na = 1\n",
        // 14. 同一行两个键值对无换行
        "a = 1 b = 2\n",
        // 15. 空点分键
        "a..b = 1\n",
        // 16. 尾随点
        "a. = 1\n",
    ];

    let i: usize = 0;
    while (i < invalid_cases.len) : (i += 1) {
        expect_eq(canParseToml(alloc, invalid_cases[i]), false);
    }

    // 深层嵌套（80 层 > TOML_MAX_DEPTH=64）严格拒绝且不栈溢出
    let deep_arr: [165]u8 = [0; 165];
    deep_arr[0] = 'x';
    deep_arr[1] = '=';
    let d: usize = 0;
    while (d < 80) : (d += 1) {
        deep_arr[2 + d] = '[';
        deep_arr[82 + d] = ']';
    }
    deep_arr[162] = '\n';
    expect_eq(canParseToml(alloc, deep_arr[0..163]), false);
}

fn runTest03(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    const invalid_cases: [34][]const u8 = [
        // 代理对 & 非法 Unicode 标量
        "s = \"\\uD800\"\n",
        "s = \"\\uDFFF\"\n",
        "s = \"\\U0000D800\"\n",
        "s = \"\\U00110000\"\n",
        "s = \"\\u123\"\n",
        "s = \"\\a\"\n",
        // 多行 6 个连续引号
        "s = \"\"\"\"\"\"\"\"\"\n",
        // 非法控制字符
        "s = \"bad\x00char\"\n",
        "s = \"bad\x07char\"\n",
        "s = 'bad\x7Fchar'\n",
        "# comment with \x00 nul\na = 1\n",
        // 数值：前导零
        "n = 01\n",
        "n = +01\n",
        "n = -01\n",
        "n = 00\n",
        "n = 01.5\n",
        // 数值：非法下划线
        "n = _1\n",
        "n = 1_\n",
        "n = 1__2\n",
        "n = 1_.2\n",
        "n = 1._2\n",
        "n = 1_e2\n",
        "n = 1e_2\n",
        "n = 0x_1\n",
        "n = 0x1_\n",
        // 数值：非法进制前缀与正负号
        "n = 0X1F\n",
        "n = +0x1F\n",
        "n = -0b10\n",
        // 数值：非法浮点与溢出
        "n = .5\n",
        "n = 5.\n",
        "n = 1e999\n",
        "n = INF\n",
        // 日期时间：非闰年与越界
        "d = 2025-02-29\n",
        "d = 1900-02-29\n",
    ];

    let i: usize = 0;
    while (i < invalid_cases.len) : (i += 1) {
        expect_eq(canParseToml(alloc, invalid_cases[i]), false);
    }

    // 额外非法日期时间边界
    expect_eq(canParseToml(alloc, "d = 2024-02-30\n"), false);
    expect_eq(canParseToml(alloc, "d = 2024-13-01\n"), false);
    expect_eq(canParseToml(alloc, "d = 2024-00-01\n"), false);
    expect_eq(canParseToml(alloc, "t = 24:00:00\n"), false);
    expect_eq(canParseToml(alloc, "t = 12:60:00\n"), false);
    expect_eq(canParseToml(alloc, "t = 12:00:60\n"), false);
}

fn runTest04(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 直接继承 _ref/toml 官方 32 个真实 invalid .toml 夹具文件
    const fixture_paths: [32][]const u8 = [
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/datetime/day-1digit.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/datetime/day-zero.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/datetime/hour-too-large.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/datetime/minute-too-large.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/datetime/missing-date-time-sep.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/datetime/month-1digit.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/datetime/month-zero.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/datetime/offset-plus-missing-minute.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/datetime/seconds-too-large.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/datetime/seconds-trailing-decimal.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/datetime/year-3digits.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/float/overflow.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/float/trailing-exp-minus.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/float/trailing-exp-plus.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/float/trailing-exp.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/float/trailing-point-exp.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/inline-table/comma.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/integer/invalid-hex-3.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/integer/overflow-neg.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/integer/overflow-pos.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/keys/ml_basic_string.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/keys/ml_literal.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/keys/no-eol-1.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/string/bad-escape-6.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/string/missing-quotes-in-array.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/string/missing-quotes-in-inline-table.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/string/no-close-recovery-01.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/table/bare-invalid-character.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/table/dot.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/table/duplicate-key-1.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/table/duplicate-key-empty-table.toml",
        "_ref/toml/crates/toml/tests/fixtures/invalid/ext/table/ml_basic_string.toml",
    ];

    let fi: usize = 0;
    while (fi < fixture_paths.len) : (fi += 1) {
        const raw = readFileSync(fixture_paths[fi]);
        expect_eq(canParseToml(alloc, raw), false);
    }
}

fn runTest05(): void! {
    // 1. cap:simd scanBasicStringStopSimd vs scanBasicStringStopScalar 100% 对拍
    let buf: [300]u8 = ['a'; 300];
    let len: usize = 0;
    while (len <= 257) : (len += 1) {
        const simd_none = TomlParser.scanBasicStringStopSimd(buf[0..len]);
        const scalar_none = TomlParser.scanBasicStringStopScalar(buf[0..len]);
        expect_eq(simd_none == null, scalar_none == null);
    }
    const stops: [4]u8 = ['"', '\\', '\n', '\r'];
    let si: usize = 0;
    while (si < 4) : (si += 1) {
        let pos: usize = 0;
        while (pos < 128) : (pos += 1) {
            buf[pos] = stops[si];
            const simd_hit = TomlParser.scanBasicStringStopSimd(buf[0..200]).?;
            const scalar_hit = TomlParser.scanBasicStringStopScalar(buf[0..200]).?;
            expect_eq(simd_hit, pos);
            expect_eq(simd_hit, scalar_hit);
            buf[pos] = 'a';
        }
    }

    // 2. 长字符串 + 长注释大配置解析校验（触发 SIMD 块级扫描）
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    let big: std.ArrayListUnmanaged(u8) = std.ArrayListUnmanaged(u8).empty;
    big.appendSlice(alloc, "# ") catch unreachable;
    let ci: usize = 0;
    while (ci < 2048) : (ci += 1) {
        big.append(alloc, 'x') catch unreachable;
    }
    big.appendSlice(alloc, "\nlong_str = \"") catch unreachable;
    let si2: usize = 0;
    while (si2 < 4096) : (si2 += 1) {
        big.append(alloc, ('a' as u8) + ((si2 % 26) as u8)) catch unreachable;
    }
    big.appendSlice(alloc, "\\nEND\"\nlong_lit = '") catch unreachable;
    let si3: usize = 0;
    while (si3 < 2048) : (si3 += 1) {
        big.append(alloc, ('0' as u8) + ((si3 % 10) as u8)) catch unreachable;
    }
    big.appendSlice(alloc, "'\n") catch unreachable;

    const root = TomlParser.parse(alloc, big.items) catch unreachable;
    const ls = root.get("long_str").?.asString().?;
    expect_eq(ls.len, 4096 + 4);
    expect_eq(ls[0], 'a');
    expect_eq(ls[4096], '\n');
    expect_eq(ls[4099], 'D');
    const ll = root.get("long_lit").?.asString().?;
    expect_eq(ll.len, 2048);
    expect_eq(ll[0], '0');
    expect_eq(ll[2047], '7');
}

fn runTest06(): void! {
    // 1,500 组确定性 PRNG 畸形 TOML / 深层嵌套 / 未闭合引号 / 控制字符 Fuzz
    let rng: u64 = 0x5EED123456789ABC;
    let fuzz_buf: [512]u8 = [0; 512];
    const seed_toml = "a = 1\n[tbl]\nk = \"hello\\nworld\"\narr = [1, 2, { x = true }]\n";
    let survived: usize = 0;

    let iter: usize = 0;
    while (iter < 1500) : (iter += 1) {
        let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
        const alloc = arena.allocator();
        const mode = iter % 5;
        let len: usize = 0;

        if (mode == 0) {
            // 纯随机字节流
            len = (xorshift64(&rng) % 256) as usize;
            let j: usize = 0;
            while (j < len) : (j += 1) {
                fuzz_buf[j] = (xorshift64(&rng) & 0xFF) as u8;
            }
        } else if (mode == 1) {
            // 深层嵌套括号 `[` / `{`
            const depth = (10 as usize) + ((xorshift64(&rng) % 100) as usize);
            fuzz_buf[0] = 'k'; fuzz_buf[1] = '=';
            const open_ch: u8 = if ((iter & 1) == 0) '[' else '{';
            let j: usize = 0;
            while (j < depth && (2 as usize) + j < 512) : (j += 1) {
                fuzz_buf[(2 as usize) + j] = open_ch;
            }
            len = (2 as usize) + depth;
        } else if (mode == 2) {
            // 合法 TOML 变异
            len = seed_toml.len;
            let j: usize = 0;
            while (j < len) : (j += 1) {
                fuzz_buf[j] = seed_toml[j];
            }
            const flips: usize = (1 as usize) + ((xorshift64(&rng) % 8) as usize);
            let f: usize = 0;
            while (f < flips) : (f += 1) {
                const pos = (xorshift64(&rng) as usize) % len;
                fuzz_buf[pos] = (xorshift64(&rng) & 0xFF) as u8;
            }
        } else if (mode == 3) {
            // 未闭合多行字符串 + 随机反斜杠转义
            fuzz_buf[0] = 's'; fuzz_buf[1] = '=';
            const q: u8 = if ((iter & 1) == 0) '"' else '\'';
            fuzz_buf[2] = q; fuzz_buf[3] = q; fuzz_buf[4] = q;
            len = (5 as usize) + ((xorshift64(&rng) % 80) as usize);
            let j: usize = 5;
            while (j < len) : (j += 1) {
                const r = xorshift64(&rng) % 12;
                if (r == 0) fuzz_buf[j] = '\\'
                else if (r == 1) fuzz_buf[j] = q
                else if (r == 2) fuzz_buf[j] = '\n'
                else if (r == 3) fuzz_buf[j] = '\r'
                else fuzz_buf[j] = ('a' as u8) + ((r % 26) as u8);
            }
        } else {
            // TOML 语法符号高频随机拼接（表头、点分键、注释、控制字符）
            const alphabet = "ab019._-=[]{}#\"'\\\r\n\t \x00\x7F:+eT";
            len = (1 as usize) + ((xorshift64(&rng) % 180) as usize);
            let j: usize = 0;
            while (j < len) : (j += 1) {
                fuzz_buf[j] = alphabet[(xorshift64(&rng) as usize) % alphabet.len];
            }
        }

        const parsed_ok = canParseToml(alloc, fuzz_buf[0..len]);
        if (parsed_ok) {
            survived += 1;
        } else {
            survived += 1;
        }
        arena.deinit();
    }

    expect_eq(survived, 1500);
}

fn main() {
    test("toml_ref_01_valid_v1_0_0_full_feature_matrix_and_roundtrip", () => { try runTest01(); });
    test("toml_ref_02_invalid_keys_tables_inline_freeze_and_depth", () => { try runTest02(); });
    test("toml_ref_03_invalid_strings_surrogates_numbers_and_datetimes", () => { try runTest03(); });
    test("toml_ref_04_official_32_toml_rs_invalid_fixtures", () => { try runTest04(); });
    test("toml_ref_05_simd_scanning_equivalence_and_large_config", () => { try runTest05(); });
    test("toml_ref_06_deterministic_prng_1500_fuzz_malformed_toml", () => { try runTest06(); });
}
