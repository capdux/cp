// Source: Absorbed from /data/dev/bun/src/sys.zig (POSIX /etc/passwd & /etc/group Barrel)
export {
    PasswdEntry,
    PasswdIter,
    lookupUserByName,
    lookupUserByUid,
    GroupEntry,
    GroupIter,
    lookupGroupByName,
    lookupGroupByGid,
} from "./passwd.cp";
