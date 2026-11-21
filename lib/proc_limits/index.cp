// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/<pid>/limits Zero-Alloc Parser)

export {
    LimitValue,
    ProcLimitEntry,
    ProcLimitsIter,
    parseLimitValue,
    findProcLimit,
    maxOpenFiles,
} from "./limits.cp";
