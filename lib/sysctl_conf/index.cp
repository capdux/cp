// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux sysctl.conf Zero-Alloc Parser)

export {
    SysctlEntry,
    SysctlIter,
    normalizeSysctlKey,
    sysctlKeysEqual,
    getSysctlValue,
} from "./sysctl.cp";
