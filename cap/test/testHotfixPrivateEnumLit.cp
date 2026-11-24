// Regression test for task-bun-20261124-private-enum-drop:
// Enum literals named after CP keywords such as `.private`, `.union`, `.defer`
// must parse cleanly as enum literals (not fail as pipeline placeholder or silently drop).

pub enum Kind {
    normal,
    @"private",
    @"priv",
    implements,
}

fn classify(k: Kind): []const u8 {
    if (k == .@"private") return "p";
    if (k == .priv) return "pv";
    if (k == .implements) return "im";
    return "n";
}

fn main() {
    if (!std.mem.eql(u8, classify(.private), "p")) {
        std.debug.panic("expected .private -> p", .{});
    }
    if (!std.mem.eql(u8, classify(.@"private"), "p")) {
        std.debug.panic("expected .@\"private\" -> p", .{});
    }
    if (!std.mem.eql(u8, classify(.priv), "pv")) {
        std.debug.panic("expected .priv -> pv", .{});
    }
    if (!std.mem.eql(u8, classify(.implements), "im")) {
        std.debug.panic("expected .implements -> im", .{});
    }
    if (!std.mem.eql(u8, classify(.normal), "n")) {
        std.debug.panic("expected .normal -> n", .{});
    }
}
