// Source: /data/dev/bun/src/shell/
// ============================================================================
// Absorbed from Bun: /data/dev/bun/src/shell/
// Upstream Sync: Refer to /data/dev/bun/src/shell/ for future upstream updates.
// ============================================================================
// lib/shell/parser.cp
// AST nodes and parser for CP Shell (cap:shell)
// Supports assignments (FOO=bar cmd), pipelines (|), logical short-circuit (&& / ||),
// sequencing (; / \n), and stdout/stdin/stderr redirections (>, >>, <, 2>, 2>>, 2>&1, &>).

import { Lexer, Token, TokenKind, QuoteKind } from "./lexer.cp";
import { ShellEnv } from "./env.cp";

pub enum RedirectOp {
    Out,
    Append,
    In,
    Err,
    ErrAppend,
    ErrToOut,
    OutToErr,
    All,
    AllAppend,
}

pub struct Redirect {
    op: RedirectOp = .Out,
    target: []const u8 = "",
}

pub struct EnvAssign {
    pub name: []const u8 = "",
    pub raw_value: []const u8 = "",
}

pub const MAX_COMMAND_ARGS: usize = 32;
pub const MAX_COMMAND_ASSIGNS: usize = 16;
pub const MAX_PIPELINE_CMDS: usize = 16;
pub const MAX_STATEMENTS: usize = 32;

pub struct Command {
    args: [MAX_COMMAND_ARGS][]const u8 = undefined,
    args_len: usize = 0,
    raw_args: [MAX_COMMAND_ARGS][]const u8 = undefined,
    has_raw_args: [MAX_COMMAND_ARGS]bool = [false; MAX_COMMAND_ARGS],
    assigns: [MAX_COMMAND_ASSIGNS]EnvAssign = undefined,
    assigns_len: usize = 0,
    redirect_out: []const u8 = "",
    redirect_append: bool = false,
    redirect_in: []const u8 = "",
    redirect_err: []const u8 = "",
    redirect_err_append: bool = false,
    redirect_err_to_out: bool = false,
    redirect_out_to_err: bool = false,
    redirect_all: []const u8 = "",
    redirect_all_append: bool = false,

    pub static fn init(): Command {
        return Command {
            args: undefined,
            args_len: 0,
            raw_args: undefined,
            has_raw_args: [false; MAX_COMMAND_ARGS],
            assigns: undefined,
            assigns_len: 0,
            redirect_out: "",
            redirect_append: false,
            redirect_in: "",
            redirect_err: "",
            redirect_err_append: false,
            redirect_err_to_out: false,
            redirect_out_to_err: false,
            redirect_all: "",
            redirect_all_append: false,
        };
    }

    pub fn addArg(*_, arg: []const u8): bool {
        if (_.args_len >= MAX_COMMAND_ARGS) {
            return false;
        }
        _.args[_.args_len] = arg;
        _.raw_args[_.args_len] = arg;
        _.has_raw_args[_.args_len] = false;
        _.args_len += 1;
        return true;
    }

    pub fn addRawArg(*_, arg: []const u8, raw: []const u8): bool {
        if (_.args_len >= MAX_COMMAND_ARGS) {
            return false;
        }
        _.args[_.args_len] = arg;
        _.raw_args[_.args_len] = raw;
        _.has_raw_args[_.args_len] = true;
        _.args_len += 1;
        return true;
    }

    pub fn addAssign(*_, name: []const u8, raw_val: []const u8): bool {
        if (_.assigns_len >= MAX_COMMAND_ASSIGNS) {
            return false;
        }
        _.assigns[_.assigns_len] = EnvAssign {
            name: name,
            raw_value: raw_val,
        };
        _.assigns_len += 1;
        return true;
    }

    pub fn hasContent(): bool {
        return _.args_len > 0 || _.assigns_len > 0 ||
               _.redirect_out.len > 0 || _.redirect_err.len > 0 ||
               _.redirect_all.len > 0 || _.redirect_in.len > 0;
    }

    pub fn argv(*_): []const []const u8 {
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

    pub fn cmds(*_): []const Command {
        return _.commands[0.._.commands_len];
    }
}

pub enum ControlOp {
    None,
    And,      // &&
    Or,       // ||
    Sequence, // ; or \n
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

    pub fn stmts(*_): []const Statement {
        return _.statements[0.._.statements_len];
    }
}

fn tryParseAssign(tok: Token): EnvAssign? {
    if (tok.quote == .Single || tok.quote == .Double) {
        return null;
    }
    const raw = tok.raw_slice;
    let i: usize = 0;
    while (i < raw.len) {
        if (raw[i] == '=') {
            if (i == 0) {
                return null;
            }
            const name = raw[0..i];
            if (!ShellEnv.isValidIdent(name)) {
                return null;
            }
            return EnvAssign {
                name: name,
                raw_value: raw[i + 1 ..],
            };
        }
        i += 1;
    }
    return null;
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

    fn skipSeparators(*_) {
        while (true) {
            const tok = _.peek();
            if (tok.kind == .Newline || tok.kind == .Semicolon) {
                * = _.next();
            } else {
                break;
            }
        }
    }

    pub fn parseScript(*_): Script {
        let script = Script.init();

        while (true) {
            _.skipSeparators();
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
                    if (cmd.args_len == 0) {
                        if (assign := tryParseAssign(t)) {
                            * = cmd.addAssign(assign.name, assign.raw_value);
                            continue;
                        }
                    }
                    * = cmd.addRawArg(t.slice, t.raw_slice);
                } else if (t.kind == .RedirectOut) {
                    * = _.next();
                    const target_tok = _.next();
                    if (target_tok.kind == .Word) {
                        cmd.redirect_out = target_tok.raw_slice;
                        cmd.redirect_append = false;
                    }
                } else if (t.kind == .RedirectAppend) {
                    * = _.next();
                    const target_tok = _.next();
                    if (target_tok.kind == .Word) {
                        cmd.redirect_out = target_tok.raw_slice;
                        cmd.redirect_append = true;
                    }
                } else if (t.kind == .RedirectIn) {
                    * = _.next();
                    const target_tok = _.next();
                    if (target_tok.kind == .Word) {
                        cmd.redirect_in = target_tok.raw_slice;
                    }
                } else if (t.kind == .RedirectErr) {
                    * = _.next();
                    const target_tok = _.next();
                    if (target_tok.kind == .Word) {
                        cmd.redirect_err = target_tok.raw_slice;
                        cmd.redirect_err_append = false;
                    }
                } else if (t.kind == .RedirectErrAppend) {
                    * = _.next();
                    const target_tok = _.next();
                    if (target_tok.kind == .Word) {
                        cmd.redirect_err = target_tok.raw_slice;
                        cmd.redirect_err_append = true;
                    }
                } else if (t.kind == .RedirectErrToOut) {
                    * = _.next();
                    cmd.redirect_err_to_out = true;
                } else if (t.kind == .RedirectOutToErr) {
                    * = _.next();
                    cmd.redirect_out_to_err = true;
                } else if (t.kind == .RedirectAll) {
                    * = _.next();
                    const target_tok = _.next();
                    if (target_tok.kind == .Word) {
                        cmd.redirect_all = target_tok.raw_slice;
                        cmd.redirect_all_append = false;
                    }
                } else if (t.kind == .RedirectAllAppend) {
                    * = _.next();
                    const target_tok = _.next();
                    if (target_tok.kind == .Word) {
                        cmd.redirect_all = target_tok.raw_slice;
                        cmd.redirect_all_append = true;
                    }
                } else if (t.kind == .Pipe) {
                    * = _.next();
                    if (cmd.hasContent()) {
                        * = pipe.addCommand(cmd);
                    }
                    cmd = Command.init();
                    // Allow newline after pipe
                    while (_.peek().kind == .Newline) {
                        * = _.next();
                    }
                } else {
                    break;
                }
            }

            if (cmd.hasContent()) {
                * = pipe.addCommand(cmd);
            }

            let next_op: ControlOp = .None;
            const sep = _.peek();
            if (sep.kind == .And) {
                * = _.next();
                next_op = .And;
                while (_.peek().kind == .Newline) {
                    * = _.next();
                }
            } else if (sep.kind == .Or) {
                * = _.next();
                next_op = .Or;
                while (_.peek().kind == .Newline) {
                    * = _.next();
                }
            } else if (sep.kind == .Semicolon || sep.kind == .Newline) {
                * = _.next();
                next_op = .Sequence;
            }

            if (pipe.commands_len > 0) {
                const stmt = Statement {
                    pipeline: pipe,
                    op: next_op,
                };
                * = script.addStatement(stmt);
            } else {
                break;
            }
        }

        return script;
    }
}
