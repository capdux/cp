import std from "std";
import { test, expect_eq } from "cap:test";
import {
    CssOrderError,
    CssOrderLoaderKind,
    CssOrderGraph,
} from "cap:bundler";

test("1. Diamond dependency graph produces exact JS DFS postorder [D, B, C, A]", () => {
    const alloc = std.testing.allocator;
    let g = CssOrderGraph.init(alloc);
    defer g.deinit();

    const a = g.addFile(CssOrderLoaderKind.js) catch unreachable;
    const b = g.addFile(CssOrderLoaderKind.css) catch unreachable;
    const c = g.addFile(CssOrderLoaderKind.css) catch unreachable;
    const d = g.addFile(CssOrderLoaderKind.css) catch unreachable;

    g.addImport(a, b) catch unreachable;
    g.addImport(a, c) catch unreachable;
    g.addImport(b, d) catch unreachable;
    g.addImport(c, d) catch unreachable;

    const all = g.findAllInJsPostorder(alloc, a) catch unreachable;
    defer alloc.free(all);
    expect_eq(all.len, usize(4));
    expect_eq(all[0], d);
    expect_eq(all[1], b);
    expect_eq(all[2], c);
    expect_eq(all[3], a);

    const css_only = g.findImportedCssInJsOrder(alloc, a) catch unreachable;
    defer alloc.free(css_only);
    expect_eq(css_only.len, usize(3));
    expect_eq(css_only[0], d);
    expect_eq(css_only[1], b);
    expect_eq(css_only[2], c);
});

test("2. Mixed JS, CSS, and asset loaders filter only CSS in exact cascade order", () => {
    const alloc = std.testing.allocator;
    let g = CssOrderGraph.init(alloc);
    defer g.deinit();

    const entry = g.addFile(CssOrderLoaderKind.js) catch unreachable;
    const btn_js = g.addFile(CssOrderLoaderKind.js) catch unreachable;
    const base_css = g.addFile(CssOrderLoaderKind.css) catch unreachable;
    const btn_css = g.addFile(CssOrderLoaderKind.css) catch unreachable;
    const icon_svg = g.addFile(CssOrderLoaderKind.other) catch unreachable;
    const theme_css = g.addFile(CssOrderLoaderKind.css) catch unreachable;

    expect_eq(CssOrderLoaderKind.css.isCss(), true);
    expect_eq(CssOrderLoaderKind.js.isCss(), false);
    expect_eq(CssOrderLoaderKind.other.isCss(), false);

    g.addImport(entry, btn_js) catch unreachable;
    g.addImport(entry, icon_svg) catch unreachable;
    g.addImport(entry, theme_css) catch unreachable;
    g.addImport(btn_js, base_css) catch unreachable;
    g.addImport(btn_js, btn_css) catch unreachable;

    const order = g.findImportedCssInJsOrder(alloc, entry) catch unreachable;
    defer alloc.free(order);

    expect_eq(order.len, usize(3));
    expect_eq(order[0], base_css);
    expect_eq(order[1], btn_css);
    expect_eq(order[2], theme_css);
});

test("3. Cyclic JS and CSS imports terminate cleanly without duplicates", () => {
    const alloc = std.testing.allocator;
    let g = CssOrderGraph.init(alloc);
    defer g.deinit();

    const a = g.addFile(CssOrderLoaderKind.js) catch unreachable;
    const b = g.addFile(CssOrderLoaderKind.js) catch unreachable;
    const c1 = g.addFile(CssOrderLoaderKind.css) catch unreachable;
    const c2 = g.addFile(CssOrderLoaderKind.css) catch unreachable;

    g.addImport(a, b) catch unreachable;
    g.addImport(b, a) catch unreachable; // JS 环
    g.addImport(b, c1) catch unreachable;
    g.addImport(c1, c2) catch unreachable;
    g.addImport(c2, c1) catch unreachable; // CSS 环

    const order = g.findImportedCssInJsOrder(alloc, a) catch unreachable;
    defer alloc.free(order);

    expect_eq(order.len, usize(2));
    expect_eq(order[0], c2);
    expect_eq(order[1], c1);
});

test("4. findImportedCssForEntries deduplicates shared CSS across multiple entry points", () => {
    const alloc = std.testing.allocator;
    let g = CssOrderGraph.init(alloc);
    defer g.deinit();

    const e1 = g.addFile(CssOrderLoaderKind.js) catch unreachable;
    const e2 = g.addFile(CssOrderLoaderKind.js) catch unreachable;
    const shared_css = g.addFile(CssOrderLoaderKind.css) catch unreachable;
    const page1_css = g.addFile(CssOrderLoaderKind.css) catch unreachable;
    const page2_css = g.addFile(CssOrderLoaderKind.css) catch unreachable;

    g.addImport(e1, shared_css) catch unreachable;
    g.addImport(e1, page1_css) catch unreachable;
    g.addImport(e2, shared_css) catch unreachable;
    g.addImport(e2, page2_css) catch unreachable;

    const entries: [2]u32 = [e1, e2];
    const order = g.findImportedCssForEntries(alloc, entries[0..]) catch unreachable;
    defer alloc.free(order);

    expect_eq(order.len, usize(3));
    expect_eq(order[0], shared_css);
    expect_eq(order[1], page1_css);
    expect_eq(order[2], page2_css);
});

test("5. Deep 1000-node import chain runs iteratively without stack overflow", () => {
    const alloc = std.testing.allocator;
    let g = CssOrderGraph.init(alloc);
    defer g.deinit();

    const total: u32 = 1000;
    let i: u32 = 0;
    while (i < total) {
        const loader = if (i == total - 1) CssOrderLoaderKind.css else CssOrderLoaderKind.js;
        * = g.addFile(loader) catch unreachable;
        i += 1;
    }
    let e: u32 = 0;
    while (e + 1 < total) {
        g.addImport(e, e + 1) catch unreachable;
        e += 1;
    }

    expect_eq(g.fileCount(), usize(1000));
    const order = g.findImportedCssInJsOrder(alloc, 0) catch unreachable;
    defer alloc.free(order);
    expect_eq(order.len, usize(1));
    expect_eq(order[0], u32(999));
});

test("6. Out-of-range source indices return SourceIndexOutOfRange cleanly", () => {
    const alloc = std.testing.allocator;
    let g = CssOrderGraph.init(alloc);
    defer g.deinit();

    const a = g.addFile(CssOrderLoaderKind.js) catch unreachable;

    let e1 = false;
    g.addImport(a, 5) catch |err_val| {
        if (err_val == CssOrderError.SourceIndexOutOfRange) e1 = true;
    };
    expect_eq(e1, true);

    let e2 = false;
    * = g.findImportedCssInJsOrder(alloc, 9) catch |err_val| {
        if (err_val == CssOrderError.SourceIndexOutOfRange) e2 = true;
    };
    expect_eq(e2, true);

    const bad_entries: [2]u32 = [0, 42];
    let e3 = false;
    * = g.findImportedCssForEntries(alloc, bad_entries[0..]) catch |err_val| {
        if (err_val == CssOrderError.SourceIndexOutOfRange) e3 = true;
    };
    expect_eq(e3, true);
});
