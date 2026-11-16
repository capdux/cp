// ============================================================================
// Absorbed from Bun: bun/src/shell/
// Upstream Sync: Refer to bun/src/shell/ for future upstream updates.
// ============================================================================
// lib/shell/index.cp
// Unified Public Exports for CP Shell Engine (cap:shell)
// Absorbed from Bun Shell architecture with pure CP semantics

import { Parser } from "./parser.cp";
import { Executor, ShellResult } from "./exec.cp";

export * from "./lexer.cp";
export * from "./parser.cp";
export * from "./builtin.cp";
export * from "./exec.cp";

// Quick synchronous shell execution helper: sh("echo hello | cat")
pub fn sh(command_str: []const u8): ShellResult {
    let parser = Parser.init(command_str);
    const script = parser.parseScript();
    return Executor.executeScript(script);
}

pub struct Shell {
    pub static fn run(command_str: []const u8): ShellResult {
        return sh(command_str);
    }
}
