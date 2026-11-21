// Source: cp/cap/test/test_unicode_lut_production_deep.cp (Production deep tests for cap:unicode_lut)

import { test, expect_eq } from "cap:test";
import {
    Generator,
    Tables,
    BLOCK_SIZE,
    MAX_UNICODE_CODEPOINT,
    MAX_UNIQUE_ELEMENTS,
} from "cap:unicode_lut";

struct ModPropCtx {
    fn get(cp: u21): u8 {
        if (cp < 128) {
            return (cp % 7) as u8;
        }
        if (cp >= 0x0300 && cp <= 0x036F) {
            return 10;
        }
        if (cp >= 0x0500 && cp <= 0x0537) {
            return 11;
        }
        return 0;
    }

    fn eql(a: u8, b: u8): bool {
        return a == b;
    }
}

enum CharCategory: u8 {
    other = 0,
    ascii_digit = 1,
    ascii_alpha = 2,
    cjk = 3,
    emoji = 4,
}

struct CharProps {
    category: CharCategory,
    width: u8,
    is_id_start: bool,
}

struct CharPropsCtx {
    fn get(cp: u21): CharProps {
        if (cp >= '0' && cp <= '9') {
            return CharProps{ category: .ascii_digit, width: 1, is_id_start: false };
        }
        if ((cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z')) {
            return CharProps{ category: .ascii_alpha, width: 1, is_id_start: true };
        }
        if (cp >= 0x4E00 && cp <= 0x9FFF) {
            return CharProps{ category: .cjk, width: 2, is_id_start: true };
        }
        if (cp >= 0x1F600 && cp <= 0x1F64F) {
            return CharProps{ category: .emoji, width: 2, is_id_start: false };
        }
        return CharProps{ category: .other, width: 1, is_id_start: false };
    }

    fn eql(a: CharProps, b: CharProps): bool {
        return a.category == b.category && a.width == b.width && a.is_id_start == b.is_id_start;
    }
}

struct IdentityU16Ctx {
    fn get(cp: u21): u16 {
        return cp as u16;
    }

    fn eql(a: u16, b: u16): bool {
        return a == b;
    }
}

test("Custom non-aligned range build, exhaustive O(1) lookup and out-of-range guard") {
    const alloc = std.testing.allocator;
    const max_cp: u21 = 0x0537; // 非 256 对齐尾块边界
    const ctx = ModPropCtx{};
    const gen = Generator<u8, ModPropCtx>.init(ctx);
    const lut = gen.generateUpTo(alloc, max_cp) catch unreachable;

    let cp: u21 = 0;
    while (cp <= max_cp) {
        const expected = ctx.get(cp);
        if (got := lut.get(cp)) {
            expect_eq(got, expected);
        } else {
            expect_eq(true, false);
        }
        expect_eq(lut.getUnchecked(cp), expected);
        cp += 1;
    }

    // 越界查询严格返回 null
    expect_eq(lut.get(max_cp + 1) == null, true);
    expect_eq(lut.get(MAX_UNICODE_CODEPOINT) == null, true);
}

test("BMP 65536 codepoints block deduplication compression ratio") {
    const alloc = std.testing.allocator;
    const ctx = CharPropsCtx{};
    const gen = Generator<CharProps, CharPropsCtx>.init(ctx);
    const lut = gen.generateUpTo(alloc, 0xFFFF) catch unreachable;

    // 仅 4 种唯一结构体（other, ascii_digit, ascii_alpha, cjk；emoji 在 BMP 之外）
    expect_eq(lut.uniqueCount(), @as(usize, 4));
    expect_eq(lut.stage1.len, @as(usize, 256));

    // 大量重复块被合并，压缩后体积极小（远小于未压缩的 65536 * @sizeOf(CharProps)）
    expect_eq(lut.totalBytes() * 10 < lut.uncompressedBytes(), true);

    // 逐码点验证全部 65536 个 BMP 码点
    let cp: u32 = 0;
    while (cp <= 0xFFFF) {
        const c = cp as u21;
        const expected = ctx.get(c);
        const got = lut.getUnchecked(c);
        expect_eq(ctx.eql(got, expected), true);
        cp += 1;
    }
}

test("Struct and enum Elem lookup semantics") {
    const alloc = std.testing.allocator;
    const gen = Generator<CharProps, CharPropsCtx>.init(CharPropsCtx{});
    const lut = gen.generateUpTo(alloc, 0x1F6FF) catch unreachable;

    expect_eq(lut.uniqueCount(), @as(usize, 5));

    const p_digit = lut.get('7').?;
    expect_eq(p_digit.category == .ascii_digit, true);
    expect_eq(p_digit.width, @as(u8, 1));
    expect_eq(p_digit.is_id_start, false);

    const p_alpha = lut.get('Z').?;
    expect_eq(p_alpha.category == .ascii_alpha, true);
    expect_eq(p_alpha.is_id_start, true);

    const p_cjk = lut.get(0x4E2D).?; // '中'
    expect_eq(p_cjk.category == .cjk, true);
    expect_eq(p_cjk.width, @as(u8, 2));
    expect_eq(p_cjk.is_id_start, true);

    const p_emoji = lut.get(0x1F600).?;
    expect_eq(p_emoji.category == .emoji, true);
    expect_eq(p_emoji.width, @as(u8, 2));
    expect_eq(p_emoji.is_id_start, false);
}

test("Boundary guards: exact 256 unique elements, TooManyUniqueElements and CodepointOutOfRange") {
    const alloc = std.testing.allocator;
    const gen = Generator<u16, IdentityU16Ctx>.init(IdentityU16Ctx{});

    // 1. 恰好 256 种唯一元素（0..=255）应成功构建
    const exact = gen.generateUpTo(alloc, 255) catch unreachable;
    expect_eq(exact.uniqueCount(), MAX_UNIQUE_ELEMENTS);
    expect_eq(exact.get(0).?, @as(u16, 0));
    expect_eq(exact.get(255).?, @as(u16, 255));

    // 2. 257 种唯一元素（0..=256）必须返回 TooManyUniqueElements 且无内存泄漏
    const too_many = switch (gen.generateUpTo(alloc, 256)) {
        err.TooManyUniqueElements => true,
        else => false,
    };
    expect_eq(too_many, true);

    // 3. 超过 MAX_UNICODE_CODEPOINT (0x10FFFF) 必须返回 CodepointOutOfRange
    const out_of_range = switch (gen.generateUpTo(alloc, 0x110000)) {
        err.CodepointOutOfRange => true,
        else => false,
    };
    expect_eq(out_of_range, true);
}

test("Full Unicode range 0..=0x10FFFF generation, extreme compression and RAII cleanup") {
    const alloc = std.testing.allocator;
    const gen = Generator<CharProps, CharPropsCtx>.init(CharPropsCtx{});
    const full = gen.generate(alloc) catch unreachable;

    expect_eq(full.max_codepoint, MAX_UNICODE_CODEPOINT);
    expect_eq(full.stage1.len, @as(usize, (MAX_UNICODE_CODEPOINT as usize + 1) / BLOCK_SIZE));
    expect_eq(full.uniqueCount(), @as(usize, 5));
    // 全量 1,114,112 码点压缩后应远小于未压缩大小（> 100x 压缩比）
    expect_eq(full.totalBytes() * 100 < full.uncompressedBytes(), true);

    expect_eq(full.get('a').?.category == .ascii_alpha, true);
    expect_eq(full.get(0x9FFF).?.category == .cjk, true);
    expect_eq(full.get(0x1F64F).?.category == .emoji, true);
    expect_eq(full.get(0x10FFFF).?.category == .other, true);
}

test("Multi-round RAII lifecycle isolation and zero-leak verification") {
    const alloc = std.testing.allocator;
    const gen = Generator<u8, ModPropCtx>.init(ModPropCtx{});
    const limits: [4]u21 = [0, 127, 255, 1024];

    let i: usize = 0;
    while (i < 4) {
        const tbl = gen.generateUpTo(alloc, limits[i]) catch unreachable;
        expect_eq(tbl.max_codepoint, limits[i]);
        expect_eq(tbl.get(0).?, @as(u8, 0));
        expect_eq(tbl.get(limits[i]) != null, true);
        expect_eq(tbl.get(limits[i] + 1) == null, true);
        i += 1;
    }
}
