import { test, expect_eq } from "cap:test";
import {
    VteActionKind,
    VteEvent,
    VteParser,
} from "../../packages/vte/src/index.cp";

fn main() {
    test("1. @cap/vte — Printable characters and C0 control characters (\\r, \\n, \\t)", () => {
        let parser = VteParser.new();
        let events: [16]VteEvent = undefined;
        const input = "Hi\r\n\t!";
        const n = parser.advance(input, events[0..16]);

        expect_eq(n, 6 as usize);
        expect_eq(events[0].kind == VteActionKind.Print, true);
        expect_eq(events[0].ch, 72 as u8); // 'H'
        expect_eq(events[1].kind == VteActionKind.Print, true);
        expect_eq(events[1].ch, 105 as u8); // 'i'
        expect_eq(events[2].kind == VteActionKind.Execute, true);
        expect_eq(events[2].ch, 13 as u8); // '\r'
        expect_eq(events[3].kind == VteActionKind.Execute, true);
        expect_eq(events[3].ch, 10 as u8); // '\n'
        expect_eq(events[4].kind == VteActionKind.Execute, true);
        expect_eq(events[4].ch, 9 as u8); // '\t'
        expect_eq(events[5].kind == VteActionKind.Print, true);
        expect_eq(events[5].ch, 33 as u8); // '!'
    });

    test("2. @cap/vte — CSI SGR color sequence \\x1b[31;1m and text", () => {
        let parser = VteParser.new();
        let events: [16]VteEvent = undefined;
        const input = "\x1b[31;1mOK\x1b[0m";
        const n = parser.advance(input, events[0..16]);

        expect_eq(n, 4 as usize);
        expect_eq(events[0].kind == VteActionKind.CsiDispatch, true);
        expect_eq(events[0].final_byte, 109 as u8); // 'm'
        expect_eq(events[0].param_count, 2 as usize);
        expect_eq(events[0].params[0], 31 as u16);
        expect_eq(events[0].params[1], 1 as u16);

        expect_eq(events[1].kind == VteActionKind.Print, true);
        expect_eq(events[1].ch, 79 as u8); // 'O'
        expect_eq(events[2].kind == VteActionKind.Print, true);
        expect_eq(events[2].ch, 75 as u8); // 'K'

        expect_eq(events[3].kind == VteActionKind.CsiDispatch, true);
        expect_eq(events[3].final_byte, 109 as u8); // 'm'
        expect_eq(events[3].param_count, 1 as usize);
        expect_eq(events[3].params[0], 0 as u16);
    });

    test("3. @cap/vte — CSI cursor position \\x1b[10;20H, empty params \\x1b[H and private mode \\x1b[?25h", () => {
        let parser = VteParser.new();
        let events: [16]VteEvent = undefined;
        const input = "\x1b[10;20H\x1b[H\x1b[?25h";
        const n = parser.advance(input, events[0..16]);

        expect_eq(n, 3 as usize);
        expect_eq(events[0].kind == VteActionKind.CsiDispatch, true);
        expect_eq(events[0].final_byte, 72 as u8); // 'H'
        expect_eq(events[0].param_count, 2 as usize);
        expect_eq(events[0].params[0], 10 as u16);
        expect_eq(events[0].params[1], 20 as u16);

        expect_eq(events[1].kind == VteActionKind.CsiDispatch, true);
        expect_eq(events[1].final_byte, 72 as u8); // 'H'
        expect_eq(events[1].param_count, 0 as usize);

        expect_eq(events[2].kind == VteActionKind.CsiDispatch, true);
        expect_eq(events[2].final_byte, 104 as u8); // 'h'
        expect_eq(events[2].intermediate, 63 as u8); // '?'
        expect_eq(events[2].param_count, 1 as usize);
        expect_eq(events[2].params[0], 25 as u16);
    });

    test("4. @cap/vte — OSC window title sequences (BEL \\x07 and ST \\x1b\\\\)", () => {
        let parser = VteParser.new();
        let events: [16]VteEvent = undefined;
        const n1 = parser.advance("\x1b]0;title\x07", events[0..16]);

        expect_eq(n1, 1 as usize);
        expect_eq(events[0].kind == VteActionKind.OscDispatch, true);
        expect_eq(events[0].final_byte, 7 as u8);
        expect_eq(events[0].param_count, 1 as usize);
        expect_eq(events[0].params[0], 0 as u16);
        expect_eq(parser.oscLen(), 7 as usize); // "0;title"
        expect_eq(parser.oscByte(2), 116 as u8); // 't'

        const n2 = parser.advance("\x1b]2;cp\x1b\\", events[0..16]);
        expect_eq(n2, 1 as usize);
        expect_eq(events[0].kind == VteActionKind.OscDispatch, true);
        expect_eq(events[0].final_byte, 92 as u8); // '\\'
        expect_eq(events[0].params[0], 2 as u16);
        expect_eq(parser.oscLen(), 4 as usize); // "2;cp"
    });

    test("5. @cap/vte — ESC dispatch and chunked streaming across byte boundaries", () => {
        let parser = VteParser.new();
        let events: [16]VteEvent = undefined;

        // ESC 7 (save cursor) and ESC ( B (charset with intermediate '(')
        const n1 = parser.advance("\x1b7\x1b(B", events[0..16]);
        expect_eq(n1, 2 as usize);
        expect_eq(events[0].kind == VteActionKind.EscDispatch, true);
        expect_eq(events[0].final_byte, 55 as u8); // '7'
        expect_eq(events[0].intermediate, 0 as u8);
        expect_eq(events[1].kind == VteActionKind.EscDispatch, true);
        expect_eq(events[1].final_byte, 66 as u8); // 'B'
        expect_eq(events[1].intermediate, 40 as u8); // '('

        // Split "\x1b[12;34m" across 3 separate advance() calls
        expect_eq(parser.advance("\x1b[1", events[0..16]), 0 as usize);
        expect_eq(parser.advance("2;3", events[0..16]), 0 as usize);
        const n3 = parser.advance("4m", events[0..16]);
        expect_eq(n3, 1 as usize);
        expect_eq(events[0].kind == VteActionKind.CsiDispatch, true);
        expect_eq(events[0].final_byte, 109 as u8);
        expect_eq(events[0].param_count, 2 as usize);
        expect_eq(events[0].params[0], 12 as u16);
        expect_eq(events[0].params[1], 34 as u16);
    });

    test("6. @cap/vte — CAN abort, undersized output buffer & parameter saturation", () => {
        let parser = VteParser.new();
        let events: [2]VteEvent = undefined;

        // CAN (0x18) aborts CSI and emits Execute, followed by Print 'A'
        const n1 = parser.advance("\x1b[31\x18A", events[0..2]);
        expect_eq(n1, 2 as usize);
        expect_eq(events[0].kind == VteActionKind.Execute, true);
        expect_eq(events[0].ch, 0x18 as u8);
        expect_eq(events[1].kind == VteActionKind.Print, true);
        expect_eq(events[1].ch, 65 as u8);

        // Undersized out_events caps emitted count safely without OOB
        const n2 = parser.advance("ABCD", events[0..2]);
        expect_eq(n2, 2 as usize);
        expect_eq(events[0].ch, 65 as u8);
        expect_eq(events[1].ch, 66 as u8);

        // Huge parameter saturates at 65535
        const n3 = parser.advance("\x1b[99999m", events[0..2]);
        expect_eq(n3, 1 as usize);
        expect_eq(events[0].params[0], 65535 as u16);
    });
}
