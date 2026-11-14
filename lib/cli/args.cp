// lib/cli/args.cp
// Zero-allocation Command-Line Flag, Option, and Positional Argument Parser
// Absorbed from Bun src/clap/args.cp & clap.cp

const std = @import("std");

pub struct SliceIterator {
    remain: []const []const u8;

    pub static fn init(args: []const []const u8): SliceIterator {
        return SliceIterator {
            remain: args,
        };
    }

    pub fn next(*_): ([]const u8)? {
        if (_.remain.len > 0) {
            const res = _.remain[0];
            _.remain = _.remain[1..];
            return res;
        }
        return null;
    }

    pub fn peek(): ([]const u8)? {
        if (_.remain.len > 0) {
            return _.remain[0];
        }
        return null;
    }

    pub fn remainingCount(): usize {
        return _.remain.len;
    }
}

pub struct CliOption {
    name: []const u8 = "";
    value: []const u8 = "";
    has_value: bool = false;
    is_short: bool = false;
}

pub struct CliArgs {
    options: [64]CliOption = undefined;
    option_count: usize = 0;
    positionals: [64][]const u8 = undefined;
    positional_count: usize = 0;

    /// Parses a slice of command-line arguments supporting `--key=value`, `--flag`, `-f`, and `--`.
    pub static fn parse(raw_args: []const []const u8): CliArgs {
        let parsed = CliArgs {
            options: undefined,
            option_count: 0,
            positionals: undefined,
            positional_count: 0,
        };
        let iter = SliceIterator.init(raw_args);
        let only_positionals: bool = false;

        while (arg := iter.next()) {
            if (only_positionals) {
                parsed.addPositional(arg);
                continue;
            }

            if (std.mem.eql(u8, arg, "--")) {
                only_positionals = true;
                continue;
            }

            if (std.mem.startsWith(u8, arg, "--") && arg.len > 2) {
                const body = arg[2..];
                if (eq_idx := std.mem.indexOfScalar(u8, body, '=')) {
                    parsed.addOption(body[0..eq_idx], body[eq_idx + 1 ..], true, false);
                } else {
                    parsed.addOption(body, "", false, false);
                }
                continue;
            }

            if (arg.len >= 2 && arg[0] == '-' && arg[1] != '-') {
                const body = arg[1..];
                if (eq_idx := std.mem.indexOfScalar(u8, body, '=')) {
                    parsed.addOption(body[0..eq_idx], body[eq_idx + 1 ..], true, true);
                } else {
                    // Support combined short flags like -abc or single -v
                    let k: usize = 0;
                    while (k < body.len) {
                        parsed.addOption(body[k .. k + 1], "", false, true);
                        k += 1;
                    }
                }
                continue;
            }

            parsed.addPositional(arg);
        }

        return parsed;
    }

    pub fn hasFlag(name: []const u8): bool {
        let i: usize = 0;
        while (i < _.option_count) {
            if (std.mem.eql(u8, _.options[i].name, name)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn getOption(name: []const u8): ([]const u8)? {
        let i: usize = _.option_count;
        while (i > 0) {
            i -= 1;
            if (std.mem.eql(u8, _.options[i].name, name) && _.options[i].has_value) {
                return _.options[i].value;
            }
        }
        return null;
    }

    pub fn getOptionOr(name: []const u8, default_val: []const u8): []const u8 {
        if (val := _.getOption(name)) {
            return val;
        }
        return default_val;
    }

    pub fn positional(index: usize): ([]const u8)? {
        if (index >= _.positional_count) {
            return null;
        }
        return _.positionals[index];
    }

    fn addOption(*_, name: []const u8, value: []const u8, has_value: bool, is_short: bool) {
        if (_.option_count < 64) {
            _.options[_.option_count] = CliOption {
                name: name,
                value: value,
                has_value: has_value,
                is_short: is_short,
            };
            _.option_count += 1;
        }
    }

    fn addPositional(*_, val: []const u8) {
        if (_.positional_count < 64) {
            _.positionals[_.positional_count] = val;
            _.positional_count += 1;
        }
    }
}
