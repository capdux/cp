// Source: Absorbed from /data/dev/bun/src/sys.zig (POSIX /etc/fstab & /proc/mounts Parser)

export {
    MountEntry,
    MountIter,
    unescapeMountPath,
    findMountForPath,
} from "./fstab.cp";
