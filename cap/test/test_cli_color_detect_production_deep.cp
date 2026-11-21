import {
    ColorLevel,
    ColorEnv,
    parseForceColor,
    detectColorLevel,
    supportsColor,
    supportsTruecolor,
} from "cap:cli";
import { test, expect } from "cap:test";

test("FORCE_COLOR and NO_COLOR precedence", () => {
    let env = ColorEnv.empty(false);
    expect(detectColorLevel(env) == ColorLevel.none);
    expect(!supportsColor(env));

    env.force_color = "3";
    env.no_color = "1";
    expect(detectColorLevel(env) == ColorLevel.truecolor);
    expect(supportsTruecolor(env));

    env.force_color = "0";
    expect(detectColorLevel(env) == ColorLevel.none);

    env.force_color = null;
    env.is_tty = true;
    env.no_color = "1";
    expect(detectColorLevel(env) == ColorLevel.none);

    // Empty NO_COLOR does NOT disable color per no-color.org
    env.no_color = "";
    env.colorterm = "truecolor";
    expect(detectColorLevel(env) == ColorLevel.truecolor);
});

test("TERM, TERM_PROGRAM, WT_SESSION, CLICOLOR, and CI detection", () => {
    let e1 = ColorEnv.empty(true);
    e1.term = "xterm-256color";
    expect(detectColorLevel(e1) == ColorLevel.ansi256);

    let e2 = ColorEnv.empty(true);
    e2.term = "dumb";
    expect(detectColorLevel(e2) == ColorLevel.none);

    let e3 = ColorEnv.empty(true);
    e3.term_program = "Apple_Terminal";
    expect(detectColorLevel(e3) == ColorLevel.ansi256);

    let e4 = ColorEnv.empty(true);
    e4.wt_session = "abc-123";
    expect(detectColorLevel(e4) == ColorLevel.truecolor);

    let e5 = ColorEnv.empty(false);
    e5.ci = "true";
    expect(detectColorLevel(e5) == ColorLevel.basic16);

    expect(parseForceColor("2").? == ColorLevel.ansi256);
    expect(parseForceColor("false").? == ColorLevel.none);
});
