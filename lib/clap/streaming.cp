// Source: Absorbed from /data/dev/bun/src/clap/clap.cp | Synced: 2026-11-23T21:05:03Z
// Streaming argument parser for incremental command-line processing
import std from "std";
import { Param, Names, Diagnostic } from "./clap.cp";


pub let warn_on_unrecognized_flag = false;

/// The result returned from StreamingClap.next
pub fn Arg(comptime Id: type): type {
    return struct {
        param: &Param(Id),
        value: ([]const u8)? = null,
    };
}

pub struct Chaining {
    arg: []const u8,
    index: usize,
}

pub enum State {
    normal,
    chaining: Chaining,
    rest_are_positional,
}

pub err ArgError {
    DoesntTakeValue,
    MissingValue,
    InvalidArgument,
}

pub enum ArgKind {
    long,
    short,
    positional,
}

pub struct ArgInfo {
    arg: []const u8,
    kind: ArgKind,
}

/// A command line argument parser which, given an ArgIterator, will parse arguments according
/// to the params. StreamingClap parses in an iterating manner.
pub fn StreamingClap(comptime Id: type, comptime ArgIterator: type): type {
    return struct {
        const ArgType = Arg(Id);

        params: []const Param(Id),
        iter: *ArgIterator,
        state: State = .normal,
        positional: (&Param(Id))? = null,
        diagnostic: (*Diagnostic)? = null,

        /// Get the next Arg that matches a Param.
        pub fn next(*_): Arg(Id)? ! ArgError {
            switch (_.state) {
                .normal => return _.normal()?,
                .chaining state => return _.chaining(state)?,
                .rest_are_positional => {
                    const param = _.positionalParam() ?? unreachable;
                    if (value := _.iter.next()) {
                        return ArgType{ param: param, value: value };
                    }
                    return null;
                },
            }
        }

        fn normal(*_): Arg(Id)? ! ArgError {
            const arg_info_opt = _.parseNextArg()?;
            if (arg_info_opt == null) return null;
            const arg_info = arg_info_opt.?;
            const arg = arg_info.arg;

            switch (arg_info.kind) {
                .long => {
                    const eql_index = std.mem.indexOfScalar(u8, arg, '=');
                    const name = if (eql_index != null) arg[0..eql_index.?] else arg;
                    const maybe_value = if (eql_index != null) arg[eql_index.? + 1 ..] else null;

                    let p_idx: usize = 0;
                    while (p_idx < _.params.len) : (p_idx += 1) {
                        const param = &_.params[p_idx];
                        if (!param.names.matchesLong(name))
                            continue;

                        if (param.takes_value == .none || param.takes_value == .one_optional) {
                            if (param.takes_value == .none && maybe_value != null) {
                                return _.makeErr(arg, Names{ long: name }, ArgError.DoesntTakeValue);
                            }

                            return ArgType{ param: param, value: maybe_value };
                        }

                        if (v := maybe_value) {
                            return ArgType{ param: param, value: v };
                        }
                        if (v := _.iter.next()) {
                            return ArgType{ param: param, value: v };
                        }
                        return _.makeErr(arg, Names{ long: name }, ArgError.MissingValue);
                    }

                    if (arg_info.kind == .long || arg_info.kind == .short) {
                        if (warn_on_unrecognized_flag) {
                            const pfx = if (arg_info.kind == .long) "--" else "-";
                            print(`unrecognized flag: {pfx}{name}\n`);
                        }
                        return _.next();
                    }

                    if (warn_on_unrecognized_flag) {
                        print(`unrecognized argument: {name}\n`);
                    }
                    return null;
                },
                .short => return _.chaining(Chaining{
                    arg: arg,
                    index: 0,
                })?,
                .positional => {
                    if (param := _.positionalParam()) {
                        if (std.mem.eql(u8, arg, "--")) {
                            _.state = .rest_are_positional;
                            if (value := _.iter.next()) {
                                return ArgType{ param: param, value: value };
                            }
                            return null;
                        }

                        return ArgType{ param: param, value: arg };
                    } else {
                        return _.makeErr(arg, Names{}, ArgError.InvalidArgument);
                    }
                },
            }
        }

        fn chaining(*_, state: Chaining): Arg(Id)? ! ArgError {
            const arg = state.arg;
            const index = state.index;
            const next_index = index + 1;

            let p_idx: usize = 0;
            while (p_idx < _.params.len) : (p_idx += 1) {
                const param = &_.params[p_idx];
                const short = param.names.short ?? continue;
                if (short != arg[index])
                    continue;

                defer {
                    if (arg.len <= next_index || param.takes_value != .none) {
                        _.state = .normal;
                    } else {
                        _.state = State{
                            chaining: Chaining{
                                arg: arg,
                                index: next_index,
                            },
                        };
                    }
                }

                const next_is_eql = if (next_index < arg.len) arg[next_index] == '=' else false;
                if (param.takes_value == .none || param.takes_value == .one_optional) {
                    if (next_is_eql && param.takes_value == .none)
                        return _.makeErr(arg, Names{ short: short }, ArgError.DoesntTakeValue);
                    return ArgType{ param: param };
                }

                if (arg.len <= next_index) {
                    if (value := _.iter.next()) {
                        return ArgType{ param: param, value: value };
                    }
                    return _.makeErr(arg, Names{ short: short }, ArgError.MissingValue);
                }

                if (next_is_eql)
                    return ArgType{ param: param, value: arg[next_index + 1 ..] };

                return ArgType{ param: param, value: arg[next_index..] };
            }

            return _.makeErr(arg, Names{ short: arg[index] }, ArgError.InvalidArgument);
        }

        fn positionalParam(*_): (&Param(Id))? {
            if (p := _.positional)
                return p;

            let p_idx: usize = 0;
            while (p_idx < _.params.len) : (p_idx += 1) {
                const param = &_.params[p_idx];
                if (param.names.long != null)
                    continue;
                if (param.names.short != null)
                    continue;

                _.positional = param;
                return param;
            }

            return null;
        }

        fn parseNextArg(*_): ArgInfo? ! ArgError {
            if (full_arg := _.iter.next()) {
                if (std.mem.eql(u8, full_arg, "--") || std.mem.eql(u8, full_arg, "-"))
                    return ArgInfo{ arg: full_arg, kind: .positional };
                if (std.mem.startsWith(u8, full_arg, "--"))
                    return ArgInfo{ arg: full_arg[2..], kind: .long };
                if (std.mem.startsWith(u8, full_arg, "-"))
                    return ArgInfo{ arg: full_arg[1..], kind: .short };

                return ArgInfo{ arg: full_arg, kind: .positional };
            }
            return null;
        }

        fn makeErr(*_, arg: []const u8, names: Names, _err: ArgError): ArgError {
            if (d := _.diagnostic) {
                d.* = Diagnostic{ arg: arg, name: names };
            }
            return _err;
        }
    };
}
