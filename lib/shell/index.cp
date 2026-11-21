// Source: /data/dev/bun/src/shell/
// ============================================================================
// Absorbed from Bun: /data/dev/bun/src/shell/ & /data/dev/bun/src/shell_parser/
// Upstream Sync: Refer to /data/dev/bun/src/shell/ for future upstream updates.
// ============================================================================
// lib/shell/index.cp
// Unified Public Exports for CP Shell Engine (cap:shell)

const std = @import("std");
import { Parser } from "./parser.cp";
import { Executor, ShellResult, ShellSession } from "./exec.cp";

export * from "./lexer.cp";
export * from "./braces.cp";
export * from "./env.cp";
export * from "./arith.cp";
export * from "./parser.cp";
export * from "./builtin_fs.cp";
export * from "./builtin_text.cp";
export * from "./builtin.cp";
export * from "./exec.cp";

export {
    ShellEnvEntry,
    ShellEnvIter,
    getShellEnv,
} from "./envparse.cp";

export {
    needsQuoting,
    quotePosix,
    joinArgs,
    ShellArgIter,
} from "./quote.cp";

export {
    BraceExpandIter,
} from "./brace.cp";

export {
    ShellError,
    ShellToken,
    deinitTokens,
    tokenize,
} from "./posixlexer.cp";
import plex from "./posixlexer.cp";
export const PosixTokenKind = plex.TokenKind;

export {
    RedirectKind,
    Assignment,
    SimpleCommand,
    ListOp,
    ListItem,
    ShellScript,
    parseScript,
} from "./posixast.cp";
import past from "./posixast.cp";
export const PosixRedirect = past.Redirect;
export const PosixPipeline = past.Pipeline;

export {
    quoteArg,
    joinCommand,
} from "./posixquote.cp";
import pquote from "./posixquote.cp";
export const posixNeedsQuoting = pquote.needsQuoting;

export {
    EnvAssignment,
    isSafeUnquoted,
    escapeDoubleQuoted,
    posixBasename,
    posixDirname,
    isValidEnvVarName,
    splitEnvAssignment,
} from "./util.cp";
import sutil from "./util.cp";
export const quotePosixBuf = sutil.quotePosix;
export const joinCommandBuf = sutil.joinCommand;

// Quick synchronous shell execution helper: sh("echo hello | cat")
pub fn sh(command_str: []const u8): ShellResult {
    let parser = Parser.init(command_str);
    let script_ptr = parser.parseScriptHeap();
    defer {
        script_ptr.destroy();
        std.heap.page_allocator.destroy(script_ptr);
    }
    let sess = ShellSession.init();
    defer sess.~init();
    return Executor.executeScriptPtrWithSession(script_ptr, &sess);
}

// Compatibility alias for exec("...")
pub fn exec(command_str: []const u8): ShellResult {
    return sh(command_str);
}

pub struct Shell {
    pub static fn run(command_str: []const u8): ShellResult {
        return sh(command_str);
    }

    pub static fn session(): ShellSession {
        return ShellSession.init();
    }
}
