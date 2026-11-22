// cap/test/test_css_logical_production_deep.cp
// Production-deep regression tests for cap:css logical module
// Source: /data/dev/bun/src/css/logical.cp & /data/dev/bun/docs/bundler/css.mdx

import { test, expect } from "cap:test";
import {
    CssLogicalError,
    LogicalPropertyCategory,
    LogicalPropertyGroup,
    WritingDirection,
    classifyLogicalProperty,
    logicalGroupOf,
    mapLogicalToPhysical,
    formatLogicalFallbackRule,
} from "cap:css";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("css logical: all 12 LogicalPropertyGroup classifications & physical contrast") {
    expect(classifyLogicalProperty("margin-inline-start") == LogicalPropertyCategory.logical);
    expect(classifyLogicalProperty("MARGIN-BLOCK-END") == LogicalPropertyCategory.logical);
    expect(classifyLogicalProperty("margin-left") == LogicalPropertyCategory.physical);
    expect(classifyLogicalProperty("color") == LogicalPropertyCategory.physical);
    expect(LogicalPropertyCategory.default() == LogicalPropertyCategory.physical);

    expect(logicalGroupOf("border-inline-start-color").? == LogicalPropertyGroup.border_color);
    expect(logicalGroupOf("border-block-end-style").? == LogicalPropertyGroup.border_style);
    expect(logicalGroupOf("border-inline-width").? == LogicalPropertyGroup.border_width);
    expect(logicalGroupOf("border-start-start-radius").? == LogicalPropertyGroup.border_radius);
    expect(logicalGroupOf("margin-inline").? == LogicalPropertyGroup.margin);
    expect(logicalGroupOf("scroll-margin-inline-end").? == LogicalPropertyGroup.scroll_margin);
    expect(logicalGroupOf("padding-block").? == LogicalPropertyGroup.padding);
    expect(logicalGroupOf("scroll-padding-block-start").? == LogicalPropertyGroup.scroll_padding);
    expect(logicalGroupOf("inset-inline-start").? == LogicalPropertyGroup.inset);
    expect(logicalGroupOf("inline-size").? == LogicalPropertyGroup.size);
    expect(logicalGroupOf("min-block-size").? == LogicalPropertyGroup.min_size);
    expect(logicalGroupOf("max-inline-size").? == LogicalPropertyGroup.max_size);
    expect(logicalGroupOf("width") == null);
}

test("css logical: LTR vs RTL physical property mapping across corners, inset & sizing") {
    const mis = mapLogicalToPhysical("margin-inline-start") catch { expect(false); return; };
    expect(mis.isDirectionDependent());
    expect(!mis.is_shorthand_pair);
    expect(strEq(mis.physicalFor(WritingDirection.ltr), "margin-left"));
    expect(strEq(mis.physicalFor(WritingDirection.rtl), "margin-right"));

    const corner = mapLogicalToPhysical("border-end-start-radius") catch { expect(false); return; };
    expect(corner.isDirectionDependent());
    expect(strEq(corner.physicalFor(WritingDirection.ltr), "border-bottom-left-radius"));
    expect(strEq(corner.physicalFor(WritingDirection.rtl), "border-bottom-right-radius"));

    const bs = mapLogicalToPhysical("block-size") catch { expect(false); return; };
    expect(!bs.isDirectionDependent());
    expect(strEq(bs.physicalFor(WritingDirection.ltr), "height"));
    expect(strEq(bs.physicalFor(WritingDirection.rtl), "height"));

    const ins = mapLogicalToPhysical("inset-inline") catch { expect(false); return; };
    expect(ins.is_shorthand_pair);
    expect(ins.isDirectionDependent());
    expect(strEq(ins.physicalFor(WritingDirection.ltr), "left"));
    expect(strEq(ins.secondPhysicalFor(WritingDirection.ltr), "right"));
    expect(strEq(ins.physicalFor(WritingDirection.rtl), "right"));
    expect(strEq(ins.secondPhysicalFor(WritingDirection.rtl), "left"));
}

test("css logical: direction-independent & symmetric shorthand fallback rules") {
    let buf: [256]u8 = [0; 256];

    const n1 = formatLogicalFallbackRule(".box", "block-size", "120px", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n1], ".box { height: 120px; }"));

    const n2 = formatLogicalFallbackRule(".box", "padding-block", "8px 12px", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n2], ".box { padding-top: 8px; padding-bottom: 12px; }"));

    // Symmetric inline shorthand emits single rule without :dir() duplication
    const n3 = formatLogicalFallbackRule(".box", "margin-inline", "auto", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n3], ".box { margin-left: auto; margin-right: auto; }"));

    // Value containing calc(...) spaces must not split inside parentheses
    const n4 = formatLogicalFallbackRule(".box", "margin-inline", "calc(100% - 16px)", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n4], ".box { margin-left: calc(100% - 16px); margin-right: calc(100% - 16px); }"));
}

test("css logical: direction-dependent :dir(ltr) / :dir(rtl) fallback generation") {
    let buf: [256]u8 = [0; 256];

    const n1 = formatLogicalFallbackRule(".card", "margin-inline-start", "16px", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n1], ":dir(ltr) .card { margin-left: 16px; } :dir(rtl) .card { margin-right: 16px; }"));

    // Asymmetric inline shorthand flips start/end in RTL
    const n2 = formatLogicalFallbackRule(".card", "padding-inline", "4px 20px", buf[0..]) catch { expect(false); return; };
    expect(strEq(
        buf[0..n2],
        ":dir(ltr) .card { padding-left: 4px; padding-right: 20px; } :dir(rtl) .card { padding-right: 4px; padding-left: 20px; }",
    ));

    // Logical border radius corner
    const n3 = formatLogicalFallbackRule(".btn", "border-start-end-radius", "6px", buf[0..]) catch { expect(false); return; };
    expect(strEq(
        buf[0..n3],
        ":dir(ltr) .btn { border-top-right-radius: 6px; } :dir(rtl) .btn { border-top-left-radius: 6px; }",
    ));
}

test("css logical: scroll-margin, scroll-padding & border-*-style/width/color coverage") {
    let buf: [256]u8 = [0; 256];

    const n1 = formatLogicalFallbackRule(".item", "scroll-margin-block-start", "24px", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n1], ".item { scroll-margin-top: 24px; }"));

    const n2 = formatLogicalFallbackRule(".item", "scroll-padding-inline-end", "12px", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n2], ":dir(ltr) .item { scroll-padding-right: 12px; } :dir(rtl) .item { scroll-padding-left: 12px; }"));

    const n3 = formatLogicalFallbackRule(".item", "border-block-color", "red blue", buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n3], ".item { border-top-color: red; border-bottom-color: blue; }"));
}

test("css logical: malformed inputs & BufferTooSmall defenses") {
    let e_empty = false;
    * = mapLogicalToPhysical("   ") catch |e| {
        expect(e == CssLogicalError.EmptyProperty);
        e_empty = true;
    };
    expect(e_empty);

    let e_not_logical = false;
    * = mapLogicalToPhysical("background-color") catch |e| {
        expect(e == CssLogicalError.NotLogicalProperty);
        e_not_logical = true;
    };
    expect(e_not_logical);

    let buf: [64]u8 = [0; 64];
    let e_sel = false;
    * = formatLogicalFallbackRule("", "inline-size", "10px", buf[0..]) catch |e| {
        expect(e == CssLogicalError.EmptySelector);
        e_sel = true;
    };
    expect(e_sel);

    let e_val = false;
    * = formatLogicalFallbackRule(".a", "inline-size", "  ", buf[0..]) catch |e| {
        expect(e == CssLogicalError.EmptyValue);
        e_val = true;
    };
    expect(e_val);

    let tiny: [10]u8 = [0; 10];
    let e_small = false;
    * = formatLogicalFallbackRule(".card", "margin-inline-start", "16px", tiny[0..]) catch |e| {
        expect(e == CssLogicalError.BufferTooSmall);
        e_small = true;
    };
    expect(e_small);
}
