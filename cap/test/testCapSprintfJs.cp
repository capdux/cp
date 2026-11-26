import { test, expect_eq } from "cap:test";
import {
    FmtArgKind,
    FmtArg,
    sprintf,
} from "../../packages/@cap/sprintf-js/src/index.cp";

fn main() {
    test("1. @cap/sprintf-js — basic %s, %d, %i, %c, %% formatting", () => {
        const args: [4]FmtArg = [
            FmtArg.str("CP"),
            FmtArg.int(42),
            FmtArg.int(-7),
            FmtArg.ch(0x21),
        ];
        let buf: [128]u8 = [0; 128];
        const n: usize = sprintf("Hello %s: %d, %i, 100%%%c", args[0..4], buf[0..128]);
        expect_eq(buf[0..n], "Hello CP: 42, -7, 100%!");
        expect_eq(args[0].kind == FmtArgKind.Str, true);
    });

    test("2. @cap/sprintf-js — unsigned radices %u, %x, %X, %o, %b", () => {
        const args: [5]FmtArg = [
            FmtArg.uint(255),
            FmtArg.uint(0xdead),
            FmtArg.uint(0xbeef),
            FmtArg.uint(511),
            FmtArg.uint(13),
        ];
        let buf: [128]u8 = [0; 128];
        const n: usize = sprintf("%u %x %X %o %b", args[0..5], buf[0..128]);
        expect_eq(buf[0..n], "255 dead BEEF 777 1101");
    });

    test("3. @cap/sprintf-js — width, left-align (-), plus (+), zero-pad (0)", () => {
        const args: [5]FmtArg = [
            FmtArg.int(42),
            FmtArg.int(-42),
            FmtArg.int(42),
            FmtArg.int(7),
            FmtArg.uint(0x1A),
        ];
        let buf: [128]u8 = [0; 128];
        const n: usize = sprintf("[%5d][%05d][%+05d][%-4d][%04X]", args[0..5], buf[0..128]);
        expect_eq(buf[0..n], "[   42][-0042][+0042][7   ][001A]");
    });

    test("4. @cap/sprintf-js — %f floating point precision, rounding & signs", () => {
        const args: [4]FmtArg = [
            FmtArg.float(3.14159),
            FmtArg.float(-2.5),
            FmtArg.float(9.996),
            FmtArg.float(1.25),
        ];
        let buf: [128]u8 = [0; 128];
        const n: usize = sprintf("%.2f | %.0f | %.2f | %+07.2f", args[0..4], buf[0..128]);
        expect_eq(buf[0..n], "3.14 | -3 | 10.00 | +001.25");
    });

    test("5. @cap/sprintf-js — %s precision truncation, padding & bool/int fallback", () => {
        const args: [4]FmtArg = [
            FmtArg.str("abcdef"),
            FmtArg.str("hi"),
            FmtArg.boolean(true),
            FmtArg.boolean(false),
        ];
        let buf: [128]u8 = [0; 128];
        const n: usize = sprintf("[%.3s][%-5s][%s][%s]", args[0..4], buf[0..128]);
        expect_eq(buf[0..n], "[abc][hi   ][true][false]");
    });

    test("6. @cap/sprintf-js — zero values, i64 min & buffer truncation safety", () => {
        const args: [2]FmtArg = [
            FmtArg.int(0),
            FmtArg.int(-9223372036854775807 - 1),
        ];
        let buf: [64]u8 = [0; 64];
        const n: usize = sprintf("%d %d", args[0..2], buf[0..64]);
        expect_eq(buf[0..n], "0 -9223372036854775808");

        let tiny: [6]u8 = [0; 6];
        const tn: usize = sprintf("%d %d", args[0..2], tiny[0..6]);
        expect_eq(tn, 6 as usize);
        expect_eq(tiny[0..6], "0 -922");
    });
}
