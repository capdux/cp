import { test, expect_eq } from "cap:test";
import {
    ENQ_MAX_CHOICES,
    ConfirmPrompt,
    SelectPrompt,
    MultiSelectPrompt
} from "../../packages/@cap/enquirer/src/index.cp";

fn main() {
    test("enquirer: ConfirmPrompt parses yes/no/true/false/1/0 and empty fallback", () => {
        let p_yes = ConfirmPrompt.new("Continue?", true);
        expect_eq(p_yes.parseAnswer("").?, true);
        expect_eq(p_yes.parseAnswer("   ").?, true);
        expect_eq(p_yes.parseAnswer("y").?, true);
        expect_eq(p_yes.parseAnswer("YES").?, true);
        expect_eq(p_yes.parseAnswer("true").?, true);
        expect_eq(p_yes.parseAnswer("1").?, true);
        expect_eq(p_yes.parseAnswer("n").?, false);
        expect_eq(p_yes.parseAnswer("No").?, false);
        expect_eq(p_yes.parseAnswer("FALSE").?, false);
        expect_eq(p_yes.parseAnswer("0").?, false);
        expect_eq(p_yes.parseAnswer("maybe") == null, true);

        let p_no = ConfirmPrompt.new("Delete all?", false);
        expect_eq(p_no.parseAnswer("").?, false);
    });

    test("enquirer: SelectPrompt skips disabled choices and wraps around on moveDown/moveUp", () => {
        let sel = SelectPrompt.new("Pick target");
        expect_eq(sel.addChoice("disabled-first", false), true);
        expect_eq(sel.addChoice("linux", true), true);
        expect_eq(sel.addChoice("disabled-mid", false), true);
        expect_eq(sel.addChoice("macos", true), true);

        // Initial cursor automatically skips disabled-first and lands on linux (index 1)
        expect_eq(sel.cursor, 1 as usize);

        sel.moveDown();
        expect_eq(sel.cursor, 3 as usize); // skips disabled-mid -> macos

        sel.moveDown();
        expect_eq(sel.cursor, 1 as usize); // wraps around -> linux

        sel.moveUp();
        expect_eq(sel.cursor, 3 as usize); // wraps backward -> macos

        let picked = sel.submit();
        expect_eq(picked != null, true);
        expect_eq(picked.?, "macos");
        expect_eq(sel.submitted, true);
    });

    test("enquirer: SelectPrompt render produces plain and ANSI-colored output via @cap/picocolors", () => {
        let sel = SelectPrompt.new("Framework");
        * = sel.addChoice("Cap", true);
        * = sel.addChoice("Other", true);

        let plain_buf: [256]u8 = [0 as u8; 256];
        let n_plain = sel.render(false, plain_buf[0..256]);
        let plain: string = plain_buf[0..n_plain];
        expect_eq(plain, "? Framework\n> Cap\n  Other\n");

        let color_buf: [256]u8 = [0 as u8; 256];
        let n_color = sel.render(true, color_buf[0..256]);
        // Colored output includes ANSI escape sequences (\x1b[...) so it is strictly longer than plain
        expect_eq(n_color > n_plain, true);
        expect_eq(color_buf[0], 27 as u8); // ESC
    });

    test("enquirer: SelectPrompt submit returns null when empty or all choices disabled", () => {
        let empty = SelectPrompt.new("Empty");
        expect_eq(empty.submit() == null, true);

        let all_off = SelectPrompt.new("All disabled");
        * = all_off.addChoice("a", false);
        * = all_off.addChoice("b", false);
        all_off.moveDown();
        expect_eq(all_off.submit() == null, true);
    });

    test("enquirer: MultiSelectPrompt toggles choices and tracks selectedCount", () => {
        let ms = MultiSelectPrompt.new("Features");
        expect_eq(ms.addChoice("lint"), true);
        expect_eq(ms.addChoice("format"), true);
        expect_eq(ms.addChoice("test"), true);

        expect_eq(ms.selectedCount(), 0 as usize);
        expect_eq(ms.toggle(0), true);
        expect_eq(ms.toggle(2), true);
        expect_eq(ms.toggle(99), false);

        expect_eq(ms.selectedCount(), 2 as usize);
        expect_eq(ms.isSelected(0), true);
        expect_eq(ms.isSelected(1), false);
        expect_eq(ms.isSelected(2), true);
        expect_eq(ms.isSelected(99), false);

        expect_eq(ms.toggle(0), true);
        expect_eq(ms.selectedCount(), 1 as usize);

        let buf: [256]u8 = [0 as u8; 256];
        let n = ms.render(false, buf[0..256]);
        let rendered: string = buf[0..n];
        expect_eq(rendered, "? Features\n[ ] lint\n[ ] format\n[x] test\n");
    });

    test("enquirer: enforces ENQ_MAX_CHOICES fixed capacity of 16", () => {
        let ms = MultiSelectPrompt.new("Cap");
        let i: usize = 0;
        while (i < ENQ_MAX_CHOICES) {
            expect_eq(ms.addChoice("opt"), true);
            i = i + 1;
        }
        expect_eq(ms.addChoice("overflow"), false);
        expect_eq(ms.choice_count, 16 as usize);
    });
}
