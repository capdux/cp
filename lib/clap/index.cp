// Source: Absorbed from /data/dev/bun/src/clap/clap.cp | Synced: 2026-11-20T11:53:32Z
// CP Standard Library: cap:clap
// Fast, robust command-line argument parsing with comptime validation

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
    parseArgs,
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
