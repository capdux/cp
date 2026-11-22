import {
    CSS_CONTAINER_MAX_NAMES,
    CssContainError,
    isReservedContainerName,
    CssContainType,
    CssContainerNameList,
    CssContainerShorthand,
} from "cap:css";
import { expect, test } from "cap:test";

fn sliceEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("css containProp: CssContainType parse, default and size container queries", fn () {
    expect(CssContainType.default()).toBe(CssContainType.normal);
    expect(CssContainType.normal.isDefault()).toBe(true);
    expect(CssContainType.normal.establishesSizeContainer()).toBe(false);
    expect(CssContainType.size.establishesSizeContainer()).toBe(true);
    expect(CssContainType.inline_size.establishesSizeContainer()).toBe(true);

    expect(CssContainType.parse("INLINE-SIZE") ?? CssContainType.normal).toBe(CssContainType.inline_size);
    expect(CssContainType.parse("size") ?? CssContainType.normal).toBe(CssContainType.size);
    expect(CssContainType.parse("block-size") == null).toBe(true);
    expect(sliceEql(CssContainType.inline_size.asStr(), "inline-size")).toBe(true);
});

test("css containProp: CssContainerNameList none, multi-ident and case-sensitive contains", fn () {
    const none_list = CssContainerNameList.parse("  NONE ") catch CssContainerNameList.none();
    expect(none_list.isNone()).toBe(true);
    expect(none_list.len).toBe(0);

    let buf: [64]u8 = [0; 64];
    const n0 = none_list.toCss(buf[0..]) catch 0;
    expect(sliceEql(buf[0..n0], "none")).toBe(true);

    const multi = CssContainerNameList.parse("sidebar CardLayout main-grid") catch CssContainerNameList.none();
    expect(multi.isNone()).toBe(false);
    expect(multi.len).toBe(3);
    expect(multi.contains("sidebar")).toBe(true);
    expect(multi.contains("CardLayout")).toBe(true);
    expect(multi.contains("cardlayout")).toBe(false); // case-sensitive <custom-ident>
    expect(multi.get(3) == null).toBe(true);

    const n1 = multi.toCss(buf[0..]) catch 0;
    expect(sliceEql(buf[0..n1], "sidebar CardLayout main-grid")).toBe(true);
});

test("css containProp: CssContainerShorthand parse and minimal toCss", fn () {
    let buf: [64]u8 = [0; 64];

    const c1 = CssContainerShorthand.parse("sidebar card / inline-size") catch CssContainerShorthand.default();
    expect(c1.name.len).toBe(2);
    expect(c1.container_type).toBe(CssContainType.inline_size);
    const n1 = c1.toCss(buf[0..]) catch 0;
    expect(sliceEql(buf[0..n1], "sidebar card / inline-size")).toBe(true);

    // Default `normal` type omitted in minimal serialization
    const c2 = CssContainerShorthand.parse("layout / normal") catch CssContainerShorthand.default();
    const c3 = CssContainerShorthand.parse("layout") catch CssContainerShorthand.default();
    expect(c2.eql(c3)).toBe(true);
    const n2 = c2.toCss(buf[0..]) catch 0;
    expect(sliceEql(buf[0..n2], "layout")).toBe(true);

    // `none / size` retains explicit size type
    const c4 = CssContainerShorthand.parse("none / size") catch CssContainerShorthand.default();
    expect(c4.name.isNone()).toBe(true);
    expect(c4.container_type).toBe(CssContainType.size);
    const n4 = c4.toCss(buf[0..]) catch 0;
    expect(sliceEql(buf[0..n4], "none / size")).toBe(true);
});

test("css containProp: rejects reserved keywords and invalid <custom-ident> syntax", fn () {
    expect(isReservedContainerName("and")).toBe(true);
    expect(isReservedContainerName("NOT")).toBe(true);
    expect(isReservedContainerName("revert-layer")).toBe(true);
    expect(isReservedContainerName("sidebar")).toBe(false);

    // Reserved keyword mixed in list
    let got_reserved = false;
    * = CssContainerNameList.parse("sidebar and card") catch |e| {
        got_reserved = e == CssContainError.ReservedKeyword;
    };
    expect(got_reserved).toBe(true);

    // `none` combined with another ident is forbidden
    let got_none_mix = false;
    * = CssContainerNameList.parse("none sidebar") catch |e| {
        got_none_mix = e == CssContainError.ReservedKeyword;
    };
    expect(got_none_mix).toBe(true);

    // Custom property syntax `--foo` and digit-leading `123abc` are invalid <custom-ident>
    let got_dash = false;
    * = CssContainerNameList.parse("--custom") catch |e| {
        got_dash = e == CssContainError.InvalidName;
    };
    expect(got_dash).toBe(true);

    let got_digit = false;
    * = CssContainerNameList.parse("123card") catch |e| {
        got_digit = e == CssContainError.InvalidName;
    };
    expect(got_digit).toBe(true);
});

test("css containProp: rejects malformed shorthands, invalid type and empty input", fn () {
    let got_empty = false;
    * = CssContainerShorthand.parse("   ") catch |e| {
        got_empty = e == CssContainError.EmptyInput;
    };
    expect(got_empty).toBe(true);

    let got_multi_slash = false;
    * = CssContainerShorthand.parse("a / size / normal") catch |e| {
        got_multi_slash = e == CssContainError.InvalidShorthand;
    };
    expect(got_multi_slash).toBe(true);

    let got_empty_type = false;
    * = CssContainerShorthand.parse("sidebar /") catch |e| {
        got_empty_type = e == CssContainError.InvalidShorthand;
    };
    expect(got_empty_type).toBe(true);

    let got_bad_type = false;
    * = CssContainerShorthand.parse("sidebar / flex") catch |e| {
        got_bad_type = e == CssContainError.InvalidType;
    };
    expect(got_bad_type).toBe(true);
});

test("css containProp: max names capacity (16) and buffer too small guards", fn () {
    expect(CSS_CONTAINER_MAX_NAMES).toBe(16);
    const seventeen = "n1 n2 n3 n4 n5 n6 n7 n8 n9 n10 n11 n12 n13 n14 n15 n16 n17";
    let got_too_many = false;
    * = CssContainerNameList.parse(seventeen) catch |e| {
        got_too_many = e == CssContainError.TooManyNames;
    };
    expect(got_too_many).toBe(true);

    const c = CssContainerShorthand.parse("sidebar / inline-size") catch CssContainerShorthand.default();
    let tiny: [8]u8 = [0; 8];
    let got_small = false;
    * = c.toCss(tiny[0..]) catch |e| {
        got_small = e == CssContainError.BufferTooSmall;
    };
    expect(got_small).toBe(true);
});
