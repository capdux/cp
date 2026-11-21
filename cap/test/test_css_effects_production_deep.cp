import { test, expect } from "cap:test";
import { FilterKind, Filter, FilterList } from "cap:css_effects";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("css_effects: FilterList multi-filter parsing and serialization", () => {
    let buf: [256]u8 = [0; 256];

    const fl = FilterList.parse("blur(4px) brightness(120%) hue-rotate(90deg) drop-shadow(2px 4px 6px rgba(0, 0, 0, 0.5))");
    expect(fl != null);
    expect(!fl.?.is_none && fl.?.len == 4);
    expect(fl.?.items[0].kind == FilterKind.blur);
    expect(fl.?.items[1].value > 1.19 && fl.?.items[1].value < 1.21);
    expect(fl.?.items[3].kind == FilterKind.drop_shadow);
    expect(strEq(
        fl.?.toCss(buf[0..]),
        "blur(4px) brightness(120%) hue-rotate(90deg) drop-shadow(2px 4px 6px rgba(0, 0, 0, 0.5))",
    ));

    const none_fl = FilterList.parse("none");
    expect(none_fl != null && none_fl.?.is_none);
    expect(strEq(none_fl.?.toCss(buf[0..]), "none"));
});

test("css_effects: clamping, url(), defaults, and invalid input rejection", () => {
    let buf: [128]u8 = [0; 128];

    // grayscale clamps >100% to 100%
    const g = Filter.parse("grayscale(150%)");
    expect(g != null && g.?.value == 1.0);
    expect(strEq(g.?.toCss(buf[0..]), "grayscale(100%)"));

    // url() strips quotes
    const u = Filter.parse("url(\"#gooey\")");
    expect(u != null);
    expect(strEq(u.?.toCss(buf[0..]), "url(#gooey)"));

    // Invalid: negative blur, negative opacity, unknown filter, negative drop-shadow blur
    expect(Filter.parse("blur(-2px)") == null);
    expect(Filter.parse("opacity(-0.5)") == null);
    expect(Filter.parse("unknown(10px)") == null);
    expect(Filter.parse("drop-shadow(1px 2px -3px red)") == null);
    expect(FilterList.parse("blur(2px") == null);
});
