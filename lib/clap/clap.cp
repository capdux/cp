// Source: Absorbed from /data/dev/bun/src/clap/clap.cp | Synced: 2026-11-20T11:53:32Z
import { OsIterator } from "./args.cp";
import { ComptimeClap } from "./comptime.cp";
// CP Standard Library: cap:clap
// Fast, robust command-line argument parsing with comptime validation
import std from "std";

const empty_aliases: []const string = [];

/// The names a Param can have.
pub struct Names {
    /// '-' prefix
    short: u8? = null,

    /// '--' prefix (primary name, used for display/help)
    long: ([]const u8)? = null,

    /// Additional '--' prefixed aliases
    long_aliases: []const []const u8 = empty_aliases,

    /// Check if the given name matches the primary long name or any alias
    pub fn matchesLong(name: []const u8): bool {
        if (l := _.long) {
            if (std.mem.eql(u8, name, l)) return true;
        }
        let i: usize = 0;
        while (i < _.long_aliases.len) : (i += 1) {
            if (std.mem.eql(u8, name, _.long_aliases[i])) return true;
        }
        return false;
    }
}

/// Whether a param takes no value (a flag), one value, or can be specified multiple times.
pub enum Values {
    none,
    one,
    many,
    one_optional,
}

/// Represents a parameter for the command line.
pub fn Param(comptime Id: type): type {
    return struct {
        id: Id = std.mem.zeroes(Id),
        names: Names = std.mem.zeroes(Names),
        takes_value: Values = .none,
    };
}

pub struct Help {
    msg: []const u8 = "",
    value: []const u8 = "",
}

pub type ParamHelp = Param(Help);

pub err ParseParamError {
    NoParamFound,
    InvalidShortParam,
    TrailingComma,
}

/// Takes a string and parses it to a Param(Help).
pub fn parseParam(line: []const u8): ParamHelp ! ParseParamError {
    @setEvalBranchQuota(999999);

    let found_comma = false;
    let it = std.mem.tokenizeAny(u8, line, " \t");
    const first_tok = it.next();
    if (first_tok == null) return ParseParamError.NoParamFound;
    let param_str = first_tok.?;

    let short_name: u8? = null;
    if (!std.mem.startsWith(u8, param_str, "--") && std.mem.startsWith(u8, param_str, "-")) {
        found_comma = param_str[param_str.len - 1] == ',';
        if (found_comma)
            param_str = param_str[0 .. param_str.len - 1];

        if (param_str.len != 2)
            return ParseParamError.InvalidShortParam;

        const s_name = param_str[1];
        if (!found_comma) {
            let res = parseParamRest(it.rest());
            res.names.short = s_name;
            return res;
        }

        const next_tok = it.next();
        if (next_tok == null) return ParseParamError.NoParamFound;
        param_str = next_tok.?;
        short_name = s_name;
    }

    if (std.mem.startsWith(u8, param_str, "--")) {
        if (param_str[param_str.len - 1] == ',')
            return ParseParamError.TrailingComma;
    } else if (found_comma) {
        return ParseParamError.TrailingComma;
    } else if (short_name == null) {
        return parseParamRest(std.mem.trimStart(u8, line, " \t"));
    }

    let res = parseParamRest(it.rest());
    res.names.short = short_name;

    const long_names = parseLongNames(param_str);
    res.names.long = long_names.long;
    res.names.long_aliases = long_names.long_aliases;
    return res;
}

fn parseLongNames(param_str: []const u8): Names {
        let alias_count: usize = 0;
        let c_i: usize = 0;
        while (c_i < param_str.len) : (c_i += 1) {
            if (param_str[c_i] == '/') alias_count += 1;
        }

        if (alias_count == 0) {
            if (std.mem.startsWith(u8, param_str, "--")) {
                return Names{ long: param_str[2..], long_aliases: empty_aliases };
            }
            return Names{ long: null, long_aliases: empty_aliases };
        }

        let primary: string? = null;
        let name_it = std.mem.splitScalar(u8, param_str, '/');
        while (name_part := name_it.next()) {
            const np: string = name_part;
            if (!std.mem.startsWith(u8, np, "--")) continue;
            primary = np[2..];
            break;
        }

        return Names{
            long: primary,
            long_aliases: empty_aliases,
        };
    }

fn parseParamRest(line: []const u8): ParamHelp {
    if (std.mem.startsWith(u8, line, "<")) {
        if (len := std.mem.indexOfScalar(u8, line, '>')) {
            const takes_many = std.mem.startsWith(u8, line[len + 1 ..], "...");
            const takes_one_optional = std.mem.startsWith(u8, line[len + 1 ..], "?");
            let many_off: usize = 0;
            if (takes_many) many_off = 3;
            let opt_off: usize = 0;
            if (takes_one_optional) opt_off = 1;
            const help_start = len + 1 + many_off + opt_off;
            return ParamHelp{
                takes_value: if (takes_many) Values.many else if (takes_one_optional) Values.one_optional else Values.one,
                id: Help{
                    msg: std.mem.trim(u8, line[help_start..], " \t"),
                    value: line[1..len],
                },
            };
        }
    }

    return ParamHelp{ id: Help{ msg: std.mem.trim(u8, line, " \t") } };
}

/// Optional diagnostics used for reporting useful errors
pub struct Diagnostic {
    arg: []const u8 = "",
    name: Names = Names{},

    pub fn report(*_, stream: auto, the_err: anyerror)! {
        * = stream;
        let name_buf: [1024]u8 = undefined;
        let arg_name: string = _.arg;
        if (s := _.name.short) {
            name_buf[0] = '-';
            name_buf[1] = s;
            name_buf[2] = 0;
            arg_name = name_buf[0..2];
        } else if (l := _.name.long) {
            name_buf[0] = '-';
            name_buf[1] = '-';
            const max_l = @min(l.len, name_buf.len - 2);
            const long = l[0..max_l];
            @memcpy(name_buf[2 .. 2 + long.len], long);
            arg_name = name_buf[0 .. 2 + long.len];
        }

        switch (the_err) {
            error.DoesntTakeValue => {
                std.debug.print("error: The argument '{s}' does not take a value.\n", [arg_name]);
            },
            error.MissingValue => {
                std.debug.print("error: The argument '{s}' requires a value but none was supplied.\n", [arg_name]);
            },
            error.InvalidArgument => {
                std.debug.print("error: Invalid Argument '{s}'\n", [arg_name]);
            },
            else => {
                std.debug.print("error: {s} while parsing argument '{s}'\n", [@errorName(the_err), arg_name]);
            },
        }
    }
}

pub fn Args(comptime Id: type, comptime params: []const Param(Id)): type {
    const ClapType = ComptimeClap(Id, params);
    return struct {
        arena_allocator: std.heap.ArenaAllocator,
        clap: ClapType,
        exe_arg: ([]const u8)?,

        pub fn deinit(*_) {
            _.arena_allocator.deinit();
        }

        pub fn flag(*_, comptime name: []const u8): bool {
            return _.clap.flag(name);
        }

        pub fn option(*_, comptime name: []const u8): ([]const u8)? {
            return _.clap.option(name);
        }

        pub fn options(*_, comptime name: []const u8): []const []const u8 {
            return _.clap.options(name);
        }

        pub fn positionals(*_): []const []const u8 {
            return _.clap.positionals();
        }

        pub fn remaining(*_): []const []const u8 {
            return _.clap.remaining();
        }

        pub static fn hasFlag(comptime name: []const u8): bool {
            return ClapType.hasFlag(name);
        }
    };
}

/// Options that can be set to customize the behavior of parsing.
pub struct ParseOptions {
    allocator: std.mem.Allocator = std.heap.page_allocator,
    diagnostic: (*Diagnostic)? = null,
    stop_after_positional_at: usize = 0,
}

/// Same as `parseEx` but uses the `OsIterator` by default.
pub fn parseArgs(
    comptime Id: type,
    comptime params: []const Param(Id),
    opt: ParseOptions,
): Args(Id, params)! {
    const ArgsType = Args(Id, params);
    let iter = OsIterator.init(opt.allocator);
    let res = ArgsType{
        arena_allocator: iter.arena_allocator,
        exe_arg: iter.exe_arg,
        clap: undefined,
    };

    res.clap = parseEx(Id, params, &iter, ParseOptions{
        allocator: res.arena_allocator.allocator(),
        diagnostic: opt.diagnostic,
        stop_after_positional_at: opt.stop_after_positional_at,
    })?;
    return res;
}

/// Parses the command line arguments passed into the program based on an
/// array of `Param`s.
pub fn parseEx(
    comptime Id: type,
    comptime params: []const Param(Id),
    iter: auto,
    opt: ParseOptions,
): ComptimeClap(Id, params)! {
    const Clap = ComptimeClap(Id, params);
    return Clap.parse(iter, opt)?;
}

fn paramPrintLength(
    comptime Id: type,
    param: Param(Id),
    context: auto,
    valueText: auto,
): usize! {
    let len: usize = 2;
    if (l := param.names.long) {
        len += 4 + l.len;
    }
    const val_len = (valueText(context, param)?).len;
    switch (param.takes_value) {
        .none => {},
        .one => len += 3 + val_len,
        .one_optional => len += 4 + val_len,
        .many => len += 6 + val_len,
    }
    return len;
}

pub fn helpFull(
    stream: auto,
    comptime Id: type,
    params: []const Param(Id),
    comptime ErrType: type,
    context: auto,
    helpText: auto,
    valueText: auto,
)! {
    let max_spacing: usize = 0;
    let p_i: usize = 0;
    while (p_i < params.len) : (p_i += 1) {
        const param = params[p_i];
        const written = paramPrintLength(Id, param, context, valueText)?;
        if (max_spacing < written)
            max_spacing = written;
    }

    let i: usize = 0;
    while (i < params.len) : (i += 1) {
        const param = params[i];
        if (param.names.short == null && param.names.long == null)
            continue;

        const help_text = helpText(context, param)?;
        if (help_text.len > 0) {
            stream.writeAll("\t")?;
            printParam(stream, Id, param, ErrType, context, valueText)?;
            const written = paramPrintLength(Id, param, context, valueText)?;
            if (max_spacing > written) {
                let pad = max_spacing - written;
                let s_i: usize = 0;
                while (s_i < pad) : (s_i += 1) {
                    stream.writeByte(' ')?;
                }
            }
            stream.writeAll("\t")?;
            stream.writeAll(help_text)?;
            stream.writeByte('\n')?;
        }
    }
}

fn printParam(
    stream: auto,
    comptime Id: type,
    param: Param(Id),
    comptime _Error: type,
    context: auto,
    valueText: auto,
)! {
    * = _Error;
    if (s := param.names.short) {
        stream.writeAll("-")?;
        stream.writeByte(s)?;
    } else {
        stream.writeAll("  ")?;
    }
    if (l := param.names.long) {
        if (param.names.short != null) {
            stream.writeAll(", --")?;
        } else {
            stream.writeAll("  --")?;
        }
        stream.writeAll(l)?;
    }

    switch (param.takes_value) {
        .none => {},
        .one => {
            stream.writeAll(" <")?;
            stream.writeAll(valueText(context, param)?)?;
            stream.writeAll(">")?;
        },
        .one_optional => {
            stream.writeAll(" <")?;
            stream.writeAll(valueText(context, param)?)?;
            stream.writeAll(">?")?;
        },
        .many => {
            stream.writeAll(" <")?;
            stream.writeAll(valueText(context, param)?)?;
            stream.writeAll(">...")?;
        },
    }
}

pub err EmptyErr {}

fn ctxHelp(c: auto, p: auto): ([]const u8) ! EmptyErr {
    return c.helpText(p);
}

fn ctxValue(c: auto, p: auto): ([]const u8) ! EmptyErr {
    return c.valueText(p);
}

/// A wrapper around helpFull for simple helpText and valueText functions.
pub fn helpEx(
    stream: auto,
    comptime Id: type,
    params: []const Param(Id),
    helpText: auto,
    valueText: auto,
)! {
    return helpFull(
        stream,
        Id,
        params,
        EmptyErr,
        {
            helpText: helpText,
            valueText: valueText,
        },
        ctxHelp,
        ctxValue,
    );
}

fn getHelpSimple(param: ParamHelp): []const u8 {
    return param.id.msg;
}

fn getValueSimple(param: ParamHelp): []const u8 {
    return @field(param.id, "value");
}

/// A wrapper around helpEx that takes a Param(Help).
pub fn help(stream: auto, params: []const ParamHelp)! {
    helpEx(stream, Help, params, getHelpSimple, getValueSimple)?;
}

pub fn usageFull(
    stream: auto,
    comptime Id: type,
    params: []const Param(Id),
    comptime _Error: type,
    context: auto,
    valueText: auto,
)! {
    * = _Error;
    let bytes_written: usize = 0;
    let i: usize = 0;
    while (i < params.len) : (i += 1) {
        const param = params[i];
        const name = param.names.short ?? continue;
        if (param.takes_value != .none)
            continue;

        if (bytes_written == 0) {
            stream.writeAll("[-")?;
            bytes_written += 2;
        }
        let bbuf: [1]u8 = [name];
        stream.writeAll(bbuf[0..1])?;
        bytes_written += 1;
    }
    if (bytes_written != 0) {
        stream.writeAll("]")?;
        bytes_written += 1;
    }

    let positional: Param(Id)? = null;
    let j: usize = 0;
    while (j < params.len) : (j += 1) {
        const param = params[j];
        if (param.takes_value == .none && param.names.short != null)
            continue;

        const prefix = if (param.names.short != null) "-" else "--";

        let short_buf: [1]u8 = [0];
        let name: string = "";
        if (s := param.names.short) {
            short_buf[0] = s;
            name = short_buf[0..1];
        } else if (l := param.names.long) {
            name = l;
        } else {
            positional = param;
            continue;
        }
        if (bytes_written != 0) {
            stream.writeAll(" ")?;
            bytes_written += 1;
        }

        stream.writeAll("[")?;
        stream.writeAll(prefix)?;
        stream.writeAll(name)?;
        bytes_written += 1 + prefix.len + name.len;

        const vt = valueText(context, param)?;
        switch (param.takes_value) {
            .none => {},
            .one => {
                stream.writeAll(" <")?;
                stream.writeAll(vt)?;
                stream.writeAll(">")?;
                bytes_written += 3 + vt.len;
            },
            .one_optional => {
                stream.writeAll(" <")?;
                stream.writeAll(vt)?;
                stream.writeAll(">?")?;
                bytes_written += 4 + vt.len;
            },
            .many => {
                stream.writeAll(" <")?;
                stream.writeAll(vt)?;
                stream.writeAll(">...")?;
                bytes_written += 6 + vt.len;
            },
        }

        stream.writeAll("]")?;
        bytes_written += 1;
    }

    if (p := positional) {
        if (bytes_written != 0) {
            stream.writeAll(" ")?;
            bytes_written += 1;
        }
        const p_vt = valueText(context, p)?;
        stream.writeAll("<")?;
        stream.writeAll(p_vt)?;
        stream.writeAll(">")?;
        bytes_written += 2 + p_vt.len;
    }
}

/// A wrapper around usageFull for simple valueText functions.
pub fn usageEx(
    stream: auto,
    comptime Id: type,
    params: []const Param(Id),
    valueText: auto,
)! {
    return usageFull(
        stream,
        Id,
        params,
        EmptyErr,
        { valueText: valueText },
        ctxValue,
    );
}

/// A wrapper around usageEx that takes a Param(Help).
pub fn usage(stream: auto, params: []const ParamHelp)! {
    usageEx(stream, Help, params, getValueSimple)?;
}
