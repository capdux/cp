// Source: Absorbed from /data/dev/bun/src/clap/clap.cp | Synced: 2026-11-23T21:05:03Z
// Comptime CLI argument parser with strongly-typed field access
import std from "std";
import { Param, Names, Diagnostic, ParseOptions } from "./clap.cp";
import { StreamingClap } from "./streaming.cp";


type ParamUsize = Param(usize);
type OptStr = ([]const u8)?;
type StrSlice = []const []const u8;
type StrList = std.array_list.Managed(string);

pub fn ComptimeClap(
    comptime Id: type,
    comptime params: []const Param(Id),
): type {
    let _flags: usize = 0;
    let _single_options: usize = 0;
    let _multi_options: usize = 0;
    let _converted_params: [ParamUsize; params.len] = [ParamUsize{}; params.len];
    let p_i: usize = 0;
    while (p_i < params.len) : (p_i += 1) {
        const param = params[p_i];
        let index: usize = 0;
        if (param.names.long != null || param.names.short != null) {
            switch (param.takes_value) {
                .none => {
                    index = _flags;
                    _flags += 1;
                },
                .one_optional, .one => {
                    index = _single_options;
                    _single_options += 1;
                },
                .many => {
                    index = _multi_options;
                    _multi_options += 1;
                },
            }
        }

        _converted_params[p_i] = ParamUsize{
            id: index,
            names: param.names,
            takes_value: param.takes_value,
        };
    }
    const flags = _flags;
    const single_options = _single_options;
    const multi_options = _multi_options;
    const converted_params = _converted_params;

    return struct {
        const Self = @This();

        single_options: [single_options]OptStr,
        multi_options: [multi_options]StrSlice,
        flags: [flags]bool,
        pos: StrSlice,
        passthrough_positionals: StrSlice,
        allocator: std.mem.Allocator,

        pub static fn parse(iter: auto, opt: ParseOptions): Self! {
            const allocator = opt.allocator;
            let multis: [multi_options]StrList = undefined;
            let m_i: usize = 0;
            while (m_i < multi_options) : (m_i += 1) {
                multis[m_i] = StrList.init(allocator);
            }

            let pos = StrList.init(allocator);
            let passthrough_positionals = StrList.init(allocator);

            let res = Self{
                single_options: undefined,
                multi_options: undefined,
                flags: undefined,
                pos: undefined,
                allocator: allocator,
                passthrough_positionals: undefined,
            };
            @memset(&res.single_options, null);
            @memset(&res.flags, false);

            const StreamType = StreamingClap(usize, @typeInfo(@TypeOf(iter)).pointer.child);
            let stream = StreamType{
                params: &converted_params,
                iter: iter,
                diagnostic: opt.diagnostic,
            };

            while (arg := stream.next()?) {
                const param = arg.param;
                const arg_val = @field(arg, "value");
                if (param.names.long == null && param.names.short == null) {
                    pos.append(arg_val.?) catch unreachable;
                    if (opt.stop_after_positional_at > 0 && pos.items.len >= opt.stop_after_positional_at) {
                        let remaining_ = stream.iter.remain;
                        const first: []const u8 = if (remaining_.len > 0) remaining_[0] else "";
                        if (first.len > 0 && std.mem.eql(u8, first, "--")) {
                            remaining_ = remaining_[1..];
                        }

                        passthrough_positionals.ensureTotalCapacityPrecise(remaining_.len) catch unreachable;
                        let r_i: usize = 0;
                        while (r_i < remaining_.len) : (r_i += 1) {
                            passthrough_positionals.appendAssumeCapacity(remaining_[r_i]);
                        }
                        break;
                    }
                } else if (param.takes_value == .one || param.takes_value == .one_optional) {
                    std.debug.assert(res.single_options.len != 0);
                    if (res.single_options.len != 0)
                        res.single_options[param.id] = arg_val ?? "";
                } else if (param.takes_value == .many) {
                    std.debug.assert(multis.len != 0);
                    if (multis.len != 0)
                        multis[param.id].append(arg_val.?) catch unreachable;
                } else {
                    std.debug.assert(res.flags.len != 0);
                    if (res.flags.len != 0)
                        res.flags[param.id] = true;
                }
            }

            let out_i: usize = 0;
            while (out_i < multi_options) : (out_i += 1) {
                res.multi_options[out_i] = multis[out_i].toOwnedSlice() catch unreachable;
            }
            res.pos = pos.toOwnedSlice() catch unreachable;
            res.passthrough_positionals = passthrough_positionals.toOwnedSlice() catch unreachable;
            return res;
        }

        pub fn deinit(*_) {
            let i: usize = 0;
            while (i < _.multi_options.len) : (i += 1) {
                _.allocator.free(_.multi_options[i]);
            }
            _.allocator.free(_.pos);
        }

        pub fn flag(*_, comptime name: []const u8): bool {
            const param = comptime findParam(name);
            if (param.takes_value != .none && param.takes_value != .one_optional)
                @compileError(std.fmt.comptimePrint("{s} is an option and not a flag.", [name]));

            return _.flags[param.id];
        }

        pub fn option(*_, comptime name: []const u8): ([]const u8)? {
            const param = comptime findParam(name);
            if (param.takes_value == .none)
                @compileError(std.fmt.comptimePrint("{s} is a flag and not an option.", [name]));
            if (param.takes_value == .many)
                @compileError(std.fmt.comptimePrint("{s} takes many options, not one.", [name]));
            return _.single_options[param.id];
        }

        pub fn options(*_, comptime name: []const u8): []const []const u8 {
            const param = comptime findParam(name);
            if (param.takes_value == .none)
                @compileError(std.fmt.comptimePrint("{s} is a flag and not an option.", [name]));
            if (param.takes_value == .one || param.takes_value == .one_optional)
                @compileError(std.fmt.comptimePrint("{s} takes one option, not multiple.", [name]));

            return _.multi_options[param.id];
        }

        pub fn positionals(*_): []const []const u8 {
            return _.pos;
        }

        pub fn remaining(*_): []const []const u8 {
            return _.passthrough_positionals;
        }

        pub static fn hasFlag(comptime name: []const u8): bool {
            return comptime (findParamOpt(name) != null);
        }

        static fn findParam(comptime name: []const u8): ParamUsize {
            if (p := comptime findParamOpt(name)) return p;
            @compileError(std.fmt.comptimePrint("{s} is not a parameter.", [name]));
        }

        static fn findParamOpt(comptime name: []const u8): ParamUsize? {
            comptime {
                let i: usize = 0;
                while (i < converted_params.len) : (i += 1) {
                    const param = converted_params[i];
                    if (s := param.names.short) {
                        if (name.len == 2 && name[0] == '-' && name[1] == s)
                            return param;
                    }
                    if (l := param.names.long) {
                        if (std.mem.startsWith(u8, name, "--") && std.mem.eql(u8, name[2..], l))
                            return param;
                    }
                    let a_i: usize = 0;
                    while (a_i < param.names.long_aliases.len) : (a_i += 1) {
                        const alias = param.names.long_aliases[a_i];
                        if (std.mem.startsWith(u8, name, "--") && std.mem.eql(u8, name[2..], alias))
                            return param;
                    }
                }
                return null;
            }
        }
    };
}
