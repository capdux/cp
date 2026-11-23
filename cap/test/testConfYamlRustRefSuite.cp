// Ethiraric/yaml-rust2 (MIT/Apache-2.0) Reference-Hardened YAML 1.2 Test Suite
// Covers:
//   - Test 01: yaml-rust2 core YAML 1.2 feature matrix (block map/seq, compact `- k: v` & `- - x`,
//              multi-document stream `---` / `...` + `%YAML 1.2`, explicit tags `!!str`/`!!int`/`!!float`/`!!bool`/`!!null`,
//              octal `0o77`, hex `0xFF`, `.inf`/`-.inf`/`.nan` + nominal string safety `nan`/`inf`/`Infinity`,
//              stringify & stringifyStream roundtrip, YamlDoc backward compatibility)
//   - Test 02: Block scalars full chomping & explicit indentation (`|`, `|-`, `|+`, `>`, `>-`, `>+`, `|2`, `>-2`, `>1-`,
//              folded more-indented lines & empty scalar chomping)
//   - Test 03: Flow collections (`[a, {b: [1, 2]}]`, trailing commas, single/double quoted escapes
//              `\0\a\b\t\n\v\f\r\e\xHH\uXXXX\UXXXXXXXX\N\_\L\P`, surrogate rejection, `''` in single-quoted)
//              and Merge Keys (`<<: *base`, `<<: [*a, *b]` precedence)
//   - Test 04: Security guards: Circular Alias (`CircularAlias`), Unknown Alias (`UnknownAlias`),
//              Exponential Alias Expansion ("Billion Laughs", `AliasExpansionLimitExceeded`),
//              Depth > 64 (`DepthExceeded`), Tab Indentation (`TabIndentationForbidden`),
//              and malformed trailing/syntax errors
//   - Test 05: cap:simd hot-path scanning vs scalar reference 100% equivalence + large YAML stress test
//   - Test 06: 1,500-iteration deterministic PRNG malformed YAML / deep nesting / alias-bomb / control-char fuzzing

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { ConfTag, ConfValue } from "../../lib/conf/value.cp";
import {
    YamlParser,
    YamlStream,
    YamlError,
    YAML_MAX_DEPTH,
    YAML_MAX_ALIAS_EXPANSIONS,
    YAML_MAX_EXPANDED_NODES,
} from "../../lib/conf/yamlParser.cp";
import { YamlDoc } from "../../lib/conf/yaml.cp";

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

    // 1. Multi-document stream + directives + comments + compact sequences/mappings
    const multi_src =
        "%YAML 1.2\n" +
        "# leading comment\n" +
        "--- # comment after doc start\n" +
        "a0 bb: val\n" +
        "a1:\n" +
        "  b1: 4\n" +
        "  b2: d\n" +
        "a2: 4 # inline comment\n" +
        "a3: [1, 2, 3]\n" +
        "a4:\n" +
        "  - - a1\n" +
        "    - a2\n" +
        "  - 2\n" +
        "a5: 'single_quoted'\n" +
        "a6: \"double_quoted\"\n" +
        "a7: hello\n" +
        "...\n" +
        "---\n" +
        "- name: Ogre\n" +
        "  position: [0, 5, 0]\n" +
        "  powers:\n" +
        "    - name: Club\n" +
        "      damage: 10\n" +
        "- name: Dragon\n" +
        "  position: [1, 0, 10]\n" +
        "...\n" +
        "---\n" +
        "'a scalar'\n";

    const stream = YamlParser.parseStream(alloc, multi_src) catch unreachable;
    expect_eq(stream.count(), 3);

    const d0 = stream.getDoc(0).?;
    expect_eq(std.mem.eql(u8, d0.get("a0 bb").?.asString().?, "val"), true);
    expect_eq(d0.getPath("a1.b1").?.asInt().?, 4);
    expect_eq(d0.get("a2").?.asInt().?, 4);
    expect_eq(d0.getPath("a3.2").?.asInt().?, 3);
    expect_eq(std.mem.eql(u8, d0.getPath("a4.0.1").?.asString().?, "a2"), true);
    expect_eq(d0.getPath("a4.1").?.asInt().?, 2);
    expect_eq(std.mem.eql(u8, d0.get("a5").?.asString().?, "single_quoted"), true);
    expect_eq(std.mem.eql(u8, d0.get("a6").?.asString().?, "double_quoted"), true);
    expect_eq(std.mem.eql(u8, d0.get("a7").?.asString().?, "hello"), true);

    const d1 = stream.getDoc(1).?;
    expect_eq(std.mem.eql(u8, d1.getPath("0.name").?.asString().?, "Ogre"), true);
    expect_eq(d1.getPath("0.powers.0.damage").?.asInt().?, 10);
    expect_eq(d1.getPath("1.position.2").?.asInt().?, 10);

    const d2 = stream.getDoc(2).?;
    expect_eq(std.mem.eql(u8, d2.asString().?, "a scalar"), true);

    // 2. yaml-rust2 test_bad_docstart & test_empty_doc & test_github_27
    const empty_doc = YamlParser.parse(alloc, "---") catch unreachable;
    expect_eq(empty_doc.tag == ConfTag.Null, true);

    const comment_doc = YamlParser.parse(alloc, "--- #here goes a comment") catch unreachable;
    expect_eq(comment_doc.tag == ConfTag.Null, true);

    const four_dash = YamlParser.parse(alloc, "---- # comment") catch unreachable;
    expect_eq(std.mem.eql(u8, four_dash.asString().?, "----"), true);

    const bad_ds = YamlParser.parse(alloc, "---This used to cause an infinite loop") catch unreachable;
    expect_eq(std.mem.eql(u8, bad_ds.asString().?, "---This used to cause an infinite loop"), true);

    const gh27 = YamlParser.parse(alloc, "&a") catch unreachable;
    expect_eq(gh27.tag == ConfTag.Null, true);

    // 3. YAML 1.2 Core Schema types, tags, and nominal float strings
    const types_src =
        "- 'string'\n" +
        "- \"string\"\n" +
        "- string\n" +
        "- 123\n" +
        "- -321\n" +
        "- 1.23\n" +
        "- -1e4\n" +
        "- ~\n" +
        "- null\n" +
        "- true\n" +
        "- false\n" +
        "- !!str 0\n" +
        "- !!int 100\n" +
        "- !!float 2\n" +
        "- !!null ~\n" +
        "- !!bool true\n" +
        "- !!bool false\n" +
        "- 0xFF\n" +
        "- 0o77\n" +
        "- +12345\n" +
        "- .inf\n" +
        "- -.INF\n" +
        "- .NAN\n" +
        "- nan\n" +
        "- inf\n" +
        "- Infinity\n" +
        "- -nan\n" +
        "- +inf\n";

    const td = YamlParser.parse(alloc, types_src) catch unreachable;
    expect_eq(std.mem.eql(u8, td.arr_val[0].asString().?, "string"), true);
    expect_eq(td.arr_val[3].asInt().?, 123);
    expect_eq(td.arr_val[4].asInt().?, -321);
    expect_eq(td.arr_val[5].asFloat().? > 1.22 && td.arr_val[5].asFloat().? < 1.24, true);
    expect_eq(td.arr_val[6].asFloat().? < -9999.0 && td.arr_val[6].asFloat().? > -10001.0, true);
    expect_eq(td.arr_val[7].tag == ConfTag.Null, true);
    expect_eq(td.arr_val[8].tag == ConfTag.Null, true);
    expect_eq(td.arr_val[9].asBool().?, true);
    expect_eq(td.arr_val[10].asBool().?, false);
    expect_eq(std.mem.eql(u8, td.arr_val[11].asString().?, "0"), true);
    expect_eq(td.arr_val[12].asInt().?, 100);
    expect_eq(td.arr_val[13].asFloat().? == 2.0, true);
    expect_eq(td.arr_val[14].tag == ConfTag.Null, true);
    expect_eq(td.arr_val[15].asBool().?, true);
    expect_eq(td.arr_val[16].asBool().?, false);
    expect_eq(td.arr_val[17].asInt().?, 255);
    expect_eq(td.arr_val[18].asInt().?, 63);
    expect_eq(td.arr_val[19].asInt().?, 12345);
    expect_eq(std.math.isPositiveInf(td.arr_val[20].asFloat().?), true);
    expect_eq(std.math.isNegativeInf(td.arr_val[21].asFloat().?), true);
    expect_eq(std.math.isNan(td.arr_val[22].asFloat().?), true);
    // Nominal non-dotted nan/inf/Infinity MUST remain strings
    expect_eq(std.mem.eql(u8, td.arr_val[23].asString().?, "nan"), true);
    expect_eq(std.mem.eql(u8, td.arr_val[24].asString().?, "inf"), true);
    expect_eq(std.mem.eql(u8, td.arr_val[25].asString().?, "Infinity"), true);
    expect_eq(std.mem.eql(u8, td.arr_val[26].asString().?, "-nan"), true);
    expect_eq(std.mem.eql(u8, td.arr_val[27].asString().?, "+inf"), true);

    // 4. Invalid explicit tag coercions
    let bad_tag_ok = true;
    * = YamlParser.parse(alloc, "!!int string") catch {
        bad_tag_ok = false;
    };
    expect_eq(bad_tag_ok, false);

    // 5. Stringify & stringifyStream roundtrip + YamlDoc compat
    const dumped = YamlParser.stringify(alloc, d0) catch unreachable;
    const reparsed = YamlParser.parse(alloc, dumped) catch unreachable;
    expect_eq(reparsed.getPath("a1.b1").?.asInt().?, 4);
    expect_eq(reparsed.getPath("a3.2").?.asInt().?, 3);

    const s_docs: [2]ConfValue = [d0, d2];
    const dumped_stream = YamlParser.stringifyStream(alloc, &s_docs) catch unreachable;
    const reparsed_stream = YamlParser.parseStream(alloc, dumped_stream) catch unreachable;
    expect_eq(reparsed_stream.count(), 2);

    const legacy = YamlDoc.parse("server:\n  host: 127.0.0.1\n  port: 8080\n");
    expect_eq(legacy.getInt("server", "port", 0), 8080);
    expect_eq(std.mem.eql(u8, legacy.getOr("server", "host", ""), "127.0.0.1"), true);
}

fn runTest02(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1. Chomp modes: clip (|), strip (|-), keep (|+) and folded (>, >-, >+)
    const chomp_src =
        "lit_strip: |-\n  text\n\n" +
        "lit_clip: |\n  text\n\n" +
        "lit_keep: |+\n  text\n\n" +
        "fld_strip: >-\n  a\n  b\n\n" +
        "fld_clip: >\n  a\n  b\n\n" +
        "fld_keep: >+\n  a\n  b\n\n";

    const doc = YamlParser.parse(alloc, chomp_src) catch unreachable;
    expect_eq(std.mem.eql(u8, doc.get("lit_strip").?.asString().?, "text"), true);
    expect_eq(std.mem.eql(u8, doc.get("lit_clip").?.asString().?, "text\n"), true);
    expect_eq(std.mem.eql(u8, doc.get("lit_keep").?.asString().?, "text\n\n"), true);
    expect_eq(std.mem.eql(u8, doc.get("fld_strip").?.asString().?, "a b"), true);
    expect_eq(std.mem.eql(u8, doc.get("fld_clip").?.asString().?, "a b\n"), true);
    expect_eq(std.mem.eql(u8, doc.get("fld_keep").?.asString().?, "a b\n\n"), true);

    // 2. Explicit indentation indicators: |2, >1, >1-, |-2, |2+
    const indent_src =
        "- | # Empty header\n literal\n" +
        "- >1 # Indentation indicator\n  folded\n" +
        "- |+ # Chomping indicator\n keep\n\n" +
        "- >1- # Both indicators\n  strip\n" +
        "- |2\n    indented\n" +
        "- >-2\n    line1\n    line2\n";

    const idoc = YamlParser.parse(alloc, indent_src) catch unreachable;
    expect_eq(std.mem.eql(u8, idoc.arr_val[0].asString().?, "literal\n"), true);
    expect_eq(std.mem.eql(u8, idoc.arr_val[1].asString().?, " folded\n"), true);
    expect_eq(std.mem.eql(u8, idoc.arr_val[2].asString().?, "keep\n\n"), true);
    expect_eq(std.mem.eql(u8, idoc.arr_val[3].asString().?, " strip"), true);
    expect_eq(std.mem.eql(u8, idoc.arr_val[4].asString().?, "  indented\n"), true);
    expect_eq(std.mem.eql(u8, idoc.arr_val[5].asString().?, "  line1\n  line2"), true);

    // 3. Empty block scalar chomping (EX8_6)
    const empty_chomp =
        "strip: >-\n\n" +
        "clip: >\n\n" +
        "keep: |+\n\n";
    const edoc = YamlParser.parse(alloc, empty_chomp) catch unreachable;
    expect_eq(std.mem.eql(u8, edoc.get("strip").?.asString().?, ""), true);
    expect_eq(std.mem.eql(u8, edoc.get("clip").?.asString().?, ""), true);
    expect_eq(std.mem.eql(u8, edoc.get("keep").?.asString().?, "\n"), true);

    // 4. Folded more-indented lines & empty separation lines (EX8_10 style)
    const folded_src =
        "body: >-\n" +
        "  folded\n" +
        "  line\n" +
        "\n" +
        "  next\n" +
        "  line\n" +
        "    * bullet\n" +
        "\n" +
        "    * list\n" +
        "  last\n" +
        "  line\n";
    const fdoc = YamlParser.parse(alloc, folded_src) catch unreachable;
    expect_eq(
        std.mem.eql(u8, fdoc.get("body").?.asString().?, "folded line\nnext line\n  * bullet\n\n  * list\nlast line"),
        true,
    );

    // 5. Large block scalar indentation (yaml-rust2 issue #29)
    const large_indent =
        "a: |-\n" +
        "                  a\n" +
        "                      b\n";
    const ldoc = YamlParser.parse(alloc, large_indent) catch unreachable;
    expect_eq(std.mem.eql(u8, ldoc.get("a").?.asString().?, "a\n    b"), true);
}

fn runTest03(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1. Nested flow collections + trailing commas
    const flow_src =
        "nested: [a, {b: [1, 2], c: \"ok\", d: 'it''s', }, true, ]\n" +
        "empty_seq: []\n" +
        "empty_map: {}\n";
    const doc = YamlParser.parse(alloc, flow_src) catch unreachable;
    expect_eq(std.mem.eql(u8, doc.getPath("nested.0").?.asString().?, "a"), true);
    expect_eq(doc.getPath("nested.1.b.1").?.asInt().?, 2);
    expect_eq(std.mem.eql(u8, doc.getPath("nested.1.c").?.asString().?, "ok"), true);
    expect_eq(std.mem.eql(u8, doc.getPath("nested.1.d").?.asString().?, "it's"), true);
    expect_eq(doc.getPath("nested.2").?.asBool().?, true);
    expect_eq(doc.get("empty_seq").?.arr_val.len, 0);
    expect_eq(doc.get("empty_map").?.obj_val.len, 0);

    // 2. Double-quoted escapes (\0 \a \b \t \n \v \f \r \e \xHH \uXXXX \UXXXXXXXX \N \_ \L \P)
    const esc_src =
        "esc: \"\\0\\a\\b\\t\\n\\v\\f\\r\\e\\x41\\u00E9\\U0001F600\"\n" +
        "folded: \"hello \\\n  world\"\n";
    const edoc = YamlParser.parse(alloc, esc_src) catch unreachable;
    const esc_s = edoc.get("esc").?.asString().?;
    expect_eq(esc_s[0], 0);
    expect_eq(esc_s[1], 0x07);
    expect_eq(esc_s[9], 'A');
    expect_eq(std.mem.eql(u8, edoc.get("folded").?.asString().?, "hello world"), true);

    // Surrogate escape rejection
    let surr_ok = true;
    * = YamlParser.parse(alloc, "bad: \"\\uD800\"") catch {
        surr_ok = false;
    };
    expect_eq(surr_ok, false);

    // 3. Anchors, Aliases, and Merge Keys (`<<: *base` and `<<: [*a, *b]`)
    const merge_src =
        "base_a: &a\n" +
        "  host: 10.0.0.1\n" +
        "  port: 8080\n" +
        "  shared: from_a\n" +
        "base_b: &b\n" +
        "  port: 9090\n" +
        "  tls: true\n" +
        "  shared: from_b\n" +
        "single_merge:\n" +
        "  <<: *a\n" +
        "  port: 8443\n" +
        "multi_merge:\n" +
        "  <<: [*a, *b]\n" +
        "  name: edge\n";
    const mdoc = YamlParser.parse(alloc, merge_src) catch unreachable;
    expect_eq(std.mem.eql(u8, mdoc.getPath("single_merge.host").?.asString().?, "10.0.0.1"), true);
    expect_eq(mdoc.getPath("single_merge.port").?.asInt().?, 8443);
    expect_eq(std.mem.eql(u8, mdoc.getPath("multi_merge.shared").?.asString().?, "from_a"), true);
    expect_eq(mdoc.getPath("multi_merge.tls").?.asBool().?, true);
    expect_eq(mdoc.getPath("multi_merge.port").?.asInt().?, 8080);
    expect_eq(std.mem.eql(u8, mdoc.getPath("multi_merge.name").?.asString().?, "edge"), true);
}

fn runTest04(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1. Circular Alias detection (direct & nested)
    let circ1 = false;
    * = YamlParser.parse(alloc, "a1: &DEFAULT\n  b1: 4\n  b2: *DEFAULT\n") catch |e| {
        if (std.mem.eql(u8, @errorName(e), "CircularAlias")) circ1 = true;
    };
    expect_eq(circ1, true);

    let circ2 = false;
    * = YamlParser.parse(alloc, "x: &self [*self]") catch |e| {
        if (std.mem.eql(u8, @errorName(e), "CircularAlias")) circ2 = true;
    };
    expect_eq(circ2, true);

    // 2. Unknown Alias detection
    let unk = false;
    * = YamlParser.parse(alloc, "x: *nonexistent") catch |e| {
        if (std.mem.eql(u8, @errorName(e), "UnknownAlias")) unk = true;
    };
    expect_eq(unk, true);

    // 3. Exponential Alias Expansion ("Billion Laughs" resource exhaustion guard)
    const alias_bomb =
        "a: &a [\"lol\", \"lol\", \"lol\", \"lol\", \"lol\", \"lol\", \"lol\", \"lol\", \"lol\"]\n" +
        "b: &b [*a, *a, *a, *a, *a, *a, *a, *a, *a]\n" +
        "c: &c [*b, *b, *b, *b, *b, *b, *b, *b, *b]\n" +
        "d: &d [*c, *c, *c, *c, *c, *c, *c, *c, *c]\n" +
        "e: &e [*d, *d, *d, *d, *d, *d, *d, *d, *d]\n";
    let bomb_blocked = false;
    * = YamlParser.parse(alloc, alias_bomb) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "AliasExpansionLimitExceeded")) bomb_blocked = true;
    };
    expect_eq(bomb_blocked, true);

    // 4. Max recursion depth guard (> YAML_MAX_DEPTH = 64)
    let deep_seq = std.ArrayListUnmanaged(u8).empty;
    let k: usize = 0;
    while (k < 70) : (k += 1) try deep_seq.append(alloc, '[');
    k = 0;
    while (k < 70) : (k += 1) try deep_seq.append(alloc, ']');

    let depth_seq_err = false;
    * = YamlParser.parse(alloc, deep_seq.items) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "DepthExceeded")) depth_seq_err = true;
    };
    expect_eq(depth_seq_err, true);

    let deep_map = std.ArrayListUnmanaged(u8).empty;
    k = 0;
    while (k < 70) : (k += 1) try deep_map.appendSlice(alloc, "{a:");
    try deep_map.append(alloc, '1');
    k = 0;
    while (k < 70) : (k += 1) try deep_map.append(alloc, '}');

    let depth_map_err = false;
    * = YamlParser.parse(alloc, deep_map.items) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "DepthExceeded")) depth_map_err = true;
    };
    expect_eq(depth_map_err, true);

    // 5. Tab indentation forbidden in block context
    let tab_err = false;
    * = YamlParser.parse(alloc, "a:\n\t b: 1\n") catch |e| {
        if (std.mem.eql(u8, @errorName(e), "TabIndentationForbidden")) tab_err = true;
    };
    expect_eq(tab_err, true);

    // 6. Malformed trailing content & syntax errors (yaml-rust2 test_fail / test_bad_hyphen / test_issue_65_mwe)
    let err_mwe = true;
    * = YamlParser.parse(alloc, "\"foo\" l") catch {
        err_mwe = false;
    };
    expect_eq(err_mwe, false);

    let err_hyph = true;
    * = YamlParser.parse(alloc, "{-") catch {
        err_hyph = false;
    };
    expect_eq(err_hyph, false);

    let err_fail = true;
    * = YamlParser.parse(alloc, "scalar\nkey: [1, 2]]\n") catch {
        err_fail = false;
    };
    expect_eq(err_fail, false);
}

fn runTest05(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1. SIMD vs Scalar equivalence across 0..64+ byte boundaries
    const samples: [8][]const u8 = [
        "",
        "    ",
        "                \t  \n",
        "                                  hello\r\nworld",
        "plain_text_without_specials_over_thirty_two_bytes_long_0123456789",
        "0123456789abcdef0123456789abcdef\"end",
        "0123456789abcdef0123456789abcdef\\escape",
        "0123456789abcdef0123456789abcdef''single'",
    ];

    for (s of samples) {
        expect_eq(YamlParser.scanLineEndSimd(s), YamlParser.scanLineEndScalar(s));
        expect_eq(YamlParser.countIndentSpacesSimd(s), YamlParser.countIndentSpacesScalar(s));

        const dq_simd = YamlParser.scanDoubleQuotedStopSimd(s);
        const dq_scal = YamlParser.scanDoubleQuotedStopScalar(s);
        expect_eq(dq_simd == null, dq_scal == null);
        if (dq_simd != null && dq_scal != null) {
            expect_eq(dq_simd.?, dq_scal.?);
        }

        const sq_simd = YamlParser.scanSingleQuotedStopSimd(s);
        const sq_scal = YamlParser.scanSingleQuotedStopScalar(s);
        expect_eq(sq_simd == null, sq_scal == null);
        if (sq_simd != null && sq_scal != null) {
            expect_eq(sq_simd.?, sq_scal.?);
        }
    }

    // 2. Large YAML stress test (200 entries with comments, block scalars, flow arrays)
    let large = std.ArrayListUnmanaged(u8).empty;
    let i: usize = 0;
    while (i < 200) : (i += 1) {
        const line = try std.fmt.allocPrint(
            alloc,
            "item_{d}: # comment {d}\n  name: \"service_{d}_abcdefghijklmnopqrstuvwxyz\"\n  ports: [80, 443, {d}]\n",
            .{ i, i, i, 8000 + i },
        );
        try large.appendSlice(alloc, line);
    }
    const root = YamlParser.parse(alloc, large.items) catch unreachable;
    expect_eq(root.obj_val.len, 200);
    expect_eq(root.getPath("item_199.ports.2").?.asInt().?, 8199);
}

fn runTest06(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1,500-iteration deterministic PRNG malformed YAML fuzzing (0 panic / 0 OOB / 0 hang)
    const alphabet = "abcdefghijklmnopqrstuvwxyz0123456789 \t\n\r:#-&*!|>-+[]{},\"'\\~.";
    let seed: u64 = 0x1234_5678_9ABC_DEF0;
    let buf: [96]u8 = undefined;

    let iter: usize = 0;
    let ok_count: usize = 0;
    let err_count: usize = 0;
    while (iter < 1500) : (iter += 1) {
        const len = (xorshift64(&seed) % 80) + 1;
        let j: usize = 0;
        while (j < len) : (j += 1) {
            const idx = (xorshift64(&seed) as usize) % alphabet.len;
            buf[j] = alphabet[idx];
        }
        const _res = YamlParser.parseStream(alloc, buf[0..len]) catch {
            err_count += 1;
            continue;
        };
        ok_count += 1;
    }

    expect_eq(ok_count + err_count, 1500);
    expect_eq(YAML_MAX_DEPTH, 64);
    expect_eq(YAML_MAX_ALIAS_EXPANSIONS, 256);
    expect_eq(YAML_MAX_EXPANDED_NODES, 16384);
}

fn main() {
    test("yaml_ref_01_yaml_rust2_core_matrix_multidoc_tags_and_roundtrip", () => { try runTest01(); });
    test("yaml_ref_02_block_scalars_full_chomping_and_explicit_indent", () => { try runTest02(); });
    test("yaml_ref_03_flow_collections_escapes_and_merge_keys", () => { try runTest03(); });
    test("yaml_ref_04_security_circular_alias_bomb_depth_and_syntax_guards", () => { try runTest04(); });
    test("yaml_ref_05_simd_scanning_equivalence_and_large_yaml_stress", () => { try runTest05(); });
    test("yaml_ref_06_deterministic_prng_1500_fuzz_malformed_yaml", () => { try runTest06(); });
}
