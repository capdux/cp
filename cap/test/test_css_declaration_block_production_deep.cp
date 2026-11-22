// cap/test/test_css_declaration_block_production_deep.cp
// 深度生产级回归测试：cap:css 声明块解析、去重压缩与序列化 (lib/css/declaration_block.cp)

import { test } from "cap:test";
import {
    CSS_DECL_MAX_IMPORTANT,
    CssDeclParseOptions,
    CssDeclarationBlock,
} from "cap:css";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

test("declaration_block: parses normal and !important buckets with quotes, parens and comments") {
    const raw = "{ /* header */ color: #111; background: url('a;b:c.png') no-repeat; content: \"x;y\"; font-weight: 700 ! IMPORTANT ; }";
    const blk = CssDeclarationBlock.parse(raw, CssDeclParseOptions{})?;

    if (blk.len() != 4 || blk.normal_count != 3 || blk.important_count != 1) {
        return err.BucketCountMismatch;
    }

    const norms = blk.normalSlice();
    if (!bytesEq(norms[0].name, "color") || !bytesEq(norms[0].value, "#111")) {
        return err.FirstDeclMismatch;
    }
    if (!bytesEq(norms[1].name, "background") || !bytesEq(norms[1].value, "url('a;b:c.png') no-repeat")) {
        return err.UrlWithSemicolonCorrupted;
    }
    if (!bytesEq(norms[2].name, "content") || !bytesEq(norms[2].value, "\"x;y\"")) {
        return err.QuotedSemicolonCorrupted;
    }

    const imps = blk.importantSlice();
    if (!bytesEq(imps[0].name, "font-weight") || !bytesEq(imps[0].value, "700")) {
        return err.ImportantDeclMismatch;
    }
}

test("declaration_block: cascade-safe minifyDeduplicate and getEffective priority") {
    let blk = CssDeclarationBlock.parse(
        "color: red; margin: 4px; Color: green; margin: 8px !important; margin: 12px !important; padding: 2px;",
        CssDeclParseOptions{},
    )?;

    // 去重前 getEffective 已遵守 !important > normal 与 last-wins
    if (eff_c := blk.getEffective("color")) {
        if (!bytesEq(eff_c.value, "green")) { return err.EffectiveColorPreMismatch; }
    } else { return err.EffectiveColorPreMissing; }
    if (eff_m := blk.getEffective("margin")) {
        if (!bytesEq(eff_m.value, "12px")) { return err.EffectiveMarginPreMismatch; }
    } else { return err.EffectiveMarginPreMissing; }

    // 应消除 3 条冗余声明：color: red, margin: 4px (被 !important 覆盖), margin: 8px !important
    const removed = blk.minifyDeduplicate();
    if (removed != 3 || blk.normal_count != 2 || blk.important_count != 1) {
        return err.DeduplicateCountMismatch;
    }

    let out: [256]u8 = [0; 256];
    const n = blk.toCss(out[0..], true)?;
    if (!bytesEq(out[0..n], "Color:green;padding:2px;margin:12px!important")) {
        return err.DeduplicatedCssMismatch;
    }
}

test("declaration_block: custom properties are case-sensitive while standard properties are case-insensitive") {
    let blk = CssDeclarationBlock.parse(
        "--Theme: dark; --theme: light; DISPLAY: block; display: flex;",
        CssDeclParseOptions{},
    )?;
    const removed = blk.minifyDeduplicate();
    // 仅 DISPLAY: block 被 display: flex 消除；--Theme 与 --theme 均保留
    if (removed != 1 || blk.normal_count != 3) {
        return err.CustomPropCaseMismatch;
    }
    if (v1 := blk.getEffective("--Theme")) {
        if (!bytesEq(v1.value, "dark") || !v1.is_custom) { return err.UpperCustomPropMismatch; }
    } else { return err.UpperCustomPropMissing; }
    if (v2 := blk.getEffective("--theme")) {
        if (!bytesEq(v2.value, "light") || !v2.is_custom) { return err.LowerCustomPropMismatch; }
    } else { return err.LowerCustomPropMissing; }
}

test("declaration_block: minified and pretty-printed toCssBlock round-trip") {
    const src = "color: #fff; background: #000 !important";
    const blk = CssDeclarationBlock.parse(src, CssDeclParseOptions{})?;

    let min_buf: [128]u8 = [0; 128];
    const mn = blk.toCssBlock(min_buf[0..], true)?;
    if (!bytesEq(min_buf[0..mn], "{color:#fff;background:#000!important}")) {
        return err.MinifiedBlockMismatch;
    }

    let pretty_buf: [256]u8 = [0; 256];
    const pn = blk.toCssBlock(pretty_buf[0..], false)?;
    const expected_pretty = "{\n  color: #fff;\n  background: #000 !important;\n}";
    if (!bytesEq(pretty_buf[0..pn], expected_pretty)) {
        return err.PrettyBlockMismatch;
    }

    // Round-trip: 将序列化产物再次解析并验证语义完全等价
    const reparsed = CssDeclarationBlock.parse(pretty_buf[0..pn], CssDeclParseOptions{})?;
    if (reparsed.len() != blk.len() || reparsed.propertyIdHash() != blk.propertyIdHash()) {
        return err.RoundTripMismatch;
    }
}

test("declaration_block: propertyIdHash distinguishes normal vs important and normalizes standard case") {
    const a = CssDeclarationBlock.parse("COLOR: red; margin: 0 !important;", CssDeclParseOptions{})?;
    const b = CssDeclarationBlock.parse("color: blue; MARGIN: 4px !important;", CssDeclParseOptions{})?;
    const c = CssDeclarationBlock.parse("color: blue; margin: 4px;", CssDeclParseOptions{})?;

    // 相同属性名序列与相同 !important 分桶 -> 哈希一致（与值无关）
    if (a.propertyIdHash() != b.propertyIdHash()) {
        return err.SamePropIdsHashMismatch;
    }
    // 普通桶 vs !important 桶不同 -> 哈希不同
    if (b.propertyIdHash() == c.propertyIdHash()) {
        return err.DifferentBucketsHashCollision;
    }
}

test("declaration_block: error_recovery skips malformed declarations while strict mode rejects them") {
    const messy = "color: red; broken_without_colon; : missing_name; empty_val: ; display: grid;";

    // 严格模式拒绝缺少冒号的畸形声明
    let strict_failed = false;
    const dummy = CssDeclarationBlock.parse(messy, CssDeclParseOptions{ error_recovery: false }) catch blk: {
        strict_failed = true;
        break :blk CssDeclarationBlock.init();
    };
    * = dummy;
    if (!strict_failed) { return err.StrictModeShouldReject; }

    // 容错恢复模式跳过 3 条畸形声明并保留 2 条有效声明
    const recovered = CssDeclarationBlock.parse(messy, CssDeclParseOptions{ error_recovery: true })?;
    if (recovered.recovered_errors != 3 || recovered.normal_count != 2) {
        return err.RecoveryCountMismatch;
    }
    if (eff := recovered.getEffective("display")) {
        if (!bytesEq(eff.value, "grid")) { return err.RecoveredValueMismatch; }
    } else { return err.RecoveredDisplayMissing; }
}

test("declaration_block: empty input and empty block serialization") {
    const empty = CssDeclarationBlock.parse("  { ; ; /* only comment */ }  ", CssDeclParseOptions{})?;
    if (!empty.isEmpty() || empty.len() != 0) { return err.EmptyBlockNotEmpty; }

    let buf: [32]u8 = [0; 32];
    const n = empty.toCssBlock(buf[0..], true)?;
    if (!bytesEq(buf[0..n], "{}")) { return err.EmptyBlockCssMismatch; }
}

test("declaration_block: unclosed string/comment, capacity overflow and tiny buffer guards") {
    // 未闭合字符串拦截
    let unclosed_str = false;
    const d1 = CssDeclarationBlock.parse("content: \"unterminated;", CssDeclParseOptions{ error_recovery: true }) catch blk: {
        unclosed_str = true;
        break :blk CssDeclarationBlock.init();
    };
    * = d1;
    if (!unclosed_str) { return err.UnclosedStringNotRejected; }

    // 未闭合注释拦截
    let unclosed_cmt = false;
    const d2 = CssDeclarationBlock.parse("/* oops color: red;", CssDeclParseOptions{ error_recovery: true }) catch blk: {
        unclosed_cmt = true;
        break :blk CssDeclarationBlock.init();
    };
    * = d2;
    if (!unclosed_cmt) { return err.UnclosedCommentNotRejected; }

    // !important 容量超限拦截
    let full = CssDeclarationBlock.init();
    let i: usize = 0;
    while (i < CSS_DECL_MAX_IMPORTANT) {
        * = full.push("color", "red", true)?;
        i += 1;
    }
    let imp_overflow = false;
    full.push("color", "blue", true) catch {
        imp_overflow = true;
    };
    if (!imp_overflow) { return err.ImportantOverflowNotRejected; }

    // 序列化缓冲区过小拦截 BufferTooSmall
    let tiny: [4]u8 = [0; 4];
    let buf_small = false;
    full.toCssBlock(tiny[0..], true) catch {
        buf_small = true;
    };
    if (!buf_small) { return err.BufferTooSmallNotRejected; }
}
