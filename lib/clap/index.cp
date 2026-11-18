// Source: bun/src/clap/clap.cp
// CP Standard Library: cap:clap
// Fast, robust command-line argument parsing with comptime validation
import clap from "./clap.cp";
import args from "./args.cp";
import streaming from "./streaming.cp";
import comptime_mod from "./comptime.cp";

export {
    Names,
    Values,
    Param,
    Help,
    ParamHelp,
    Diagnostic,
    ParseOptions,
    Args,
    parseParam,
    parse,
    parseEx,
    help,
    helpEx,
    helpFull,
    usage,
    usageEx,
    usageFull,
} from "./clap.cp";

export {
    ComptimeClap,
} from "./comptime.cp";

export {
    Arg,
    ArgError,
    ArgInfo,
    ArgKind,
    State,
    Chaining,
    StreamingClap,
} from "./streaming.cp";

export {
    SliceIterator,
    ShellIterator,
    OsIterator,
    ExampleArgIterator,
    ShellState,
    ShellError,
} from "./args.cp";
