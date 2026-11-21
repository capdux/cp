import { test, expect } from "cap:test";
import { SupportsTree } from "cap:css";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn mockDecl(prop: []const u8, val: []const u8): bool {
    if (strEq(prop, "display") && (strEq(val, "grid") || strEq(val, "flex"))) return true;
    if (strEq(prop, "color") && strEq(val, "oklch(0.5 0.2 240)")) return true;
    return false;
}

fn mockSel(sel: []const u8): bool {
    return strEq(sel, ":has(a)");
}

test("css_supports: parse, serialize and evaluate logical combinations", () => {
    let buf: [256]u8 = [0; 256];

    const t1 = SupportsTree.parse("(display: grid) and (not (display: table))");
    expect(t1 != null);
    expect(strEq(t1.?.toCss(buf[0..]), "(display: grid) and (not (display: table))"));
    expect(t1.?.eval(mockDecl, mockSel));

    const t2 = SupportsTree.parse("selector(:has(a)) or (display: subgrid)");
    expect(t2 != null);
    expect(strEq(t2.?.toCss(buf[0..]), "selector(:has(a)) or (display: subgrid)"));
    expect(t2.?.eval(mockDecl, mockSel));

    const t3 = SupportsTree.parse("not ((display: grid) and (display: table))");
    expect(t3 != null);
    expect(strEq(t3.?.toCss(buf[0..]), "not ((display: grid) and (display: table))"));
    expect(t3.?.eval(mockDecl, mockSel));
});

test("css_supports: general-enclosed unknown and invalid syntax rejection", () => {
    let buf: [128]u8 = [0; 128];

    const u1 = SupportsTree.parse("font-tech(color-COLRv1)");
    expect(u1 != null);
    expect(strEq(u1.?.toCss(buf[0..]), "font-tech(color-COLRv1)"));
    expect(!u1.?.eval(mockDecl, mockSel));

    // Mixing `and` and `or` at same level is invalid per CSS Conditional Rules §3
    expect(SupportsTree.parse("(display: grid) and (display: flex) or (color: red)") == null);
    // Unclosed parens invalid
    expect(SupportsTree.parse("(display: grid") == null);
    // Empty invalid
    expect(SupportsTree.parse("") == null);
});
