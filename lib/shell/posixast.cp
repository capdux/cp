// Source: cp/lib/shell_parser/ast.cp (pure CP POSIX shell command & pipeline AST parser)
const std = @import("std");
const Allocator = std.mem.Allocator;
const lexer = @import("./posixlexer.cp");
const ShellError = lexer.ShellError;
const TokenKind = lexer.TokenKind;
const ShellToken = lexer.ShellToken;
const tokenize = lexer.tokenize;
const deinitTokens = lexer.deinitTokens;

pub const RedirectKind = enum {
    Out,
    Append,
    ErrOut,
    ErrAppend,
    In,
    ErrToOut,
    BothOut,
};

pub const Redirect = struct {
    kind: RedirectKind,
    target: []const u8,

    pub fn deinit(self: *Redirect, alloc: Allocator) void {
        if (self.target.len > 0) {
            alloc.free(self.target);
        }
    }
};

pub const Assignment = struct {
    name: []const u8,
    value: []const u8,

    pub fn deinit(self: *Assignment, alloc: Allocator) void {
        if (self.name.len > 0) {
            alloc.free(self.name);
        }
        if (self.value.len > 0) {
            alloc.free(self.value);
        }
    }
};

pub const SimpleCommand = struct {
    assignments: []Assignment,
    argv: [][]const u8,
    redirects: []Redirect,

    pub fn deinit(self: *SimpleCommand, alloc: Allocator) void {
        for (*a of self.assignments) {
            a.deinit(alloc);
        }
        if (self.assignments.len > 0) {
            alloc.free(self.assignments);
        }
        for (arg of self.argv) {
            if (arg.len > 0) {
                alloc.free(arg);
            }
        }
        if (self.argv.len > 0) {
            alloc.free(self.argv);
        }
        for (*r of self.redirects) {
            r.deinit(alloc);
        }
        if (self.redirects.len > 0) {
            alloc.free(self.redirects);
        }
    }
};

pub const Pipeline = struct {
    commands: []SimpleCommand,
    background: bool,

    pub fn deinit(self: *Pipeline, alloc: Allocator) void {
        for (*cmd of self.commands) {
            cmd.deinit(alloc);
        }
        if (self.commands.len > 0) {
            alloc.free(self.commands);
        }
    }
};

pub const ListOp = enum {
    Seq,
    And,
    Or,
};

pub const ListItem = struct {
    pipeline: Pipeline,
    op: ListOp?,

    pub fn deinit(self: *ListItem, alloc: Allocator) void {
        self.pipeline.deinit(alloc);
    }
};

pub const ShellScript = struct {
    items: []ListItem,

    pub fn deinit(self: *ShellScript, alloc: Allocator) void {
        for (*item of self.items) {
            item.deinit(alloc);
        }
        if (self.items.len > 0) {
            alloc.free(self.items);
        }
    }
};

fn isValidEnvName(name: []const u8): bool {
    if (name.len == 0) {
        return false;
    }
    const first = name[0];
    if (!((first >= 'a' and first <= 'z') or (first >= 'A' and first <= 'Z') or first == '_')) {
        return false;
    }
    let i: usize = 1;
    while (i < name.len) {
        const c = name[i];
        if (!((c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or (c >= '0' and c <= '9') or c == '_')) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn mapRedirectKind(kind: TokenKind): RedirectKind? {
    return switch (kind) {
        .RedirectOut => .Out,
        .RedirectAppend => .Append,
        .RedirectErrOut => .ErrOut,
        .RedirectErrAppend => .ErrAppend,
        .RedirectIn => .In,
        .RedirectErrToOut => .ErrToOut,
        .RedirectBothOut => .BothOut,
        else => null,
    };
}

fn isListSeparator(kind: TokenKind): bool {
    return kind == .Semi or kind == .Background or kind == .And or kind == .Or;
}

type Str = []const u8;

fn parseSimpleCommand(alloc: Allocator, tokens: []const ShellToken, idx: *usize): ShellError!SimpleCommand {
    let assignments = std.ArrayList(Assignment).empty;
    errdefer {
        for (*a of assignments.items) {
            a.deinit(alloc);
        }
        assignments.deinit(alloc);
    }

    let argv = std.ArrayList(Str).empty;
    errdefer {
        for (arg of argv.items) {
            if (arg.len > 0) {
                alloc.free(arg);
            }
        }
        argv.deinit(alloc);
    }

    let redirects = std.ArrayList(Redirect).empty;
    errdefer {
        for (*r of redirects.items) {
            r.deinit(alloc);
        }
        redirects.deinit(alloc);
    }

    let parsed_any = false;
    while (idx.* < tokens.len) {
        const tok = tokens[idx.*];
        if (tok.kind == .Pipe or isListSeparator(tok.kind)) {
            break;
        }

        if (rkind := mapRedirectKind(tok.kind)) {
            parsed_any = true;
            idx.* += 1;
            if (rkind == .ErrToOut) {
                try redirects.append(alloc, Redirect{
                    .kind = .ErrToOut,
                    .target = "",
                });
                continue;
            }
            if (idx.* >= tokens.len or tokens[idx.*].kind != .Word) {
                return error.UnexpectedToken;
            }
            const target = try alloc.dupe(u8, tokens[idx.*].value);
            errdefer alloc.free(target);
            try redirects.append(alloc, Redirect{
                .kind = rkind,
                .target = target,
            });
            idx.* += 1;
            continue;
        }

        if (tok.kind == .Word) {
            parsed_any = true;
            if (argv.items.len == 0) {
                if (eq_idx := std.mem.indexOfScalar(u8, tok.value, '=')) {
                    const name_part = tok.value[0..eq_idx];
                    if (isValidEnvName(name_part)) {
                        const val_part = tok.value[eq_idx + 1 ..];
                        const name_dup = try alloc.dupe(u8, name_part);
                        errdefer alloc.free(name_dup);
                        const val_dup = try alloc.dupe(u8, val_part);
                        errdefer alloc.free(val_dup);
                        try assignments.append(alloc, Assignment{
                            .name = name_dup,
                            .value = val_dup,
                        });
                        idx.* += 1;
                        continue;
                    }
                }
            }
            const arg_dup = try alloc.dupe(u8, tok.value);
            errdefer alloc.free(arg_dup);
            try argv.append(alloc, arg_dup);
            idx.* += 1;
            continue;
        }

        return error.UnexpectedToken;
    }

    if (!parsed_any) {
        return error.UnexpectedToken;
    }

    return SimpleCommand{
        .assignments = try assignments.toOwnedSlice(alloc),
        .argv = try argv.toOwnedSlice(alloc),
        .redirects = try redirects.toOwnedSlice(alloc),
    };
}

pub fn parseScript(alloc: Allocator, input: []const u8): ShellError!ShellScript {
    const tokens = try tokenize(alloc, input);
    defer deinitTokens(alloc, tokens);

    let items = std.ArrayList(ListItem).empty;
    errdefer {
        for (*item of items.items) {
            item.deinit(alloc);
        }
        items.deinit(alloc);
    }

    let idx: usize = 0;
    while (idx < tokens.len) {
        let commands = std.ArrayList(SimpleCommand).empty;
        errdefer {
            for (*cmd of commands.items) {
                cmd.deinit(alloc);
            }
            commands.deinit(alloc);
        }

        const first_cmd = try parseSimpleCommand(alloc, tokens, &idx);
        try commands.append(alloc, first_cmd);

        while (idx < tokens.len and tokens[idx].kind == .Pipe) {
            idx += 1;
            if (idx >= tokens.len) {
                return error.UnexpectedToken;
            }
            const next_cmd = try parseSimpleCommand(alloc, tokens, &idx);
            try commands.append(alloc, next_cmd);
        }

        let background = false;
        let list_op: ListOp? = null;

        if (idx < tokens.len) {
            const sep = tokens[idx].kind;
            if (sep == .Semi) {
                list_op = .Seq;
                idx += 1;
            } else if (sep == .Background) {
                background = true;
                list_op = .Seq;
                idx += 1;
            } else if (sep == .And) {
                list_op = .And;
                idx += 1;
                if (idx >= tokens.len) {
                    return error.UnexpectedToken;
                }
            } else if (sep == .Or) {
                list_op = .Or;
                idx += 1;
                if (idx >= tokens.len) {
                    return error.UnexpectedToken;
                }
            } else {
                return error.UnexpectedToken;
            }
        }

        const cmd_slice = try commands.toOwnedSlice(alloc);
        try items.append(alloc, ListItem{
            .pipeline = Pipeline{
                .commands = cmd_slice,
                .background = background,
            },
            .op = list_op,
        });
    }

    return ShellScript{
        .items = try items.toOwnedSlice(alloc),
    };
}
