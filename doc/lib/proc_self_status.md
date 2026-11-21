# cap:proc_self_status

> 源码吸收自 `/data/dev/bun/src/sys.zig`（Linux `/proc/<pid>/status` 零分配解析器），提供纯 CP、零外部依赖、零堆分配的进程名称、状态、PID/PPID、线程数、UID/GID、内存指标（VmPeak/VmSize/VmRSS/VmHWM）与上下文切换统计解析库。

## 导入方式

```cp
import {
    ProcStatus,
    parseProcStatus,
    findProcStatusField,
} from "cap:proc_self_status";
```

## 核心类型与 API

### `ProcStatus`

```cp
pub struct ProcStatus {
    name: []const u8;
    state_code: u8;
    pid: u32;
    ppid: u32;
    threads: u32;
    uid_real: u32;
    uid_effective: u32;
    gid_real: u32;
    gid_effective: u32;
    vm_peak_kb: u64;
    vm_size_kb: u64;
    vm_rss_kb: u64;
    vm_hwm_kb: u64;
    voluntary_ctxt_switches: u64;
    nonvoluntary_ctxt_switches: u64;

    fn isRunning(): bool;
    fn isZombie(): bool;
    fn rssBytes(): u64;
}
```

### 解析与字段查询函数

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `parseProcStatus` | `fn parseProcStatus(content: []const u8): ProcStatus?` | 单遍零分配扫描 `Key:\tValue` 行，兼容空格/Tab 与 `kB`/`MB` 单位；缺失的可选字段置 0，空或无有效字段返回 `null` |
| `findProcStatusField` | `fn findProcStatusField(content: []const u8, key: []const u8): ([]const u8)?` | 按字段名零拷贝提取去首尾空白的原始值切片（如 `"Cpus_allowed_list"`） |

## 使用示例

```cp
import { parseProcStatus, findProcStatusField } from "cap:proc_self_status";

fn example(status_text: []const u8) {
    if (st := parseProcStatus(status_text)) {
        * = st.pid;
        * = st.rssBytes();
        * = st.isRunning();
    }
    if (cpus := findProcStatusField(status_text, "Cpus_allowed_list")) {
        * = cpus; // "0-7"
    }
}
```
