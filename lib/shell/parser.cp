// Source: Custom implementation by Antigravity AI
// ============================================================================
// Absorbed from Bun: bun/src/shell/parser.cp
// Upstream Sync: Refer to bun/src/shell/parser.zig for future upstream updates.
// ============================================================================
// lib/shell/parser.cp
// AST nodes and parser for CP Shell (cap:shell)
// Absorbed from Bun Shell parser architecture with pure CP semantics

import { Lexer, Token, TokenKind } from "./lexer.cp";

pub enum RedirectOp {
    Out,
    Append,
    In,
}

pub struct Redirect {
    op: RedirectOp = .Out,
    target: []const u8 = "",
}

pub const MAX_COMMAND_ARGS: usize = 32;
pub const MAX_PIPELINE_CMDS: usize = 16;
pub const MAX_STATEMENTS: usize = 16;

pub struct Command {
    args: [MAX_COMMAND_ARGS][]const u8 = undefined,
    args_len: usize = 0,
    redirect_out: []const u8 = "",
    redirect_append: bool = false,
    redirect_in: []const u8 = "",

    pub static fn init(): Command {
        return Command {
            args: undefined,
            args_len: 0,
            redirect_out: "",
            redirect_append: false,
            redirect_in: "",
        };
    }

    pub fn addArg(*_, arg: []const u8): bool {
        if (_.args_len >= MAX_COMMAND_ARGS) {
            return false;
        }
        _.args[_.args_len] = arg;
        _.args_len += 1;
        return true;
    }

    pub fn argv(): []const []const u8 {
        return _.args[0.._.args_len];
    }
}

pub struct Pipeline {
    commands: [MAX_PIPELINE_CMDS]Command = undefined,
    commands_len: usize = 0,

    pub static fn init(): Pipeline {
        return Pipeline {
            commands: undefined,
            commands_len: 0,
        };
    }

    pub fn addCommand(*_, cmd: Command): bool {
        if (_.commands_len >= MAX_PIPELINE_CMDS) {
            return false;
        }
        _.commands[_.commands_len] = cmd;
        _.commands_len += 1;
        return true;
    }

    pub fn cmds(): []const Command {
        return _.commands[0.._.commands_len];
    }
}

pub enum ControlOp {
    None,
    And,      // &&
    Or,       // ||
    Sequence, // ;
}

pub struct Statement {
    pipeline: Pipeline = Pipeline.init(),
    op: ControlOp = .None,
}

pub struct Script {
    statements: [MAX_STATEMENTS]Statement = undefined,
    statements_len: usize = 0,

    pub static fn init(): Script {
        return Script {
            statements: undefined,
            statements_len: 0,
        };
    }

    pub fn addStatement(*_, stmt: Statement): bool {
        if (_.statements_len >= MAX_STATEMENTS) {
            return false;
        }
        _.statements[_.statements_len] = stmt;
        _.statements_len += 1;
        return true;
    }

    pub fn stmts(): []const Statement {
        return _.statements[0.._.statements_len];
    }
}

pub struct Parser {
    lexer: Lexer = Lexer.init(""),
    peeked: Token? = null,

    pub static fn init(source: []const u8): Parser {
        return Parser {
            lexer: Lexer.init(source),
            peeked: null,
        };
    }

    pub fn next(*_): Token {
        if (p := _.peeked) {
            _.peeked = null;
            return p;
        }
        return _.lexer.nextToken();
    }

    pub fn peek(*_): Token {
        if (p := _.peeked) {
            return p;
        }
        const tok = _.lexer.nextToken();
        _.peeked = tok;
        return tok;
    }

    pub fn parseScript(*_): Script {
        let script = Script.init();

        while (true) {
            const tok = _.peek();
            if (tok.kind == .Eof) {
                break;
            }

            let pipe = Pipeline.init();
            let cmd = Command.init();

            while (true) {
                const t = _.peek();
                if (t.kind == .Word) {
                    * = _.next();
                    cmd.addArg(t.slice);
                } else if (t.kind == .RedirectOut) {
                    * = _.next();
                    const target_tok = _.next();
                    if (target_tok.kind == .Word) {
                        cmd.redirect_out = target_tok.slice;
                        cmd.redirect_append = false;
                    }
                } else if (t.kind == .RedirectAppend) {
                    * = _.next();
                    const target_tok = _.next();
                    if (target_tok.kind == .Word) {
                        cmd.redirect_out = target_tok.slice;
                        cmd.redirect_append = true;
                    }
                } else if (t.kind == .RedirectIn) {
                    * = _.next();
                    const target_tok = _.next();
                    if (target_tok.kind == .Word) {
                        cmd.redirect_in = target_tok.slice;
                    }
                } else if (t.kind == .Pipe) {
                    * = _.next();
                    pipe.addCommand(cmd);
                    cmd = Command.init();
                } else {
                    break;
                }
            }

            if (cmd.args_len > 0) {
                pipe.addCommand(cmd);
            }

            let next_op: ControlOp = .None;
            const sep = _.peek();
            if (sep.kind == .And) {
                * = _.next();
                next_op = .And;
            } else if (sep.kind == .Or) {
                * = _.next();
                next_op = .Or;
            } else if (sep.kind == .Semicolon) {
                * = _.next();
                next_op = .Sequence;
            }

            if (pipe.commands_len > 0) {
                let stmt = Statement {
                    pipeline: pipe,
                    op: next_op,
                };
                script.addStatement(stmt);
            } else {
                break;
            }
        }

        return script;
    }
}
