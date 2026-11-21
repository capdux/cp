// Source: Absorbed from /data/dev/bun/src/clap/clap.cp | Synced: 2026-11-20T11:53:32Z
// Argument iterators for CLI argument parsing (Slice, Shell, OS)
import std from "std";


/// An example of what methods should be implemented on an arg iterator.
pub struct ExampleArgIterator {
    pub const Error = error{};

    pub fn next(*_): ([]const u8)? ! ExampleArgIterator.Error {
        return "2";
    }
}

/// An argument iterator which iterates over a slice of arguments.
/// This implementation does not allocate.
pub struct SliceIterator {
    pub const Error = error{};

    remain: []const []const u8,

    pub static fn init(args: []const []const u8): SliceIterator {
        return SliceIterator{
            remain: args,
        };
    }

    pub fn next(*_): ([]const u8)? {
        if (_.remain.len > 0) {
            const res = _.remain[0];
            _.remain = _.remain[1..];
            return res;
        }
        return null;
    }
}

/// An argument iterator which wraps process arguments.
pub struct OsIterator {
    arena_allocator: std.heap.ArenaAllocator,
    remain: []const []const u8,
    exe_arg: ([]const u8)? = null,

    pub static fn init(allocator: std.mem.Allocator): OsIterator {
        return OsIterator{
            arena_allocator: std.heap.ArenaAllocator.init(allocator),
            remain: &[],
            exe_arg: null,
        };
    }

    pub static fn initFromSlice(allocator: std.mem.Allocator, args: []const []const u8): OsIterator {
        let remaining = args;
        let exe: ([]const u8)? = null;
        if (args.len > 0) {
            exe = args[0];
            remaining = args[1..];
        }
        return OsIterator{
            arena_allocator: std.heap.ArenaAllocator.init(allocator),
            remain: remaining,
            exe_arg: exe,
        };
    }

    pub fn deinit(*_) {
        _.arena_allocator.deinit();
    }

    pub fn next(*_): ([]const u8)? {
        if (_.remain.len > 0) {
            const res = _.remain[0];
            _.remain = _.remain[1..];
            return res;
        }
        return null;
    }
}

pub enum ShellState {
    skip_whitespace,
    no_quote,
    no_quote_escape,
    single_quote,
    double_quote,
    double_quote_escape,
    after_quote,
}

pub err ShellError {
    DanglingEscape,
    QuoteNotClosed,
}

/// An argument iterator that takes a string and parses it into arguments, simulating
/// how shells split arguments.
pub struct ShellIterator {
    arena_allocator: std.heap.ArenaAllocator,
    str: []const u8,

    pub static fn init(allocator: std.mem.Allocator, str: []const u8): ShellIterator {
        return ShellIterator{
            arena_allocator: std.heap.ArenaAllocator.init(allocator),
            str: str,
        };
    }

    pub fn deinit(*_) {
        _.arena_allocator.deinit();
    }

    pub fn next(*_): ([]const u8)? ! ShellError {
        let list = std.array_list.Managed(u8).init(_.arena_allocator.allocator());
        let start: usize = 0;
        let state: ShellState = .skip_whitespace;

        let i: usize = 0;
        while (i < _.str.len) : (i += 1) {
            const c = _.str[i];
            switch (state) {
                .skip_whitespace => switch (c) {
                    ' ', '\t', '\n' => {},
                    '\'' => {
                        start = i + 1;
                        state = .single_quote;
                    },
                    '"' => {
                        start = i + 1;
                        state = .double_quote;
                    },
                    '\\' => {
                        start = i + 1;
                        state = .no_quote_escape;
                    },
                    else => {
                        start = i;
                        state = .no_quote;
                    },
                },

                .no_quote => switch (c) {
                    ' ', '\t', '\n' => {
                        defer _.str = _.str[i..];
                        return _.result(start, i, &list);
                    },
                    '\'' => {
                        list.appendSlice(_.str[start..i]) catch unreachable;
                        start = i + 1;
                        state = .single_quote;
                    },
                    '"' => {
                        list.appendSlice(_.str[start..i]) catch unreachable;
                        start = i + 1;
                        state = .double_quote;
                    },
                    '\\' => {
                        list.appendSlice(_.str[start..i]) catch unreachable;
                        start = i + 1;
                        state = .no_quote_escape;
                    },
                    else => {},
                },

                .after_quote => switch (c) {
                    ' ', '\t', '\n' => {
                        defer _.str = _.str[i..];
                        return _.result(start, i - 1, &list);
                    },
                    '\'' => {
                        list.appendSlice(_.str[start .. i - 1]) catch unreachable;
                        start = i + 1;
                        state = .single_quote;
                    },
                    '"' => {
                        list.appendSlice(_.str[start .. i - 1]) catch unreachable;
                        start = i + 1;
                        state = .double_quote;
                    },
                    '\\' => {
                        list.appendSlice(_.str[start .. i - 1]) catch unreachable;
                        start = i + 1;
                        state = .no_quote_escape;
                    },
                    else => {
                        list.appendSlice(_.str[start .. i - 1]) catch unreachable;
                        start = i;
                        state = .no_quote;
                    },
                },

                .single_quote => switch (c) {
                    '\'' => state = .after_quote,
                    else => {},
                },
                .double_quote => switch (c) {
                    '"' => state = .after_quote,
                    '\\' => {
                        list.appendSlice(_.str[start..i]) catch unreachable;
                        start = i + 1;
                        state = .double_quote_escape;
                    },
                    else => {},
                },

                .no_quote_escape => switch (c) {
                    else => state = .no_quote,
                },
                .double_quote_escape => switch (c) {
                    else => state = .double_quote,
                },
            }
        }

        defer _.str = _.str[_.str.len..];
        switch (state) {
            .skip_whitespace => return null,
            .no_quote => return _.result(start, _.str.len, &list),
            .after_quote => return _.result(start, _.str.len - 1, &list),
            .no_quote_escape => return ShellError.DanglingEscape,
            .single_quote, .double_quote, .double_quote_escape => return ShellError.QuoteNotClosed,
        }
    }

    fn result(*_, start: usize, end: usize, list: *std.array_list.Managed(u8)): ([]const u8)? ! ShellError {
        const res = _.str[start..end];
        let l = list;
        if (l.items.len != 0) {
            l.appendSlice(res) catch unreachable;
            return l.toOwnedSlice() catch unreachable;
        }
        return res;
    }
}
