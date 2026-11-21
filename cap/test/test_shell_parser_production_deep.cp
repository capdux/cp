// Source: cp/cap/test/test_shell_parser_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    ShellError,
    PosixTokenKind,
    deinitTokens,
    tokenize,
    RedirectKind,
    ListOp,
    parseScript,
    posixNeedsQuoting,
    quoteArg,
    joinCommand,
} from "cap:shell";

test("shell_parser: lexer quotes, escapes, vars and assignments") {
    const alloc = std.testing.allocator;
    const input = "FOO=bar BAZ=\"hello \\\"world\\\" $USER\" 'a b' c\\ d # comment\n";
    const tokens = try tokenize(alloc, input);
    defer deinitTokens(alloc, tokens);

    try expect_eq(tokens.len, 4);
    try expect_eq(tokens[0].kind, PosixTokenKind.Word);
    try expect_eq(std.mem.eql(u8, tokens[0].value, "FOO=bar"), true);
    try expect_eq(tokens[0].quoted, false);
    try expect_eq(tokens[0].has_var, false);

    try expect_eq(std.mem.eql(u8, tokens[1].value, "BAZ=hello \"world\" $USER"), true);
    try expect_eq(tokens[1].quoted, true);
    try expect_eq(tokens[1].has_var, true);

    try expect_eq(std.mem.eql(u8, tokens[2].value, "a b"), true);
    try expect_eq(tokens[2].quoted, true);
    try expect_eq(tokens[2].has_var, false);

    try expect_eq(std.mem.eql(u8, tokens[3].value, "c d"), true);
    try expect_eq(tokens[3].quoted, false);
}

test("shell_parser: AST pipeline, redirects and list operators") {
    const alloc = std.testing.allocator;
    const input = "FOO=bar BAZ=\"1 2\" cmd arg1 > out.txt 2>&1 | grep ok && echo done || echo fail; sleep 1 &";
    let script = try parseScript(alloc, input);
    defer script.deinit(alloc);

    try expect_eq(script.items.len, 4);

    // Item 0: FOO=bar BAZ="1 2" cmd arg1 > out.txt 2>&1 | grep ok &&
    const item0 = script.items[0];
    try expect_eq(item0.op.?, ListOp.And);
    try expect_eq(item0.pipeline.background, false);
    try expect_eq(item0.pipeline.commands.len, 2);

    const cmd0 = item0.pipeline.commands[0];
    try expect_eq(cmd0.assignments.len, 2);
    try expect_eq(std.mem.eql(u8, cmd0.assignments[0].name, "FOO"), true);
    try expect_eq(std.mem.eql(u8, cmd0.assignments[0].value, "bar"), true);
    try expect_eq(std.mem.eql(u8, cmd0.assignments[1].name, "BAZ"), true);
    try expect_eq(std.mem.eql(u8, cmd0.assignments[1].value, "1 2"), true);

    try expect_eq(cmd0.argv.len, 2);
    try expect_eq(std.mem.eql(u8, cmd0.argv[0], "cmd"), true);
    try expect_eq(std.mem.eql(u8, cmd0.argv[1], "arg1"), true);

    try expect_eq(cmd0.redirects.len, 2);
    try expect_eq(cmd0.redirects[0].kind, RedirectKind.Out);
    try expect_eq(std.mem.eql(u8, cmd0.redirects[0].target, "out.txt"), true);
    try expect_eq(cmd0.redirects[1].kind, RedirectKind.ErrToOut);

    const cmd1 = item0.pipeline.commands[1];
    try expect_eq(cmd1.argv.len, 2);
    try expect_eq(std.mem.eql(u8, cmd1.argv[0], "grep"), true);
    try expect_eq(std.mem.eql(u8, cmd1.argv[1], "ok"), true);

    // Item 1: echo done ||
    try expect_eq(script.items[1].op.?, ListOp.Or);
    try expect_eq(std.mem.eql(u8, script.items[1].pipeline.commands[0].argv[1], "done"), true);

    // Item 2: echo fail;
    try expect_eq(script.items[2].op.?, ListOp.Seq);
    try expect_eq(std.mem.eql(u8, script.items[2].pipeline.commands[0].argv[1], "fail"), true);

    // Item 3: sleep 1 &
    try expect_eq(script.items[3].op.?, ListOp.Seq);
    try expect_eq(script.items[3].pipeline.background, true);
    try expect_eq(std.mem.eql(u8, script.items[3].pipeline.commands[0].argv[0], "sleep"), true);
}

test("shell_parser: error handling for quotes, trailing escape and invalid syntax") {
    const alloc = std.testing.allocator;

    try std.testing.expectError(ShellError.UnterminatedQuote, tokenize(alloc, "echo 'unclosed"));
    try std.testing.expectError(ShellError.UnterminatedQuote, tokenize(alloc, "echo \"unclosed"));
    try std.testing.expectError(ShellError.TrailingEscape, tokenize(alloc, "echo hello\\"));

    try std.testing.expectError(ShellError.UnexpectedToken, parseScript(alloc, "a | | b"));
    try std.testing.expectError(ShellError.UnexpectedToken, parseScript(alloc, "| a"));
    try std.testing.expectError(ShellError.UnexpectedToken, parseScript(alloc, "a &&"));
    try std.testing.expectError(ShellError.UnexpectedToken, parseScript(alloc, "echo >"));
}

test("shell_parser: quoteArg and joinCommand roundtrip") {
    const alloc = std.testing.allocator;

    try expect_eq(posixNeedsQuoting("safe_arg-1.txt"), false);
    try expect_eq(posixNeedsQuoting("a b"), true);
    try expect_eq(posixNeedsQuoting(""), true);

    const q_empty = try quoteArg(alloc, "");
    defer alloc.free(q_empty);
    try expect_eq(std.mem.eql(u8, q_empty, "''"), true);

    const q_inject = try quoteArg(alloc, "it's $HOME; rm -rf /");
    defer alloc.free(q_inject);
    try expect_eq(std.mem.eql(u8, q_inject, "'it'\\''s $HOME; rm -rf /'"), true);

    const args: []const []const u8 = [ "echo", "hello world", "it's", "" ];
    const joined = try joinCommand(alloc, args);
    defer alloc.free(joined);
    try expect_eq(std.mem.eql(u8, joined, "echo 'hello world' 'it'\\''s' ''"), true);

    const tokens = try tokenize(alloc, joined);
    defer deinitTokens(alloc, tokens);
    try expect_eq(tokens.len, 4);
    try expect_eq(std.mem.eql(u8, tokens[0].value, "echo"), true);
    try expect_eq(std.mem.eql(u8, tokens[1].value, "hello world"), true);
    try expect_eq(std.mem.eql(u8, tokens[2].value, "it's"), true);
    try expect_eq(std.mem.eql(u8, tokens[3].value, ""), true);
}

test("shell_parser: cap.shell_parser dynamic mount and zero-leak check") {
    const alloc = std.testing.allocator;
    let script = try cap.shell.parseScript(alloc, "cat < in.txt >> out.log 2>> err.log &> all.log");
    defer script.deinit(alloc);

    try expect_eq(script.items.len, 1);
    const cmd = script.items[0].pipeline.commands[0];
    try expect_eq(cmd.redirects.len, 4);
    try expect_eq(cmd.redirects[0].kind, RedirectKind.In);
    try expect_eq(cmd.redirects[1].kind, RedirectKind.Append);
    try expect_eq(cmd.redirects[2].kind, RedirectKind.ErrAppend);
    try expect_eq(cmd.redirects[3].kind, RedirectKind.BothOut);

    const q = try cap.shell.quoteArg(alloc, "a b");
    defer alloc.free(q);
    try expect_eq(std.mem.eql(u8, q, "'a b'"), true);
}
