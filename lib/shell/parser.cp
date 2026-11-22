// Source: Absorbed from /data/dev/bun/src/shell/shell.cp | Synced: 2026-11-22T07:47:14Z
// ============================================================================
// Absorbed from Bun: /data/dev/bun/src/shell/
// Upstream Sync: Refer to /data/dev/bun/src/shell/ for future upstream updates.
// ============================================================================
// lib/shell/parser.cp
// POSIX AST nodes and recursive-descent parser for CP Shell (cap:shell)
// Supports assignments (FOO=bar cmd), pipelines (|), negation (!), background (&),
// logical short-circuit (&& / ||), sequencing (; / \n), subshells ( ... ),
// command groups { ...; }, control flow (if/elif/else/fi, for..in..do..done, while/until..do..done),
// and full redirections (>, >>, <, <<<, <<, 2>, 2>>, 2>&1, 1>&2, &>, &>>).

const std = @import("std");
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
    HereString,
    HereDoc,
}

pub struct Redirect {
    op: RedirectOp = .Out,
    target: []const u8 = "",
}

pub struct EnvAssign {
    pub name: []const u8 = "",
    pub raw_value: []const u8 = "",
}

pub enum CommandKind: u8 {
    Simple = 0,
    Subshell = 1,
    Group = 2,
    If = 3,
    For = 4,
    While = 5,
    Until = 6,
}

pub struct WordArg {
    pub slice: []const u8 = "",
    pub raw_slice: []const u8 = "",
    pub has_raw: bool = false,
}

pub const MAX_COMMAND_ARGS: usize = 32;
pub const MAX_COMMAND_ASSIGNS: usize = 8;
pub const MAX_COMMAND_REDIRECTS: usize = 4;
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
    redirect_herestring: []const u8 = "",
    redirects: [MAX_COMMAND_REDIRECTS]Redirect = undefined,
    redirects_len: usize = 0,
    kind: CommandKind = .Simple,
    compound_id: usize = 0,

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
            redirect_herestring: "",
            redirects: undefined,
            redirects_len: 0,
            kind: .Simple,
            compound_id: 0,
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

    pub fn addRedirect(*_, op: RedirectOp, target: []const u8) {
        if (_.redirects_len < MAX_COMMAND_REDIRECTS) {
            _.redirects[_.redirects_len] = Redirect { op: op, target: target };
            _.redirects_len += 1;
        }
    }

    pub fn hasContent(): bool {
        return _.kind != .Simple || _.args_len > 0 || _.assigns_len > 0 ||
               _.redirect_out.len > 0 || _.redirect_err.len > 0 ||
               _.redirect_all.len > 0 || _.redirect_in.len > 0 ||
               _.redirect_herestring.len > 0 || _.redirects_len > 0;
    }

    pub fn argv(*_): []const []const u8 {
        return _.args[0.._.args_len];
    }
}

pub struct Pipeline {
    commands: [MAX_PIPELINE_CMDS]Command = undefined,
    commands_len: usize = 0,
    negated: bool = false,
    background: bool = false,

    pub static fn init(): Pipeline {
        return Pipeline {
            commands: undefined,
            commands_len: 0,
            negated: false,
            background: false,
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

pub struct IfBranch {
    cond: usize = 0, // script_id in AstPool
    body: usize = 0, // script_id in AstPool
}

pub struct IfNode {
    branches: std.ArrayListUnmanaged(IfBranch) = .empty,
    else_body: usize? = null,
}

pub struct ForNode {
    var_name: []const u8 = "",
    has_in: bool = false,
    words: std.ArrayListUnmanaged(WordArg) = .empty,
    body: usize = 0, // script_id in AstPool
}

pub struct WhileNode {
    cond: usize = 0, // script_id in AstPool
    body: usize = 0, // script_id in AstPool
    is_until: bool = false,
}

pub struct SubshellNode {
    script_id: usize = 0,
    isolated: bool = true,
}

pub type ScriptPtr = *Script!;

pub struct AstPool {
    scripts: std.ArrayListUnmanaged(ScriptPtr) = .empty,
    if_nodes: std.ArrayListUnmanaged(IfNode) = .empty,
    for_nodes: std.ArrayListUnmanaged(ForNode) = .empty,
    while_nodes: std.ArrayListUnmanaged(WhileNode) = .empty,
    subshell_nodes: std.ArrayListUnmanaged(SubshellNode) = .empty,
    extra_args: std.ArrayListUnmanaged(WordArg) = .empty,
    ref_count: usize = 1,

    pub static fn create(): *AstPool! {
        const alloc = std.heap.page_allocator;
        let pool = alloc.create(AstPool) catch unreachable;
        pool.* = AstPool{
            scripts: .empty,
            if_nodes: .empty,
            for_nodes: .empty,
            while_nodes: .empty,
            subshell_nodes: .empty,
            extra_args: .empty,
            ref_count: 1,
        };
        return pool;
    }

    pub fn allocEmptyScript(*_): usize {
        const alloc = std.heap.page_allocator;
        let ptr = alloc.create(Script) catch unreachable;
        ptr.statements_len = 0;
        ptr.pool = null;
        const id = _.scripts.items.len;
        _.scripts.append(alloc, ptr) catch {};
        return id;
    }

    pub fn addScript(*_, s: Script): usize {
        const alloc = std.heap.page_allocator;
        let ptr = alloc.create(Script) catch unreachable;
        ptr.* = s;
        const id = _.scripts.items.len;
        _.scripts.append(alloc, ptr) catch {};
        return id;
    }

    pub fn getScript(id: usize): ScriptPtr? {
        if (id >= _.scripts.items.len) return null;
        return _.scripts.items[id];
    }

    pub fn addIf(*_, node: IfNode): usize {
        const alloc = std.heap.page_allocator;
        const id = _.if_nodes.items.len;
        _.if_nodes.append(alloc, node) catch {};
        return id;
    }

    pub fn addFor(*_, node: ForNode): usize {
        const alloc = std.heap.page_allocator;
        const id = _.for_nodes.items.len;
        _.for_nodes.append(alloc, node) catch {};
        return id;
    }

    pub fn addWhile(*_, node: WhileNode): usize {
        const alloc = std.heap.page_allocator;
        const id = _.while_nodes.items.len;
        _.while_nodes.append(alloc, node) catch {};
        return id;
    }

    pub fn addSubshell(*_, node: SubshellNode): usize {
        const alloc = std.heap.page_allocator;
        const id = _.subshell_nodes.items.len;
        _.subshell_nodes.append(alloc, node) catch {};
        return id;
    }

    pub fn destroy(*_) {
        const alloc = std.heap.page_allocator;
        for (sptr of _.scripts.items) {
            alloc.destroy(sptr);
        }
        _.scripts.deinit(alloc);
        for (*iff of _.if_nodes.items) {
            iff.branches.deinit(alloc);
        }
        _.if_nodes.deinit(alloc);
        for (*fn_node of _.for_nodes.items) {
            fn_node.words.deinit(alloc);
        }
        _.for_nodes.deinit(alloc);
        _.while_nodes.deinit(alloc);
        _.subshell_nodes.deinit(alloc);
        _.extra_args.deinit(alloc);
        alloc.destroy(_);
    }
}

pub struct Script {
    statements: [MAX_STATEMENTS]Statement = undefined,
    statements_len: usize = 0,
    pool: (*AstPool!)? = null,

    pub static fn init(): Script {
        return Script {
            statements: undefined,
            statements_len: 0,
            pool: null,
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

    pub fn destroy(*_) {
        if (p := _.pool) {
            p.destroy();
            _.pool = null;
        }
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

fn isUnquotedKeyword(tok: Token, kw: []const u8): bool {
    if (tok.kind != .Word || tok.quote != .None) return false;
    return std.mem.eql(u8, tok.raw_slice, kw);
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

    fn skipNewlines(*_) {
        while (_.peek().kind == .Newline) {
            * = _.next();
        }
    }

    fn ensurePool(pool_opt: *(*AstPool!)?): *AstPool! {
        if (p := pool_opt.*) {
            return p;
        }
        pool_opt.* = AstPool.create();
        return pool_opt.*.?;
    }

    fn isStopToken(tok: Token, stop_mode: u8): bool {
        // stop_mode:
        // 0 = top-level (EOF)
        // 1 = subshell ')'
        // 2 = group '}'
        // 3 = if-cond ('then')
        // 4 = if-body ('elif', 'else', 'fi')
        // 5 = loop-cond ('do')
        // 6 = loop-body ('done')
        if (tok.kind == .Eof) return true;
        if (stop_mode == 1 && tok.kind == .CloseParen) return true;
        if (stop_mode == 2 && isUnquotedKeyword(tok, "}")) return true;
        if (stop_mode == 3 && isUnquotedKeyword(tok, "then")) return true;
        if (stop_mode == 4 && (isUnquotedKeyword(tok, "elif") || isUnquotedKeyword(tok, "else") || isUnquotedKeyword(tok, "fi"))) return true;
        if (stop_mode == 5 && isUnquotedKeyword(tok, "do")) return true;
        if (stop_mode == 6 && isUnquotedKeyword(tok, "done")) return true;
        return false;
    }

    fn consumeRedirection(*_, cmd: *Command): bool {
        const t = _.peek();
        if (t.kind == .RedirectOut) {
            * = _.next();
            const target_tok = _.next();
            if (target_tok.kind == .Word) {
                cmd.redirect_out = target_tok.raw_slice;
                cmd.redirect_append = false;
                cmd.addRedirect(.Out, target_tok.raw_slice);
            }
            return true;
        } else if (t.kind == .RedirectAppend) {
            * = _.next();
            const target_tok = _.next();
            if (target_tok.kind == .Word) {
                cmd.redirect_out = target_tok.raw_slice;
                cmd.redirect_append = true;
                cmd.addRedirect(.Append, target_tok.raw_slice);
            }
            return true;
        } else if (t.kind == .RedirectIn) {
            * = _.next();
            const target_tok = _.next();
            if (target_tok.kind == .Word) {
                cmd.redirect_in = target_tok.raw_slice;
                cmd.addRedirect(.In, target_tok.raw_slice);
            }
            return true;
        } else if (t.kind == .HereString) {
            * = _.next();
            const target_tok = _.next();
            if (target_tok.kind == .Word) {
                cmd.redirect_herestring = target_tok.raw_slice;
                cmd.addRedirect(.HereString, target_tok.raw_slice);
            }
            return true;
        } else if (t.kind == .RedirectErr) {
            * = _.next();
            const target_tok = _.next();
            if (target_tok.kind == .Word) {
                cmd.redirect_err = target_tok.raw_slice;
                cmd.redirect_err_append = false;
                cmd.addRedirect(.Err, target_tok.raw_slice);
            }
            return true;
        } else if (t.kind == .RedirectErrAppend) {
            * = _.next();
            const target_tok = _.next();
            if (target_tok.kind == .Word) {
                cmd.redirect_err = target_tok.raw_slice;
                cmd.redirect_err_append = true;
                cmd.addRedirect(.ErrAppend, target_tok.raw_slice);
            }
            return true;
        } else if (t.kind == .RedirectErrToOut) {
            * = _.next();
            cmd.redirect_err_to_out = true;
            cmd.addRedirect(.ErrToOut, "");
            return true;
        } else if (t.kind == .RedirectOutToErr) {
            * = _.next();
            cmd.redirect_out_to_err = true;
            cmd.addRedirect(.OutToErr, "");
            return true;
        } else if (t.kind == .RedirectAll) {
            * = _.next();
            const target_tok = _.next();
            if (target_tok.kind == .Word) {
                cmd.redirect_all = target_tok.raw_slice;
                cmd.redirect_all_append = false;
                cmd.addRedirect(.All, target_tok.raw_slice);
            }
            return true;
        } else if (t.kind == .RedirectAllAppend) {
            * = _.next();
            const target_tok = _.next();
            if (target_tok.kind == .Word) {
                cmd.redirect_all = target_tok.raw_slice;
                cmd.redirect_all_append = true;
                cmd.addRedirect(.AllAppend, target_tok.raw_slice);
            }
            return true;
        }
        return false;
    }

    fn parseSubshellCmd(*_, pool_opt: *(*AstPool!)?): Command {
        * = _.next();
        let pool = Parser.ensurePool(pool_opt);
        const sub_id = pool.allocEmptyScript();
        if (sptr := pool.getScript(sub_id)) {
            _.parseBlockInto(1, pool, sptr);
        }
        if (_.peek().kind == .CloseParen) {
            * = _.next();
        }
        const node_id = pool.addSubshell(SubshellNode{ script_id: sub_id, isolated: true });
        let cmd = Command.init();
        cmd.kind = .Subshell;
        cmd.compound_id = node_id;
        while (_.consumeRedirection(&cmd)) {}
        return cmd;
    }

    fn parseGroupCmd(*_, pool_opt: *(*AstPool!)?): Command {
        * = _.next();
        let pool = Parser.ensurePool(pool_opt);
        const grp_id = pool.allocEmptyScript();
        if (sptr := pool.getScript(grp_id)) {
            _.parseBlockInto(2, pool, sptr);
        }
        if (isUnquotedKeyword(_.peek(), "}")) {
            * = _.next();
        }
        const node_id = pool.addSubshell(SubshellNode{ script_id: grp_id, isolated: false });
        let cmd = Command.init();
        cmd.kind = .Group;
        cmd.compound_id = node_id;
        while (_.consumeRedirection(&cmd)) {}
        return cmd;
    }

    fn parseIfCmd(*_, pool_opt: *(*AstPool!)?): Command {
        * = _.next();
        let pool = Parser.ensurePool(pool_opt);
        const alloc = std.heap.page_allocator;
        let if_node = IfNode{};

        while (true) {
            const c_id = pool.allocEmptyScript();
            if (cptr := pool.getScript(c_id)) {
                _.parseBlockInto(3, pool, cptr);
            }
            if (isUnquotedKeyword(_.peek(), "then")) {
                * = _.next();
            }
            const b_id = pool.allocEmptyScript();
            if (bptr := pool.getScript(b_id)) {
                _.parseBlockInto(4, pool, bptr);
            }
            if_node.branches.append(alloc, IfBranch{ cond: c_id, body: b_id }) catch {};

            if (isUnquotedKeyword(_.peek(), "elif")) {
                * = _.next();
                continue;
            }
            break;
        }

        if (isUnquotedKeyword(_.peek(), "else")) {
            * = _.next();
            const e_id = pool.allocEmptyScript();
            if (eptr := pool.getScript(e_id)) {
                _.parseBlockInto(4, pool, eptr);
            }
            if_node.else_body = e_id;
        }
        if (isUnquotedKeyword(_.peek(), "fi")) {
            * = _.next();
        }

        const node_id = pool.addIf(if_node);
        let cmd = Command.init();
        cmd.kind = .If;
        cmd.compound_id = node_id;
        while (_.consumeRedirection(&cmd)) {}
        return cmd;
    }

    fn parseForCmd(*_, pool_opt: *(*AstPool!)?): Command {
        * = _.next();
        let pool = Parser.ensurePool(pool_opt);
        const alloc = std.heap.page_allocator;
        let for_node = ForNode{};

        const var_tok = _.peek();
        if (var_tok.kind == .Word) {
            * = _.next();
            for_node.var_name = var_tok.slice;
        }

        _.skipNewlines();
        if (isUnquotedKeyword(_.peek(), "in")) {
            * = _.next();
            for_node.has_in = true;
            while (true) {
                const wt = _.peek();
                if (wt.kind == .Semicolon || wt.kind == .Newline || wt.kind == .Eof || isUnquotedKeyword(wt, "do")) {
                    break;
                }
                if (wt.kind == .Word) {
                    * = _.next();
                    for_node.words.append(alloc, WordArg{
                        slice: wt.slice,
                        raw_slice: wt.raw_slice,
                        has_raw: true,
                    }) catch {};
                } else {
                    break;
                }
            }
        }

        _.skipSeparators();
        if (isUnquotedKeyword(_.peek(), "do")) {
            * = _.next();
        }
        const b_id = pool.allocEmptyScript();
        if (bptr := pool.getScript(b_id)) {
            _.parseBlockInto(6, pool, bptr);
        }
        if (isUnquotedKeyword(_.peek(), "done")) {
            * = _.next();
        }
        for_node.body = b_id;

        const node_id = pool.addFor(for_node);
        let cmd = Command.init();
        cmd.kind = .For;
        cmd.compound_id = node_id;
        while (_.consumeRedirection(&cmd)) {}
        return cmd;
    }

    fn parseWhileCmd(*_, pool_opt: *(*AstPool!)?, is_until: bool): Command {
        * = _.next();
        let pool = Parser.ensurePool(pool_opt);
        const c_id = pool.allocEmptyScript();
        if (cptr := pool.getScript(c_id)) {
            _.parseBlockInto(5, pool, cptr);
        }
        if (isUnquotedKeyword(_.peek(), "do")) {
            * = _.next();
        }
        const b_id = pool.allocEmptyScript();
        if (bptr := pool.getScript(b_id)) {
            _.parseBlockInto(6, pool, bptr);
        }
        if (isUnquotedKeyword(_.peek(), "done")) {
            * = _.next();
        }
        const node_id = pool.addWhile(WhileNode{ cond: c_id, body: b_id, is_until: is_until });
        let cmd = Command.init();
        cmd.kind = .While;
        cmd.compound_id = node_id;
        while (_.consumeRedirection(&cmd)) {}
        return cmd;
    }

    fn parseCompoundCommand(*_, pool_opt: *(*AstPool!)?): Command? {
        const t = _.peek();
        if (t.kind == .OpenParen) return _.parseSubshellCmd(pool_opt);
        if (isUnquotedKeyword(t, "{")) return _.parseGroupCmd(pool_opt);
        if (isUnquotedKeyword(t, "if")) return _.parseIfCmd(pool_opt);
        if (isUnquotedKeyword(t, "for")) return _.parseForCmd(pool_opt);
        if (isUnquotedKeyword(t, "while")) return _.parseWhileCmd(pool_opt, false);
        if (isUnquotedKeyword(t, "until")) return _.parseWhileCmd(pool_opt, true);
        return null;
    }

    fn parseBlockInto(*_, stop_mode: u8, shared_pool: *AstPool!, out_script: ScriptPtr) {
        let pool_holder: (*AstPool!)? = shared_pool;

        while (true) {
            _.skipSeparators();
            const tok = _.peek();
            if (Parser.isStopToken(tok, stop_mode)) {
                break;
            }

            let pipe = Pipeline.init();
            if (_.peek().kind == .Bang) {
                * = _.next();
                pipe.negated = true;
            }

            while (true) {
                if (compound := _.parseCompoundCommand(&pool_holder)) {
                    * = pipe.addCommand(compound);
                    if (_.peek().kind == .Pipe) {
                        * = _.next();
                        _.skipNewlines();
                        continue;
                    }
                    break;
                }

                let cmd = Command.init();
                while (true) {
                    const t = _.peek();
                    if (Parser.isStopToken(t, stop_mode)) {
                        break;
                    }
                    if (t.kind == .Word) {
                        * = _.next();
                        if (cmd.args_len == 0) {
                            if (assign := tryParseAssign(t)) {
                                * = cmd.addAssign(assign.name, assign.raw_value);
                                continue;
                            }
                        }
                        * = cmd.addRawArg(t.slice, t.raw_slice);
                    } else if (_.consumeRedirection(&cmd)) {
                        continue;
                    } else if (t.kind == .Pipe) {
                        * = _.next();
                        if (cmd.hasContent()) {
                            * = pipe.addCommand(cmd);
                        }
                        cmd = Command.init();
                        _.skipNewlines();
                        if (compound2 := _.parseCompoundCommand(&pool_holder)) {
                            * = pipe.addCommand(compound2);
                            if (_.peek().kind == .Pipe) {
                                * = _.next();
                                _.skipNewlines();
                                continue;
                            }
                            break;
                        }
                    } else {
                        break;
                    }
                }
                if (cmd.hasContent()) {
                    * = pipe.addCommand(cmd);
                }
                break;
            }

            if (_.peek().kind == .Ampersand) {
                * = _.next();
                pipe.background = true;
            }

            let next_op: ControlOp = .None;
            const sep = _.peek();
            if (sep.kind == .And) {
                * = _.next();
                next_op = .And;
                _.skipNewlines();
            } else if (sep.kind == .Or) {
                * = _.next();
                next_op = .Or;
                _.skipNewlines();
            } else if (sep.kind == .Semicolon || sep.kind == .Newline) {
                * = _.next();
                next_op = .Sequence;
            }

            if (pipe.commands_len > 0) {
                const stmt = Statement {
                    pipeline: pipe,
                    op: next_op,
                };
                * = out_script.addStatement(stmt);
            } else {
                break;
            }
        }
    }

    pub fn parseScriptHeap(*_): ScriptPtr {
        const alloc = std.heap.page_allocator;
        let out_script = alloc.create(Script) catch unreachable;
        out_script.statements_len = 0;
        out_script.pool = null;
        let pool_holder: (*AstPool!)? = null;

        while (true) {
            _.skipSeparators();
            const tok = _.peek();
            if (tok.kind == .Eof) {
                break;
            }

            let pipe = Pipeline.init();
            if (_.peek().kind == .Bang) {
                * = _.next();
                pipe.negated = true;
            }

            while (true) {
                if (compound := _.parseCompoundCommand(&pool_holder)) {
                    * = pipe.addCommand(compound);
                    if (_.peek().kind == .Pipe) {
                        * = _.next();
                        _.skipNewlines();
                        continue;
                    }
                    break;
                }

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
                    } else if (_.consumeRedirection(&cmd)) {
                        continue;
                    } else if (t.kind == .Pipe) {
                        * = _.next();
                        if (cmd.hasContent()) {
                            * = pipe.addCommand(cmd);
                        }
                        cmd = Command.init();
                        _.skipNewlines();
                        if (compound2 := _.parseCompoundCommand(&pool_holder)) {
                            * = pipe.addCommand(compound2);
                            if (_.peek().kind == .Pipe) {
                                * = _.next();
                                _.skipNewlines();
                                continue;
                            }
                            break;
                        }
                    } else {
                        break;
                    }
                }
                if (cmd.hasContent()) {
                    * = pipe.addCommand(cmd);
                }
                break;
            }

            if (_.peek().kind == .Ampersand) {
                * = _.next();
                pipe.background = true;
            }

            let next_op: ControlOp = .None;
            const sep = _.peek();
            if (sep.kind == .And) {
                * = _.next();
                next_op = .And;
                _.skipNewlines();
            } else if (sep.kind == .Or) {
                * = _.next();
                next_op = .Or;
                _.skipNewlines();
            } else if (sep.kind == .Semicolon || sep.kind == .Newline) {
                * = _.next();
                next_op = .Sequence;
            }

            if (pipe.commands_len > 0) {
                const stmt = Statement {
                    pipeline: pipe,
                    op: next_op,
                };
                * = out_script.addStatement(stmt);
            } else {
                break;
            }
        }

        out_script.pool = pool_holder;
        return out_script;
    }

    pub fn parseScript(*_): Script {
        let sptr = _.parseScriptHeap();
        const res = sptr.*;
        std.heap.page_allocator.destroy(sptr);
        return res;
    }
}
