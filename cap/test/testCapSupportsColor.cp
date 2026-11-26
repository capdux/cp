import { test, expect_eq } from "cap:test";
import {
    COLOR_LEVEL_NONE,
    COLOR_LEVEL_BASIC,
    COLOR_LEVEL_256,
    COLOR_LEVEL_16M,
    ColorSupport,
    TerminalEnv,
    translateLevel,
    detectColorLevel,
    detectColorSupport,
} from "../../packages/@cap/supports-color/src/index.cp";

fn main() {
    test("1. @cap/supports-color — CLI flags highest priority & -- terminator", () => {
        let env: TerminalEnv = TerminalEnv.empty();
        env.no_color = "1"; // Even with NO_COLOR + non-TTY, CLI flag wins

        let a16m: [2]string = ["app", "--color=16m"];
        expect_eq(detectColorLevel(env, &a16m), COLOR_LEVEL_16M);

        let a256: [2]string = ["app", "--color=256"];
        expect_eq(detectColorLevel(env, &a256), COLOR_LEVEL_256);

        let a_basic: [2]string = ["app", "--color"];
        expect_eq(detectColorLevel(env, &a_basic), COLOR_LEVEL_BASIC);

        let tty_env: TerminalEnv = TerminalEnv.tty();
        tty_env.force_color = "3";
        let a_no: [2]string = ["app", "--no-color"];
        expect_eq(detectColorLevel(tty_env, &a_no), COLOR_LEVEL_NONE);

        // Flag after `--` end-of-options terminator must be ignored
        let a_after_dd: [3]string = ["app", "--", "--color=16m"];
        expect_eq(detectColorLevel(env, &a_after_dd), COLOR_LEVEL_NONE);
    });

    test("2. @cap/supports-color — FORCE_COLOR (0/1/2/3) and NO_COLOR precedence", () => {
        let dummy: [1]string = [""];
        let env: TerminalEnv = TerminalEnv.empty();
        env.no_color = "1";

        env.force_color = "0";
        expect_eq(detectColorLevel(env, dummy[0..0]), COLOR_LEVEL_NONE);

        env.force_color = "1";
        expect_eq(detectColorLevel(env, dummy[0..0]), COLOR_LEVEL_BASIC);

        env.force_color = "2";
        expect_eq(detectColorLevel(env, dummy[0..0]), COLOR_LEVEL_256);

        env.force_color = "3";
        expect_eq(detectColorLevel(env, dummy[0..0]), COLOR_LEVEL_16M);

        // Without FORCE_COLOR, NO_COLOR disables even on truecolor TTY
        let env2: TerminalEnv = TerminalEnv.tty();
        env2.colorterm = "truecolor";
        env2.no_color = "1";
        expect_eq(detectColorLevel(env2, dummy[0..0]), COLOR_LEVEL_NONE);
    });

    test("3. @cap/supports-color — Non-TTY and TERM=dumb degrade to level 0", () => {
        let dummy: [1]string = [""];

        let non_tty: TerminalEnv = TerminalEnv.empty();
        non_tty.colorterm = "truecolor";
        non_tty.term = "xterm-256color";
        expect_eq(detectColorLevel(non_tty, dummy[0..0]), COLOR_LEVEL_NONE);

        let dumb: TerminalEnv = TerminalEnv.tty();
        dumb.term = "dumb";
        dumb.colorterm = "truecolor";
        expect_eq(detectColorLevel(dumb, dummy[0..0]), COLOR_LEVEL_NONE);
    });

    test("4. @cap/supports-color — COLORTERM=truecolor, TERM_PROGRAM, and xterm-256color", () => {
        let dummy: [1]string = [""];

        let e_tc: TerminalEnv = TerminalEnv.tty();
        e_tc.colorterm = "truecolor";
        expect_eq(detectColorLevel(e_tc, dummy[0..0]), COLOR_LEVEL_16M);

        let e_256: TerminalEnv = TerminalEnv.tty();
        e_256.term = "xterm-256color";
        expect_eq(detectColorLevel(e_256, dummy[0..0]), COLOR_LEVEL_256);

        let e_iterm: TerminalEnv = TerminalEnv.tty();
        e_iterm.term_program = "iTerm.app";
        expect_eq(detectColorLevel(e_iterm, dummy[0..0]), COLOR_LEVEL_16M);

        let e_apple: TerminalEnv = TerminalEnv.tty();
        e_apple.term_program = "Apple_Terminal";
        expect_eq(detectColorLevel(e_apple, dummy[0..0]), COLOR_LEVEL_256);

        let e_vt: TerminalEnv = TerminalEnv.tty();
        e_vt.term = "vt100";
        expect_eq(detectColorLevel(e_vt, dummy[0..0]), COLOR_LEVEL_BASIC);
    });

    test("5. @cap/supports-color — CI environments and Windows 10 build detection", () => {
        let dummy: [1]string = [""];

        let e_gh: TerminalEnv = TerminalEnv.tty();
        e_gh.ci = "true";
        e_gh.ci_name = "GITHUB_ACTIONS";
        expect_eq(detectColorLevel(e_gh, dummy[0..0]), COLOR_LEVEL_16M);

        let e_gl: TerminalEnv = TerminalEnv.tty();
        e_gl.ci = "true";
        e_gl.ci_name = "GITLAB_CI";
        expect_eq(detectColorLevel(e_gl, dummy[0..0]), COLOR_LEVEL_BASIC);

        let e_win16m: TerminalEnv = TerminalEnv.tty();
        e_win16m.is_windows = true;
        e_win16m.win_build = 19045;
        expect_eq(detectColorLevel(e_win16m, dummy[0..0]), COLOR_LEVEL_16M);

        let e_win256: TerminalEnv = TerminalEnv.tty();
        e_win256.is_windows = true;
        e_win256.win_build = 10586;
        expect_eq(detectColorLevel(e_win256, dummy[0..0]), COLOR_LEVEL_256);

        let e_win16: TerminalEnv = TerminalEnv.tty();
        e_win16.is_windows = true;
        e_win16.win_build = 9600;
        expect_eq(detectColorLevel(e_win16, dummy[0..0]), COLOR_LEVEL_BASIC);
    });

    test("6. @cap/supports-color — translateLevel and detectColorSupport boolean consistency", () => {
        const s0: ColorSupport = translateLevel(0);
        expect_eq(s0.level, 0);
        expect_eq(s0.has_basic, false);
        expect_eq(s0.has_256, false);
        expect_eq(s0.has_16m, false);

        const s1: ColorSupport = translateLevel(1);
        expect_eq(s1.has_basic, true);
        expect_eq(s1.has_256, false);
        expect_eq(s1.has_16m, false);

        const s2: ColorSupport = translateLevel(2);
        expect_eq(s2.has_basic, true);
        expect_eq(s2.has_256, true);
        expect_eq(s2.has_16m, false);

        let env: TerminalEnv = TerminalEnv.tty();
        env.colorterm = "24bit";
        let dummy: [1]string = [""];
        const s3: ColorSupport = detectColorSupport(env, dummy[0..0]);
        expect_eq(s3.level, 3);
        expect_eq(s3.has_basic, true);
        expect_eq(s3.has_256, true);
        expect_eq(s3.has_16m, true);
    });
}
