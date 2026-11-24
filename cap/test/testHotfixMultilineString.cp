// Regression test for task-bun-20261124-multiline-string:
// Zig-style multiline string literals (`\\line1\n\\line2\n\\line3`) must be
// merged into a single string literal joined by '\n' rather than split into
// separate statements.

pub fn getHelp(): []const u8 {
    const s =
        \\line1
        \\line2
        \\line3
    ;
    return s;
}

pub fn getEscaped(): []const u8 {
    const s =
        \\say "hello"
        \\
        \\path\to\file
    ;
    return s;
}

pub fn getSingle(): []const u8 {
    const s =
        \\only-one
    ;
    return s;
}

fn main() {
    const h = getHelp();
    if (!std.mem.eql(u8, h, "line1\nline2\nline3")) {
        std.debug.panic("multiline string mismatch: got '{s}'", .{h});
    }
    const e = getEscaped();
    if (!std.mem.eql(u8, e, "say \"hello\"\n\npath\\to\\file")) {
        std.debug.panic("escaped multiline string mismatch: got '{s}'", .{e});
    }
    const s = getSingle();
    if (!std.mem.eql(u8, s, "only-one")) {
        std.debug.panic("single-line multiline string mismatch: got '{s}'", .{s});
    }
}
