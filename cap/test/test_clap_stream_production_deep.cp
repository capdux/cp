import { test, expect } from "cap:test";
import {
    TakesValue,
    ParamSpec,
    ParseError,
    StreamingClap,
} from "cap:clap_stream";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("clap_stream: long, chained short, inline values, and -- positional mode", () => {
    const specs: [5]ParamSpec = [
        ParamSpec{ id: 1, short: 'v', long: "verbose", takes_value: TakesValue.none, is_positional: false },
        ParamSpec{ id: 2, short: 'f', long: "force", takes_value: TakesValue.none, is_positional: false },
        ParamSpec{ id: 3, short: 'o', long: "output", takes_value: TakesValue.one, is_positional: false },
        ParamSpec{ id: 4, short: 'c', long: "color", takes_value: TakesValue.one_optional, is_positional: false },
        ParamSpec{ id: 5, short: 0, long: "", takes_value: TakesValue.one, is_positional: true },
    ];

    const argv: [8][]const u8 = [
        "-vf",
        "-oout.bin",
        "--output=final.bin",
        "--color",
        "--color=always",
        "src/main.cp",
        "--",
        "--not-a-flag",
    ];

    let parser = StreamingClap.init(specs[0..], argv[0..]);

    const a1 = parser.next().?;
    expect(a1.isOk() && a1.id == 1 && a1.value == null);

    const a2 = parser.next().?;
    expect(a2.isOk() && a2.id == 2 && a2.value == null);

    const a3 = parser.next().?;
    expect(a3.isOk() && a3.id == 3 && strEq(a3.value.?, "out.bin"));

    const a4 = parser.next().?;
    expect(a4.isOk() && a4.id == 3 && strEq(a4.value.?, "final.bin"));

    const a5 = parser.next().?;
    expect(a5.isOk() && a5.id == 4 && a5.value == null);

    const a6 = parser.next().?;
    expect(a6.isOk() && a6.id == 4 && strEq(a6.value.?, "always"));

    const a7 = parser.next().?;
    expect(a7.isOk() && a7.id == 5 && strEq(a7.value.?, "src/main.cp"));

    const a8 = parser.next().?;
    expect(a8.isOk() && a8.id == 5 && strEq(a8.value.?, "--not-a-flag"));

    expect(parser.next() == null);
});

test("clap_stream: error diagnostics for missing value, doesnt_take_value, unknown_flag", () => {
    const specs: [2]ParamSpec = [
        ParamSpec{ id: 1, short: 'v', long: "verbose", takes_value: TakesValue.none, is_positional: false },
        ParamSpec{ id: 2, short: 'o', long: "output", takes_value: TakesValue.one, is_positional: false },
    ];

    const argv1: [1][]const u8 = ["--verbose=1"];
    let p1 = StreamingClap.init(specs[0..], argv1[0..]);
    const e1 = p1.next().?;
    expect(e1.err == ParseError.doesnt_take_value);

    const argv2: [1][]const u8 = ["--output"];
    let p2 = StreamingClap.init(specs[0..], argv2[0..]);
    const e2 = p2.next().?;
    expect(e2.err == ParseError.missing_value);

    const argv3: [1][]const u8 = ["--bogus"];
    let p3 = StreamingClap.init(specs[0..], argv3[0..]);
    const e3 = p3.next().?;
    expect(e3.err == ParseError.unknown_flag);
});
