// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux cgroup v2 Resource Limits Parser)

export {
    CgroupLimit,
    CpuMaxQuota,
    IoMaxEntry,
    parseCgroupMax,
    parseCpuMax,
    parseIoMaxLine,
    effectiveMemoryLimit,
} from "./cgroup.cp";
