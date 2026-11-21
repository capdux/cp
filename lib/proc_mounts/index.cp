// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/mounts Zero-Alloc Parser)

export {
    MountEntry,
    ProcMountsIter,
    unescapeMountPath,
    findMountForPath,
} from "./mounts.cp";
