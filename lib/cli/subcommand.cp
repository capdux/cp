// Source: Absorbed from /data/dev/bun/src/cli/cli.cp | Synced: 2026-11-20T22:27:01Z
// lib/cli/subcommand.cp
// Multi-level SubcommandRouter (auto-aligned colored --help & Levenshtein didYouMean) and MultiProgress task tree

const std = @import("std");
import { AnsiColor, Spinner, SpinnerStyle, SpinnerState } from "./tty.cp";

pub struct SubcommandFlag {
    pub long_name: []const u8 = "";
    pub short_name: []const u8 = "";
    pub takes_value: bool = false;
    pub value_hint: []const u8 = "";
    pub description: []const u8 = "";
}

pub struct SubcommandSpec {
    pub name: []const u8 = "";
    pub alias: []const u8 = "";
    pub description: []const u8 = "";
    pub usage_args: []const u8 = "";
    pub parent: []const u8 = "";
    pub hidden: bool = false;
}

pub enum RouteStatus: u8 {
    Matched = 0,
    HelpRequested = 1,
    NoCommand = 2,
    UnknownCommand = 3,
}

pub struct SubcommandMatch {
    pub status: RouteStatus = .NoCommand,
    pub command_name: []const u8 = "",
    pub parent_name: []const u8 = "",
    pub spec_index: usize = 0,
    pub suggestion: ([]const u8)? = null,
    pub unknown_token: []const u8 = "",
    pub consumed_args: usize = 0,
    pub remaining_start: usize = 0,
    pub help_requested: bool = false,
}

pub struct SubcommandRouter {
    pub program_name: []const u8 = "cap",
    pub summary: []const u8 = "",
    pub footer: []const u8 = "",
    specs: [64]SubcommandSpec = undefined,
    spec_count: usize = 0,
    flags: [32]SubcommandFlag = undefined,
    flag_count: usize = 0,

    pub static fn init(program_name: []const u8, summary: []const u8 = "", footer: []const u8 = ""): SubcommandRouter {
        return SubcommandRouter{
            program_name: program_name,
            summary: summary,
            footer: footer,
            specs: undefined,
            spec_count: 0,
            flags: undefined,
            flag_count: 0,
        };
    }

    pub fn addCommand(*_, spec: SubcommandSpec): bool {
        if (_.spec_count >= 64) {
            return false;
        }
        _.specs[_.spec_count] = spec;
        _.spec_count += 1;
        return true;
    }

    pub fn addFlag(*_, flag: SubcommandFlag): bool {
        if (_.flag_count >= 32) {
            return false;
        }
        _.flags[_.flag_count] = flag;
        _.flag_count += 1;
        return true;
    }

    /// Computes Levenshtein edit distance between two ASCII/UTF-8 byte slices (stack-only DP).
    pub static fn levenshteinDistance(a: []const u8, b: []const u8): usize {
        if (a.len == 0) return b.len;
        if (b.len == 0) return a.len;
        const max_len: usize = 64;
        const n = if (a.len > max_len) max_len else a.len;
        const m = if (b.len > max_len) max_len else b.len;

        let prev_row: [65]usize = [0; 65];
        let curr_row: [65]usize = [0; 65];
        let j: usize = 0;
        while (j <= m) : (j += 1) {
            prev_row[j] = j;
        }

        let i: usize = 0;
        while (i < n) : (i += 1) {
            curr_row[0] = i + 1;
            j = 0;
            while (j < m) : (j += 1) {
                const cost: usize = if (a[i] == b[j]) 0 else 1;
                const del_cost = prev_row[j + 1] + 1;
                const ins_cost = curr_row[j] + 1;
                const sub_cost = prev_row[j] + cost;
                let best = if (del_cost < ins_cost) del_cost else ins_cost;
                if (sub_cost < best) {
                    best = sub_cost;
                }
                curr_row[j + 1] = best;
            }
            let k: usize = 0;
            while (k <= m) : (k += 1) {
                prev_row[k] = curr_row[k];
            }
        }
        return prev_row[m] + (a.len - n) + (b.len - m);
    }

    /// Fuzzy-matches an unknown subcommand token against registered commands under `parent`.
    pub fn didYouMean(input: []const u8, parent: []const u8 = ""): ([]const u8)? {
        if (input.len == 0) {
            return null;
        }
        let best_name: ([]const u8)? = null;
        let best_dist: usize = std.math.maxInt(usize);
        const max_allowed: usize = if (input.len <= 3) 1 else if (input.len <= 6) 2 else 3;

        let i: usize = 0;
        while (i < _.spec_count) : (i += 1) {
            const sp = _.specs[i];
            if (sp.hidden || !std.mem.eql(u8, sp.parent, parent)) {
                continue;
            }
            const d_name = SubcommandRouter.levenshteinDistance(input, sp.name);
            if (d_name <= max_allowed && d_name < best_dist) {
                best_dist = d_name;
                best_name = sp.name;
            }
            if (sp.alias.len > 0) {
                const d_alias = SubcommandRouter.levenshteinDistance(input, sp.alias);
                if (d_alias <= max_allowed && d_alias < best_dist) {
                    best_dist = d_alias;
                    best_name = sp.name;
                }
            }
        }
        return best_name;
    }

    fn findCommand(token: []const u8, parent: []const u8): usize? {
        let i: usize = 0;
        while (i < _.spec_count) : (i += 1) {
            const sp = _.specs[i];
            if (!std.mem.eql(u8, sp.parent, parent)) {
                continue;
            }
            if (std.mem.eql(u8, sp.name, token)) {
                return i;
            }
            if (sp.alias.len > 0 && std.mem.eql(u8, sp.alias, token)) {
                return i;
            }
        }
        return null;
    }

    fn hasChildren(cmd_name: []const u8): bool {
        let i: usize = 0;
        while (i < _.spec_count) : (i += 1) {
            if (std.mem.eql(u8, _.specs[i].parent, cmd_name)) {
                return true;
            }
        }
        return false;
    }

    static fn isHelpFlag(tok: []const u8): bool {
        return std.mem.eql(u8, tok, "--help") || std.mem.eql(u8, tok, "-h");
    }

    /// Resolves multi-level subcommands (e.g. `pm cache rm --force`) and reports help or fuzzy suggestions.
    pub fn resolve(argv: []const []const u8): SubcommandMatch {
        let pos: usize = 0;
        let current_parent: []const u8 = "";
        let last_match_idx: usize? = null;
        let help_seen = false;

        while (pos < argv.len) {
            const tok = argv[pos];
            if (SubcommandRouter.isHelpFlag(tok)) {
                help_seen = true;
                pos += 1;
                continue;
            }
            if (tok.len > 0 && tok[0] == '-') {
                break;
            }
            if (idx := _.findCommand(tok, current_parent)) {
                last_match_idx = idx;
                current_parent = _.specs[idx].name;
                pos += 1;
                if (!_.hasChildren(current_parent)) {
                    break;
                }
                continue;
            }
            if (last_match_idx == null) {
                return SubcommandMatch{
                    status: .UnknownCommand,
                    unknown_token: tok,
                    suggestion: _.didYouMean(tok, current_parent),
                    remaining_start: pos,
                };
            }
            // Subcommand matched parent, and `tok` is either unknown subcommand or positional arg
            if (_.hasChildren(current_parent)) {
                if (sug := _.didYouMean(tok, current_parent)) {
                    return SubcommandMatch{
                        status: .UnknownCommand,
                        parent_name: current_parent,
                        unknown_token: tok,
                        suggestion: sug,
                        remaining_start: pos,
                    };
                }
            }
            break;
        }

        // Check trailing --help/-h anywhere in remaining args
        let scan = pos;
        while (scan < argv.len) : (scan += 1) {
            if (std.mem.eql(u8, argv[scan], "--")) break;
            if (SubcommandRouter.isHelpFlag(argv[scan])) {
                help_seen = true;
                break;
            }
        }

        if (m_idx := last_match_idx) {
            const sp = _.specs[m_idx];
            return SubcommandMatch{
                status: if (help_seen) .HelpRequested else .Matched,
                command_name: sp.name,
                parent_name: sp.parent,
                spec_index: m_idx,
                consumed_args: pos,
                remaining_start: pos,
                help_requested: help_seen,
            };
        }

        return SubcommandMatch{
            status: if (help_seen) .HelpRequested else .NoCommand,
            consumed_args: pos,
            remaining_start: pos,
            help_requested: help_seen,
        };
    }

    static fn bufAppend(out: []u8, pos: *usize, s: []const u8) {
        if (pos.* >= out.len || s.len == 0) return;
        const avail = out.len - pos.*;
        const n = if (s.len <= avail) s.len else avail;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            out[pos.* + i] = s[i];
        }
        pos.* += n;
    }

    static fn bufSpaces(out: []u8, pos: *usize, count: usize) {
        let i: usize = 0;
        while (i < count && pos.* < out.len) : (i += 1) {
            out[pos.*] = ' ';
            pos.* += 1;
        }
    }

    /// Generates auto-aligned `--help` text (with optional ANSI color) for root or a subcommand namespace.
    pub fn formatHelp(out_buf: []u8, use_color: bool = true, parent: []const u8 = ""): []const u8 {
        let pos: usize = 0;

        if (_.summary.len > 0 && parent.len == 0) {
            if (use_color) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.BOLD);
            SubcommandRouter.bufAppend(out_buf, *pos, _.program_name);
            if (use_color) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.RESET);
            SubcommandRouter.bufAppend(out_buf, *pos, " - ");
            SubcommandRouter.bufAppend(out_buf, *pos, _.summary);
            SubcommandRouter.bufAppend(out_buf, *pos, "\n\n");
        }

        if (use_color) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.BOLD);
        SubcommandRouter.bufAppend(out_buf, *pos, "Usage: ");
        if (use_color) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.RESET);
        SubcommandRouter.bufAppend(out_buf, *pos, _.program_name);
        if (parent.len > 0) {
            SubcommandRouter.bufAppend(out_buf, *pos, " ");
            SubcommandRouter.bufAppend(out_buf, *pos, parent);
        }
        SubcommandRouter.bufAppend(out_buf, *pos, " ");
        if (use_color) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.CYAN);
        SubcommandRouter.bufAppend(out_buf, *pos, "<command>");
        if (use_color) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.RESET);
        SubcommandRouter.bufAppend(out_buf, *pos, " [flags] [args]\n\n");

        // Compute alignment width across commands & usage_args
        let max_left: usize = 12;
        let i: usize = 0;
        while (i < _.spec_count) : (i += 1) {
            const sp = _.specs[i];
            if (sp.hidden || !std.mem.eql(u8, sp.parent, parent)) continue;
            let w = sp.name.len;
            if (sp.usage_args.len > 0) {
                w += 1 + sp.usage_args.len;
            }
            if (w > max_left) max_left = w;
        }

        if (use_color) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.BOLD);
        SubcommandRouter.bufAppend(out_buf, *pos, "Commands:\n");
        if (use_color) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.RESET);

        i = 0;
        while (i < _.spec_count) : (i += 1) {
            const sp = _.specs[i];
            if (sp.hidden || !std.mem.eql(u8, sp.parent, parent)) continue;
            SubcommandRouter.bufAppend(out_buf, *pos, "  ");
            if (use_color) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.CYAN);
            SubcommandRouter.bufAppend(out_buf, *pos, sp.name);
            if (use_color) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.RESET);

            let used = sp.name.len;
            if (sp.usage_args.len > 0) {
                SubcommandRouter.bufAppend(out_buf, *pos, " ");
                if (use_color) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.DIM);
                SubcommandRouter.bufAppend(out_buf, *pos, sp.usage_args);
                if (use_color) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.RESET);
                used += 1 + sp.usage_args.len;
            }

            const pad = if (max_left >= used) (max_left - used + 3) else 3;
            SubcommandRouter.bufSpaces(out_buf, *pos, pad);
            SubcommandRouter.bufAppend(out_buf, *pos, sp.description);
            if (sp.alias.len > 0) {
                if (use_color) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.DIM);
                SubcommandRouter.bufAppend(out_buf, *pos, " (alias: ");
                SubcommandRouter.bufAppend(out_buf, *pos, sp.alias);
                SubcommandRouter.bufAppend(out_buf, *pos, ")");
                if (use_color) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.RESET);
            }
            SubcommandRouter.bufAppend(out_buf, *pos, "\n");
        }

        if (_.flag_count > 0) {
            SubcommandRouter.bufAppend(out_buf, *pos, "\n");
            if (use_color) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.BOLD);
            SubcommandRouter.bufAppend(out_buf, *pos, "Flags:\n");
            if (use_color) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.RESET);

            let max_flag_w: usize = 12;
            let fi: usize = 0;
            while (fi < _.flag_count) : (fi += 1) {
                const fl = _.flags[fi];
                let fw = fl.long_name.len + 2;
                if (fl.short_name.len > 0) fw += fl.short_name.len + 3;
                if (fl.value_hint.len > 0) fw += fl.value_hint.len + 1;
                if (fw > max_flag_w) max_flag_w = fw;
            }

            fi = 0;
            while (fi < _.flag_count) : (fi += 1) {
                const fl = _.flags[fi];
                SubcommandRouter.bufAppend(out_buf, *pos, "  ");
                let fw: usize = 0;
                if (fl.short_name.len > 0) {
                    if (use_color) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.GREEN);
                    SubcommandRouter.bufAppend(out_buf, *pos, "-");
                    SubcommandRouter.bufAppend(out_buf, *pos, fl.short_name);
                    if (use_color) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.RESET);
                    SubcommandRouter.bufAppend(out_buf, *pos, ", ");
                    fw += fl.short_name.len + 3;
                }
                if (use_color) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.GREEN);
                SubcommandRouter.bufAppend(out_buf, *pos, "--");
                SubcommandRouter.bufAppend(out_buf, *pos, fl.long_name);
                if (use_color) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.RESET);
                fw += fl.long_name.len + 2;

                if (fl.value_hint.len > 0) {
                    SubcommandRouter.bufAppend(out_buf, *pos, " ");
                    SubcommandRouter.bufAppend(out_buf, *pos, fl.value_hint);
                    fw += 1 + fl.value_hint.len;
                }
                const fpad = if (max_flag_w >= fw) (max_flag_w - fw + 3) else 3;
                SubcommandRouter.bufSpaces(out_buf, *pos, fpad);
                SubcommandRouter.bufAppend(out_buf, *pos, fl.description);
                SubcommandRouter.bufAppend(out_buf, *pos, "\n");
            }
        }

        if (_.footer.len > 0) {
            SubcommandRouter.bufAppend(out_buf, *pos, "\n");
            SubcommandRouter.bufAppend(out_buf, *pos, _.footer);
            SubcommandRouter.bufAppend(out_buf, *pos, "\n");
        }
        return out_buf[0..pos];
    }
}

pub enum ProgressUnit: u8 {
    None = 0,
    Files = 1,
    Bytes = 2,
}

pub enum ProgressTaskState: u8 {
    Running = 0,
    Succeeded = 1,
    Failed = 2,
}

pub const NO_PARENT_TASK: usize = std.math.maxInt(usize);

pub struct ProgressTask {
    pub id: usize = 0,
    pub parent_id: usize = NO_PARENT_TASK,
    pub name: []const u8 = "",
    pub total: u64 = 0,
    pub completed: u64 = 0,
    pub unit: ProgressUnit = .None,
    pub state: ProgressTaskState = .Running,
    pub spinner: Spinner = Spinner.init("", SpinnerStyle.Dots),
    pub start_ns: u64 = 0,
    pub last_ns: u64 = 0,
}

fn mpMonotonicNowNs(): u64 {
    let ts: std.os.linux.timespec = .{ .sec = 0, .nsec = 0 };
    zig {
        _ = std.os.linux.clock_gettime(.MONOTONIC, &ts);
    }
    return ((ts.sec as u64) * 1000000000) + (ts.nsec as u64);
}

/// Concurrent multi-task hierarchical progress tree with frame-animated Spinner, throughput & ETA formatting.
pub struct MultiProgress {
    pub title: []const u8 = "",
    tasks: [32]ProgressTask = undefined,
    task_count: usize = 0,
    bar_width: usize = 16,
    lock_word: std.atomic.Value(u32) = std.atomic.Value(u32).init(0),

    pub static fn init(title: []const u8 = "", bar_width: usize = 16): MultiProgress {
        return MultiProgress{
            title: title,
            tasks: undefined,
            task_count: 0,
            bar_width: if (bar_width == 0) 16 else bar_width,
            lock_word: std.atomic.Value(u32).init(0),
        };
    }

    fn lock(*_) {
        while (_.lock_word.cmpxchgWeak(0, 1, .acquire, .monotonic) != null) {}
    }

    fn unlock(*_) {
        _.lock_word.store(0, .release);
    }

    pub fn addTask(*_, name: []const u8, total: u64, unit: ProgressUnit = .None, style: SpinnerStyle = .Dots): usize {
        return _.addChildTaskAt(NO_PARENT_TASK, name, total, unit, style, mpMonotonicNowNs());
    }

    pub fn addChildTask(*_, parent_id: usize, name: []const u8, total: u64, unit: ProgressUnit = .None, style: SpinnerStyle = .Dots): usize {
        return _.addChildTaskAt(parent_id, name, total, unit, style, mpMonotonicNowNs());
    }

    pub fn addChildTaskAt(
        *_,
        parent_id: usize,
        name: []const u8,
        total: u64,
        unit: ProgressUnit,
        style: SpinnerStyle,
        now_ns: u64,
    ): usize {
        _.lock();
        defer _.unlock();
        if (_.task_count >= 32) {
            return 31;
        }
        const id = _.task_count;
        _.tasks[id] = ProgressTask{
            id: id,
            parent_id: parent_id,
            name: name,
            total: total,
            completed: 0,
            unit: unit,
            state: .Running,
            spinner: Spinner.init(name, style),
            start_ns: now_ns,
            last_ns: now_ns,
        };
        _.task_count += 1;
        return id;
    }

    pub fn updateTask(*_, id: usize, completed: u64) {
        _.updateTaskAt(id, completed, mpMonotonicNowNs());
    }

    pub fn updateTaskAt(*_, id: usize, completed: u64, now_ns: u64) {
        _.lock();
        defer _.unlock();
        if (id >= _.task_count) return;
        let t = &_.tasks[id];
        t.completed = if (t.total > 0 && completed > t.total) t.total else completed;
        if (now_ns >= t.last_ns) t.last_ns = now_ns;
        * = t.spinner.tick();
    }

    pub fn advanceTask(*_, id: usize, delta: u64) {
        _.advanceTaskAt(id, delta, mpMonotonicNowNs());
    }

    pub fn advanceTaskAt(*_, id: usize, delta: u64, now_ns: u64) {
        _.lock();
        defer _.unlock();
        if (id >= _.task_count) return;
        let t = &_.tasks[id];
        const next_val = t.completed + delta;
        t.completed = if (t.total > 0 && next_val > t.total) t.total else next_val;
        if (now_ns >= t.last_ns) t.last_ns = now_ns;
        * = t.spinner.tick();
    }

    pub fn completeTask(*_, id: usize, msg: []const u8 = "") {
        _.lock();
        defer _.unlock();
        if (id >= _.task_count) return;
        let t = &_.tasks[id];
        t.state = .Succeeded;
        if (t.total > 0) t.completed = t.total;
        t.spinner.succeed(msg);
    }

    pub fn failTask(*_, id: usize, msg: []const u8 = "") {
        _.lock();
        defer _.unlock();
        if (id >= _.task_count) return;
        let t = &_.tasks[id];
        t.state = .Failed;
        t.spinner.fail(msg);
    }

    pub fn tickAll(*_) {
        _.lock();
        defer _.unlock();
        let i: usize = 0;
        while (i < _.task_count) : (i += 1) {
            if (_.tasks[i].state == .Running) {
                * = _.tasks[i].spinner.tick();
            }
        }
    }

    pub fn taskCount(): usize {
        return _.task_count;
    }

    pub fn getTask(id: usize): ProgressTask? {
        if (id >= _.task_count) return null;
        return _.tasks[id];
    }

    pub static fn formatThroughput(rate: u64, unit: ProgressUnit, out_buf: []u8): []const u8 {
        return switch (unit) {
            .None => std.fmt.bufPrint(out_buf, "{d}/s", .{rate}) catch "",
            .Files => std.fmt.bufPrint(out_buf, "{d} files/s", .{rate}) catch "",
            .Bytes => {
                if (rate >= 1024 * 1024) {
                    const whole = rate / (1024 * 1024);
                    const tenths = ((rate % (1024 * 1024)) * 10) / (1024 * 1024);
                    return std.fmt.bufPrint(out_buf, "{d}.{d} MB/s", .{ whole, tenths }) catch "";
                } else if (rate >= 1024) {
                    const whole = rate / 1024;
                    const tenths = ((rate % 1024) * 10) / 1024;
                    return std.fmt.bufPrint(out_buf, "{d}.{d} KB/s", .{ whole, tenths }) catch "";
                }
                return std.fmt.bufPrint(out_buf, "{d} B/s", .{rate}) catch "";
            },
        };
    }

    pub static fn formatEta(eta_sec: u64, out_buf: []u8): []const u8 {
        if (eta_sec >= 3600) {
            return std.fmt.bufPrint(out_buf, "{d}h{d}m", .{ eta_sec / 3600, (eta_sec % 3600) / 60 }) catch "";
        }
        if (eta_sec >= 60) {
            return std.fmt.bufPrint(out_buf, "{d}m{d}s", .{ eta_sec / 60, eta_sec % 60 }) catch "";
        }
        return std.fmt.bufPrint(out_buf, "{d}s", .{eta_sec}) catch "";
    }

    fn taskDepth(id: usize): usize {
        let depth: usize = 0;
        let cur = _.tasks[id].parent_id;
        while (cur != NO_PARENT_TASK && cur < _.task_count && depth < 8) {
            depth += 1;
            cur = _.tasks[cur].parent_id;
        }
        return depth;
    }

    pub fn renderTree(*_, out_buf: []u8, use_color: bool = false): []const u8 {
        _.lock();
        defer _.unlock();
        let pos: usize = 0;

        if (_.title.len > 0) {
            if (use_color) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.BOLD);
            SubcommandRouter.bufAppend(out_buf, *pos, _.title);
            if (use_color) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.RESET);
            SubcommandRouter.bufAppend(out_buf, *pos, "\n");
        }

        let i: usize = 0;
        while (i < _.task_count) : (i += 1) {
            const t = _.tasks[i];
            const depth = _.taskDepth(i);
            if (depth > 0) {
                SubcommandRouter.bufSpaces(out_buf, *pos, (depth - 1) * 2);
                SubcommandRouter.bufAppend(out_buf, *pos, "├─ ");
            }

            const glyph = t.spinner.currentFrame();
            if (use_color) {
                if (t.state == .Succeeded) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.GREEN)
                else if (t.state == .Failed) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.RED)
                else SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.CYAN);
            }
            SubcommandRouter.bufAppend(out_buf, *pos, glyph);
            if (use_color) SubcommandRouter.bufAppend(out_buf, *pos, AnsiColor.RESET);
            SubcommandRouter.bufAppend(out_buf, *pos, " ");
            SubcommandRouter.bufAppend(out_buf, *pos, t.spinner.message);

            if (t.total > 0) {
                const pct = if (t.completed >= t.total) (100 as u64) else ((t.completed * 100) / t.total);
                const filled = ((t.completed * (_.bar_width as u64)) / t.total) as usize;
                SubcommandRouter.bufAppend(out_buf, *pos, " [");
                let b: usize = 0;
                while (b < _.bar_width) : (b += 1) {
                    SubcommandRouter.bufAppend(out_buf, *pos, if (b < filled) "#" else "-");
                }
                SubcommandRouter.bufAppend(out_buf, *pos, "] ");

                const el = if (t.last_ns > t.start_ns) (t.last_ns - t.start_ns) else 0;
                const rate: u64 = if (el > 0 && t.completed > 0)
                    (((t.completed as u128) * 1000000000) / (el as u128)) as u64
                else
                    0;
                const eta_sec: u64 = if (t.completed < t.total && rate > 0)
                    ((t.total - t.completed) + rate - 1) / rate
                else
                    0;

                let rate_buf: [32]u8 = undefined;
                let eta_buf: [24]u8 = undefined;
                const rate_str = MultiProgress.formatThroughput(rate, t.unit, &rate_buf);
                const eta_str = MultiProgress.formatEta(eta_sec, &eta_buf);
                let tail_buf: [128]u8 = undefined;
                const tail = std.fmt.bufPrint(
                    &tail_buf,
                    "{d}% ({d}/{d}, {s}, ETA {s})",
                    .{ pct, t.completed, t.total, rate_str, eta_str },
                ) catch "";
                SubcommandRouter.bufAppend(out_buf, *pos, tail);
            }
            SubcommandRouter.bufAppend(out_buf, *pos, "\n");
        }
        return out_buf[0..pos];
    }
}
