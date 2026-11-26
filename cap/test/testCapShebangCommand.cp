import { test, expect_eq } from "cap:test";
import {
    ShebangCmd,
    shebangCommand,
    parseShebangCmd,
} from "../../packages/@cap/shebang-command/src/index.cp";

fn eqStr(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/shebang-command — #!/usr/bin/env node -> node", () => {
        let out: [64]u8 = [0; 64];
        let n: usize = shebangCommand("#!/usr/bin/env node", out[0..64]);
        expect_eq(eqStr(out[0..64], n, "node"), true);
    });

    test("2. @cap/shebang-command — #!/usr/bin/env -S python3 & flags", () => {
        let bin_buf: [64]u8 = [0; 64];
        let arg_buf: [64]u8 = [0; 64];
        let n: usize = shebangCommand("#!/usr/bin/env -S python3", bin_buf[0..64]);
        expect_eq(eqStr(bin_buf[0..64], n, "python3"), true);

        let res: ShebangCmd = parseShebangCmd(
            "#!/usr/bin/env -S node --experimental",
            bin_buf[0..64],
            arg_buf[0..64]
        );
        expect_eq(res.ok, true);
        expect_eq(eqStr(bin_buf[0..64], res.bin_len, "node"), true);
        expect_eq(eqStr(arg_buf[0..64], res.arg_len, "--experimental"), true);
    });

    test("3. @cap/shebang-command — #!/bin/bash & /usr/local/bin/python3", () => {
        let out: [64]u8 = [0; 64];
        let n1: usize = shebangCommand("#!/bin/bash", out[0..64]);
        expect_eq(eqStr(out[0..64], n1, "bash"), true);

        let n2: usize = shebangCommand("#!/usr/local/bin/python3", out[0..64]);
        expect_eq(eqStr(out[0..64], n2, "python3"), true);
    });

    test("4. @cap/shebang-command — #!/bin/sh -e extracts bin & arg", () => {
        let bin_buf: [64]u8 = [0; 64];
        let arg_buf: [64]u8 = [0; 64];
        let res: ShebangCmd = parseShebangCmd("#!/bin/sh -e", bin_buf[0..64], arg_buf[0..64]);
        expect_eq(res.ok, true);
        expect_eq(eqStr(bin_buf[0..64], res.bin_len, "sh"), true);
        expect_eq(eqStr(arg_buf[0..64], res.arg_len, "-e"), true);
    });

    test("5. @cap/shebang-command — multiline script parses only first line", () => {
        let script: string = "#!/usr/bin/env ruby\r\nputs 'hello'\n#!/bin/sh\n";
        let out: [64]u8 = [0; 64];
        let n: usize = shebangCommand(script, out[0..64]);
        expect_eq(eqStr(out[0..64], n, "ruby"), true);
    });

    test("6. @cap/shebang-command — no shebang, empty string & small buffer safety", () => {
        let bin_buf: [64]u8 = [0; 64];
        let arg_buf: [64]u8 = [0; 64];
        expect_eq(shebangCommand("", bin_buf[0..64]), 0 as usize);
        expect_eq(shebangCommand("console.log(1);", bin_buf[0..64]), 0 as usize);
        expect_eq(shebangCommand("#!   \n", bin_buf[0..64]), 0 as usize);

        let res: ShebangCmd = parseShebangCmd("plain text", bin_buf[0..64], arg_buf[0..64]);
        expect_eq(res.ok, false);
        expect_eq(res.bin_len, 0 as usize);

        let tiny: [2]u8 = [0; 2];
        expect_eq(shebangCommand("#!/usr/bin/env node", tiny[0..2]), 0 as usize);
    });
}
