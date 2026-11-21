import {
    SgrColorKind,
    SgrColor,
    SgrStyle,
    formatStyled,
    applySgrSequence,
} from "cap:ansi_sgr";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("SgrStyle plain and 16-color attributes formatting", () => {
    let buf: [128]u8 = [0; 128];

    // Plain style outputs empty open sequence and copies text directly
    const plain = SgrStyle.plain();
    expect(plain.isPlain());
    expect(strEq(plain.formatOpen(buf[0..]).?, ""));
    expect(strEq(formatStyled("hello", plain, buf[0..]).?, "hello"));

    // Bold + underline + bright red fg (91) + blue bg (44)
    let st = SgrStyle.plain();
    st.bold = true;
    st.underline = true;
    st.fg = SgrColor.ansi16(91);
    st.bg = SgrColor.ansi16(44);
    expect(!st.isPlain());
    expect(strEq(st.formatOpen(buf[0..]).?, "\x1b[1;4;91;44m"));
    expect(strEq(formatStyled("ERR", st, buf[0..]).?, "\x1b[1;4;91;44mERR\x1b[0m"));

    // 0..15 palette index normalization (1 -> fg 31, 10 -> bg 102)
    let st_idx = SgrStyle.plain();
    st_idx.italic = true;
    st_idx.fg = SgrColor.ansi16(1);
    st_idx.bg = SgrColor.ansi16(10);
    expect(strEq(st_idx.formatOpen(buf[0..]).?, "\x1b[3;31;102m"));
});

test("256-color palette and 24-bit TrueColor RGB formatting and roundtrip", () => {
    let buf: [128]u8 = [0; 128];

    // 256-color fg & bg
    let st256 = SgrStyle.plain();
    st256.dim = true;
    st256.fg = SgrColor.ansi256(214);
    st256.bg = SgrColor.ansi256(17);
    const open256 = st256.formatOpen(buf[0..]).?;
    expect(strEq(open256, "\x1b[2;38;5;214;48;5;17m"));

    let decoded256 = SgrStyle.plain();
    expect(applySgrSequence(open256, &decoded256));
    expect(decoded256.dim && !decoded256.bold);
    expect(decoded256.fg.kind == SgrColorKind.ansi256 && decoded256.fg.code == 214);
    expect(decoded256.bg.kind == SgrColorKind.ansi256 && decoded256.bg.code == 17);

    // 24-bit TrueColor RGB fg & bg + inverse + strikethrough
    let st_rgb = SgrStyle.plain();
    st_rgb.inverse = true;
    st_rgb.strikethrough = true;
    st_rgb.fg = SgrColor.rgb(255, 128, 0);
    st_rgb.bg = SgrColor.rgb(12, 34, 56);
    const open_rgb = st_rgb.formatOpen(buf[0..]).?;
    expect(strEq(open_rgb, "\x1b[7;9;38;2;255;128;0;48;2;12;34;56m"));

    let decoded_rgb = SgrStyle.plain();
    expect(applySgrSequence(open_rgb, &decoded_rgb));
    expect(decoded_rgb.inverse && decoded_rgb.strikethrough);
    expect(decoded_rgb.fg.equals(SgrColor.rgb(255, 128, 0)));
    expect(decoded_rgb.bg.equals(SgrColor.rgb(12, 34, 56)));
});

test("applySgrSequence incremental updates, disables (22/23/24/27/29), and resets (0/39/49)", () => {
    let st = SgrStyle.plain();

    // Enable all attributes + colors
    expect(applySgrSequence("\x1b[1;2;3;4;7;9;32;103m", &st));
    expect(st.bold && st.dim && st.italic && st.underline && st.inverse && st.strikethrough);
    expect(st.fg.kind == SgrColorKind.ansi16 && st.fg.code == 32);
    expect(st.bg.kind == SgrColorKind.ansi16 && st.bg.code == 103);

    // Disable bold+dim (22), italic (23), underline (24), inverse (27), strikethrough (29)
    expect(applySgrSequence("\x1b[22;23;24;27;29m", &st));
    expect(!st.bold && !st.dim && !st.italic && !st.underline && !st.inverse && !st.strikethrough);
    // Colors remain intact
    expect(st.fg.code == 32 && st.bg.code == 103);

    // Default fg (39) and default bg (49)
    expect(applySgrSequence("\x1b[39m", &st));
    expect(st.fg.isNone() && !st.bg.isNone());
    expect(applySgrSequence("\x1b[49m", &st));
    expect(st.isPlain());

    // Empty \x1b[m and \x1b[0;1m mid-sequence reset
    expect(applySgrSequence("\x1b[31;4m", &st));
    expect(applySgrSequence("\x1b[m", &st));
    expect(st.isPlain());

    expect(applySgrSequence("\x1b[31;4;0;1;96m", &st));
    expect(st.bold && !st.underline && st.fg.code == 96);
});

test("buffer capacity boundary checks for formatOpen and formatStyled", () => {
    let st = SgrStyle.plain();
    st.bold = true;
    st.fg = SgrColor.ansi16(31); // open = "\x1b[1;31m" (7 bytes)

    let buf: [32]u8 = [0; 32];
    expect(strEq(st.formatOpen(buf[0..7]).?, "\x1b[1;31m"));
    expect(st.formatOpen(buf[0..6]) == null);
    expect(st.formatOpen(buf[0..0]) == null);

    // formatStyled("HI", st) = 7 + 2 + 4 = 13 bytes
    expect(strEq(formatStyled("HI", st, buf[0..13]).?, "\x1b[1;31mHI\x1b[0m"));
    expect(formatStyled("HI", st, buf[0..12]) == null);

    // Plain formatStyled exact capacity
    const plain = SgrStyle.plain();
    expect(strEq(formatStyled("ABC", plain, buf[0..3]).?, "ABC"));
    expect(formatStyled("ABC", plain, buf[0..2]) == null);
});

test("malformed SGR sequence protection preserves existing style atomically", () => {
    let st = SgrStyle.plain();
    st.bold = true;
    st.fg = SgrColor.ansi16(36);

    // Invalid wrappers
    expect(!applySgrSequence("", &st));
    expect(!applySgrSequence("\x1b[", &st));
    expect(!applySgrSequence("1;31m", &st));
    expect(!applySgrSequence("\x1b[31K", &st));
    expect(!applySgrSequence("\x1b[1;am", &st));

    // Truncated or out-of-range extended color sequences
    expect(!applySgrSequence("\x1b[38m", &st));
    expect(!applySgrSequence("\x1b[38;5m", &st));
    expect(!applySgrSequence("\x1b[38;5;256m", &st));
    expect(!applySgrSequence("\x1b[38;2;255;0m", &st));
    expect(!applySgrSequence("\x1b[48;2;255;300;0m", &st));
    expect(!applySgrSequence("\x1b[38;9;1m", &st));

    // Malformed sequence with valid prefix must NOT partially mutate `st`
    expect(!applySgrSequence("\x1b[0;38;2;1;2m", &st));
    expect(st.bold && st.fg.kind == SgrColorKind.ansi16 && st.fg.code == 36);
});
