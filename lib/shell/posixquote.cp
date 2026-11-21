// Source: cp/lib/shell_parser/quote.cp (pure CP POSIX shell safe argument quoting)
const std = @import("std");
const Allocator = std.mem.Allocator;

fn isSafeChar(c: u8): bool {
    if ((c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or (c >= '0' and c <= '9')) {
        return true;
    }
    return c == '_' or c == '@' or c == '%' or c == '+' or c == '=' or c == ':' or c == ',' or c == '.' or c == '/' or c == '-';
}

pub fn needsQuoting(arg: []const u8): bool {
    if (arg.len == 0) {
        return true;
    }
    for (c of arg) {
        if (!isSafeChar(c)) {
            return true;
        }
    }
    return false;
}

pub fn quoteArg(alloc: Allocator, arg: []const u8): ![]u8 {
    if (arg.len == 0) {
        return try alloc.dupe(u8, "''");
    }
    if (!needsQuoting(arg)) {
        return try alloc.dupe(u8, arg);
    }

    let buf = std.ArrayList(u8).empty;
    errdefer buf.deinit(alloc);

    try buf.append(alloc, '\'');
    for (c of arg) {
        if (c == '\'') {
            try buf.appendSlice(alloc, "'\\''");
        } else {
            try buf.append(alloc, c);
        }
    }
    try buf.append(alloc, '\'');

    return try buf.toOwnedSlice(alloc);
}

pub fn joinCommand(alloc: Allocator, argv: []const []const u8): ![]u8 {
    let buf = std.ArrayList(u8).empty;
    errdefer buf.deinit(alloc);

    let i: usize = 0;
    while (i < argv.len) {
        if (i > 0) {
            try buf.append(alloc, ' ');
        }
        const quoted = try quoteArg(alloc, argv[i]);
        defer alloc.free(quoted);
        try buf.appendSlice(alloc, quoted);
        i += 1;
    }

    return try buf.toOwnedSlice(alloc);
}
