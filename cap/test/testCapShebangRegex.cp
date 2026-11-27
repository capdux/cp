import { test, expect_eq } from "cap:test";
import {
    ShebangMatch,
    hasShebang,
    matchShebang,
    stripShebang,
} from "../../packages/shebang-regex/src/index.cp";

fn sliceEq(src: string, start: usize, len: usize, expected: string): bool {
    if (len != expected.len || start + len > src.len) {
        return false;
    }
    let i: usize = 0;
    while (i < len) {
        if (src[start + i] != expected[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/shebang-regex — standard LF shebang #!/usr/bin/env node", () => {
        let src: string = "#!/usr/bin/env node\nconsole.log(1);\n";
        expect_eq(hasShebang(src), true);
        let m: ShebangMatch = matchShebang(src);
        expect_eq(m.matched, true);
        expect_eq(m.line_len, 19 as usize);
        expect_eq(m.cmd_start, 2 as usize);
        expect_eq(m.cmd_len, 17 as usize);
        expect_eq(sliceEq(src, m.cmd_start, m.cmd_len, "/usr/bin/env node"), true);
    });

    test("2. @cap/shebang-regex — CRLF shebang #!/bin/sh\\r\\n trims \\r", () => {
        let src: string = "#!/bin/sh\r\necho hi\r\n";
        let m: ShebangMatch = matchShebang(src);
        expect_eq(m.matched, true);
        expect_eq(m.line_len, 9 as usize);
        expect_eq(sliceEq(src, m.cmd_start, m.cmd_len, "/bin/sh"), true);
        let body: usize = stripShebang(src);
        expect_eq(body, 11 as usize);
        expect_eq(sliceEq(src, body, 7, "echo hi"), true);
    });

    test("3. @cap/shebang-regex — space after #! (#! /usr/bin/python3)", () => {
        let src: string = "#! /usr/bin/python3 -u\nprint(1)";
        let m: ShebangMatch = matchShebang(src);
        expect_eq(m.matched, true);
        expect_eq(sliceEq(src, m.cmd_start, m.cmd_len, " /usr/bin/python3 -u"), true);
    });

    test("4. @cap/shebang-regex — single-line shebang without newline", () => {
        let src: string = "#!/usr/bin/env bash";
        let m: ShebangMatch = matchShebang(src);
        expect_eq(m.matched, true);
        expect_eq(m.line_len, 19 as usize);
        expect_eq(sliceEq(src, m.cmd_start, m.cmd_len, "/usr/bin/env bash"), true);
        expect_eq(stripShebang(src), 19 as usize);
    });

    test("5. @cap/shebang-regex — non-leading #! & plain text return matched=false", () => {
        let a: string = " #!/bin/sh\n";
        let b: string = "echo hi\n#!/bin/sh\n";
        let c: string = "# comment line\n";
        expect_eq(hasShebang(a), false);
        expect_eq(hasShebang(b), false);
        expect_eq(hasShebang(c), false);

        let mb: ShebangMatch = matchShebang(b);
        expect_eq(mb.matched, false);
        expect_eq(mb.line_len, 0 as usize);
        expect_eq(stripShebang(b), 0 as usize);
    });

    test("6. @cap/shebang-regex — stripShebang body offset & empty/short boundaries", () => {
        let src: string = "#!/bin/zsh\nfn main() {}";
        let off: usize = stripShebang(src);
        expect_eq(off, 11 as usize);
        expect_eq(sliceEq(src, off, src.len - off, "fn main() {}"), true);

        expect_eq(hasShebang(""), false);
        expect_eq(hasShebang("#"), false);
        expect_eq(hasShebang("#!"), true);
        let m2: ShebangMatch = matchShebang("#!");
        expect_eq(m2.matched, true);
        expect_eq(m2.cmd_len, 0 as usize);
        expect_eq(stripShebang(""), 0 as usize);
    });
}
