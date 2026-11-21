# cap:cgroup_v2_limits

> 源码吸收自 `/data/dev/bun/src/sys.zig`（Linux cgroup v2 容器资源限额解析器），提供纯 CP、零外部依赖、零堆分配的 `cpu.max`、`memory.max`、`pids.max`、`io.max` 配额文件解析与有效容器资源推导库。

## 导入方式

```cp
import {
    CgroupLimit,
    CpuMaxQuota,
    IoMaxEntry,
    parseCgroupMax,
    parseCpuMax,
    parseIoMaxLine,
    effectiveMemoryLimit,
} from "cap:cgroup_v2_limits";
```

## 核心类型与 API

### `CgroupLimit` 与 `parseCgroupMax`

```cp
pub struct CgroupLimit {
    is_unlimited: bool;
    value: u64;

    fn unlimited(): CgroupLimit;
}
```

- `fn parseCgroupMax(content: []const u8): CgroupLimit?`：解析 `memory.max` / `pids.max` 的 `"max\n"` 或无符号整数，严格拦截负数、非数字、多余字段与溢出。

### `CpuMaxQuota` 与 `parseCpuMax`

```cp
pub struct CpuMaxQuota {
    quota_us: u64;
    period_us: u64;
    is_unlimited: bool;

    fn effectiveCpusCeil(): u32;   // 无限制返回 0，否则 ceil(quota_us / period_us) 且最小为 1
    fn effectiveMilliCpus(): u64;  // 无限制返回 0，否则 quota_us * 1000 / period_us
}
```

- `fn parseCpuMax(content: []const u8): CpuMaxQuota?`：解析 `"200000 100000\n"` 或 `"max 100000\n"`，拒绝 `period_us == 0` 与畸形输入。

### `IoMaxEntry` 与 `parseIoMaxLine`

```cp
pub struct IoMaxEntry {
    major: u32;
    minor: u32;
    rbps: CgroupLimit;
    wbps: CgroupLimit;
    riops: CgroupLimit;
    wiops: CgroupLimit;
}
```

- `fn parseIoMaxLine(line: []const u8): IoMaxEntry?`：解析 `"8:0 rbps=1048576 wbps=max riops=max wiops=1000"`。

### `effectiveMemoryLimit`

- `fn effectiveMemoryLimit(cgroup_max: CgroupLimit, host_mem_bytes: u64): u64`：在容器内存限额与宿主机物理内存之间取较小值。

## 使用示例

```cp
import { parseCpuMax, parseCgroupMax, effectiveMemoryLimit } from "cap:cgroup_v2_limits";

fn example() {
    if (cpu := parseCpuMax("150000 100000\n")) {
        * = cpu.effectiveCpusCeil();  // 2
        * = cpu.effectiveMilliCpus(); // 1500
    }
    if (mem := parseCgroupMax("536870912\n")) {
        * = effectiveMemoryLimit(mem, 16 * 1024 * 1024 * 1024); // 536870912
    }
}
```
