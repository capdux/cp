import { test, expect } from "cap:test";
import {
    CssCounterStyleRule,
} from "cap:css";

fn eqBytes(a: []const u8, b: []const u8): bool {
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

test("css counter_style: additive upper-roman parsing, formatting and range bounds") {
    const roman = CssCounterStyleRule.parse(
        "@counter-style upper-roman {\n" ++
        "  system: additive;\n" ++
        "  range: 1 3999;\n" ++
        "  additive-symbols: 1000 M, 900 CM, 500 D, 400 CD, 100 C, 90 XC, 50 L, 40 XL, 10 X, 9 IX, 5 V, 4 IV, 1 I;\n" ++
        "  suffix: \". \";\n" ++
        "  fallback: decimal;\n" ++
        "}"
    )?;

    expect(eqBytes(roman.name, "upper-roman"));
    expect(roman.system.kind == .additive);
    expect(roman.additive_count == 13);
    expect(roman.isInRange(1));
    expect(roman.isInRange(3999));
    expect(!roman.isInRange(0));
    expect(!roman.isInRange(4000));

    let buf: [64]u8 = [0; 64];
    expect(eqBytes(buf[0..roman.formatCounter(1, buf[0..])?], "I"));
    expect(eqBytes(buf[0..roman.formatCounter(4, buf[0..])?], "IV"));
    expect(eqBytes(buf[0..roman.formatCounter(42, buf[0..])?], "XLII"));
    expect(eqBytes(buf[0..roman.formatCounter(1999, buf[0..])?], "MCMXCIX"));
    expect(eqBytes(buf[0..roman.formatMarker(2026, buf[0..])?], "MMXXVI. "));

    // 越界返回 OutOfRange
    let out_err: usize = 0;
    * = roman.formatCounter(0, buf[0..]) catch { out_err += 1; 0; };
    * = roman.formatCounter(4000, buf[0..]) catch { out_err += 1; 0; };
    expect(out_err == 2);
}

test("css counter_style: alphabetic bijective base-26 and numeric with negative + pad") {
    const alpha = CssCounterStyleRule.parse(
        "@counter-style excel-col { system: alphabetic; symbols: A B C D E F G H I J K L M N O P Q R S T U V W X Y Z; }"
    )?;
    let buf: [64]u8 = [0; 64];
    expect(eqBytes(buf[0..alpha.formatCounter(1, buf[0..])?], "A"));
    expect(eqBytes(buf[0..alpha.formatCounter(26, buf[0..])?], "Z"));
    expect(eqBytes(buf[0..alpha.formatCounter(27, buf[0..])?], "AA"));
    expect(eqBytes(buf[0..alpha.formatCounter(52, buf[0..])?], "AZ"));
    expect(eqBytes(buf[0..alpha.formatCounter(702, buf[0..])?], "ZZ"));
    expect(eqBytes(buf[0..alpha.formatCounter(703, buf[0..])?], "AAA"));

    // numeric + negative 前后缀 + pad 最小宽度（按 W3C 规范负号计入宽度）
    const padded = CssCounterStyleRule.parse(
        "@counter-style ledger { system: numeric; symbols: '0' '1' '2' '3' '4' '5' '6' '7' '8' '9'; negative: '(' ')'; pad: 4 '0'; prefix: '['; suffix: '] '; }"
    )?;
    expect(eqBytes(buf[0..padded.formatCounter(0, buf[0..])?], "0000"));
    expect(eqBytes(buf[0..padded.formatCounter(7, buf[0..])?], "0007"));
    expect(eqBytes(buf[0..padded.formatCounter(12345, buf[0..])?], "12345"));
    // 负数 -7 含 '(' 与 ')' 共占 3 码点，pad 到 4 码点需补 1 个 '0' => "(07)"
    expect(eqBytes(buf[0..padded.formatCounter(-7, buf[0..])?], "(07)"));
    expect(eqBytes(buf[0..padded.formatMarker(-7, buf[0..])?], "[(07)] "));
}

test("css counter_style: cyclic, fixed with custom start, and symbolic algorithms") {
    let buf: [64]u8 = [0; 64];

    const cyc = CssCounterStyleRule.parse(
        "@counter-style ring { system: cyclic; symbols: 'a' 'b' 'c'; }"
    )?;
    expect(eqBytes(buf[0..cyc.formatCounter(1, buf[0..])?], "a"));
    expect(eqBytes(buf[0..cyc.formatCounter(2, buf[0..])?], "b"));
    expect(eqBytes(buf[0..cyc.formatCounter(4, buf[0..])?], "a"));
    expect(eqBytes(buf[0..cyc.formatCounter(0, buf[0..])?], "c"));
    expect(eqBytes(buf[0..cyc.formatCounter(-1, buf[0..])?], "b"));

    const fix = CssCounterStyleRule.parse(
        "@counter-style medals { system: fixed 3; symbols: 'bronze' 'silver' 'gold'; }"
    )?;
    expect(eqBytes(buf[0..fix.formatCounter(3, buf[0..])?], "bronze"));
    expect(eqBytes(buf[0..fix.formatCounter(5, buf[0..])?], "gold"));
    let fix_err: usize = 0;
    * = fix.formatCounter(2, buf[0..]) catch { fix_err += 1; 0; };
    * = fix.formatCounter(6, buf[0..]) catch { fix_err += 1; 0; };
    expect(fix_err == 2);

    const sym = CssCounterStyleRule.parse(
        "@counter-style footnotes { system: symbolic; symbols: '*' '+'; }"
    )?;
    expect(eqBytes(buf[0..sym.formatCounter(1, buf[0..])?], "*"));
    expect(eqBytes(buf[0..sym.formatCounter(2, buf[0..])?], "+"));
    expect(eqBytes(buf[0..sym.formatCounter(3, buf[0..])?], "**"));
    expect(eqBytes(buf[0..sym.formatCounter(4, buf[0..])?], "++"));
    expect(eqBytes(buf[0..sym.formatCounter(5, buf[0..])?], "***"));
}

test("css counter_style: multi-range, extends, speak-as and flexible descriptor order") {
    const ranged = CssCounterStyleRule.parse(
        "@counter-style custom-range {\n" ++
        "  system: additive;\n" ++
        "  additive-symbols: 'X' 10, 'I' 1, '0' 0;\n" ++
        "  range: infinite 0, 5 10, 20 infinite;\n" ++
        "  pad: '0' 3;\n" ++
        "  speak-as: numbers;\n" ++
        "}"
    )?;
    expect(ranged.range_count == 3);
    expect(ranged.isInRange(-5));
    expect(ranged.isInRange(0));
    expect(!ranged.isInRange(3));
    expect(ranged.isInRange(7));
    expect(!ranged.isInRange(15));
    expect(ranged.isInRange(25));
    expect(ranged.pad_width == 3);
    expect(eqBytes(ranged.pad_symbol, "0"));
    expect(ranged.speak_as == .numbers);

    let buf: [64]u8 = [0; 64];
    expect(eqBytes(buf[0..ranged.formatCounter(0, buf[0..])?], "000"));

    const ext = CssCounterStyleRule.parse(
        "@counter-style paren-decimal { system: extends decimal; prefix: '('; suffix: ') '; speak-as: spell-out; }"
    )?;
    expect(ext.system.kind == .extends);
    expect(eqBytes(ext.system.extends_name, "decimal"));
    expect(eqBytes(ext.prefix, "("));
    expect(eqBytes(ext.suffix, ") "));
    expect(ext.speak_as == .spell_out);
}

test("css counter_style: malformed input and specification constraint guards") {
    let rejected: usize = 0;

    // 保留关键字名称
    * = CssCounterStyleRule.parse("@counter-style none { system: cyclic; symbols: '*'; }") catch { rejected += 1; CssCounterStyleRule.initEmpty(""); };
    * = CssCounterStyleRule.parse("@counter-style initial { system: cyclic; symbols: '*'; }") catch { rejected += 1; CssCounterStyleRule.initEmpty(""); };
    // 未闭合块与非 counter-style 规则
    * = CssCounterStyleRule.parse("@layer foo { }") catch { rejected += 1; CssCounterStyleRule.initEmpty(""); };
    * = CssCounterStyleRule.parse("@counter-style a { system: cyclic; symbols: '*';") catch { rejected += 1; CssCounterStyleRule.initEmpty(""); };
    // 非法 system / numeric 少于 2 符号
    * = CssCounterStyleRule.parse("@counter-style a { system: unknown; symbols: '*'; }") catch { rejected += 1; CssCounterStyleRule.initEmpty(""); };
    * = CssCounterStyleRule.parse("@counter-style a { system: numeric; symbols: '0'; }") catch { rejected += 1; CssCounterStyleRule.initEmpty(""); };
    * = CssCounterStyleRule.parse("@counter-style a { system: alphabetic; symbols: 'A'; }") catch { rejected += 1; CssCounterStyleRule.initEmpty(""); };
    // additive-symbols 非降序或负权重
    * = CssCounterStyleRule.parse("@counter-style a { system: additive; additive-symbols: 1 I, 5 V; }") catch { rejected += 1; CssCounterStyleRule.initEmpty(""); };
    * = CssCounterStyleRule.parse("@counter-style a { system: additive; additive-symbols: -1 X; }") catch { rejected += 1; CssCounterStyleRule.initEmpty(""); };
    // extends 不允许声明 symbols
    * = CssCounterStyleRule.parse("@counter-style a { system: extends decimal; symbols: '1'; }") catch { rejected += 1; CssCounterStyleRule.initEmpty(""); };
    // range 下界大于上界 / pad 负宽度 / 非法 speak-as
    * = CssCounterStyleRule.parse("@counter-style a { system: cyclic; symbols: '*'; range: 10 5; }") catch { rejected += 1; CssCounterStyleRule.initEmpty(""); };
    * = CssCounterStyleRule.parse("@counter-style a { system: cyclic; symbols: '*'; pad: -2 '0'; }") catch { rejected += 1; CssCounterStyleRule.initEmpty(""); };
    * = CssCounterStyleRule.parse("@counter-style a { system: cyclic; symbols: '*'; speak-as: invalid-mode; }") catch { rejected += 1; CssCounterStyleRule.initEmpty(""); };

    expect(rejected == 13);
}

test("css counter_style: parse -> toCss -> parse round-trip idempotency and buffer guard") {
    const src =
        "@counter-style custom-hex {\n" ++
        "  system: numeric;\n" ++
        "  symbols: '0' '1' '2' '3' '4' '5' '6' '7' '8' '9' 'A' 'B' 'C' 'D' 'E' 'F';\n" ++
        "  negative: '-';\n" ++
        "  prefix: '0x';\n" ++
        "  suffix: ' ';\n" ++
        "  range: -255 65535;\n" ++
        "  pad: 2 '0';\n" ++
        "  fallback: decimal;\n" ++
        "  speak-as: numbers;\n" ++
        "}";

    const r1 = CssCounterStyleRule.parse(src)?;
    let buf1: [512]u8 = [0; 512];
    const n1 = r1.toCss(buf1[0..])?;

    const r2 = CssCounterStyleRule.parse(buf1[0..n1])?;
    expect(r1.eql(r2));

    let buf2: [512]u8 = [0; 512];
    const n2 = r2.toCss(buf2[0..])?;
    expect(eqBytes(buf1[0..n1], buf2[0..n2]));

    // 验证十六进制渲染
    let out: [32]u8 = [0; 32];
    expect(eqBytes(out[0..r2.formatMarker(255, out[0..])?], "0xFF "));
    expect(eqBytes(out[0..r2.formatMarker(10, out[0..])?], "0x0A "));

    // 缓冲区不足防护
    let tiny: [10]u8 = [0; 10];
    const small_res = r1.toCss(tiny[0..]) catch {
        expect(true);
        return;
    };
    * = small_res;
    expect(false);
}
