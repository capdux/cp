# cap:proc_cpuinfo

> 源码吸收自 `/data/dev/bun/src/sys.zig`（Linux `/proc/cpuinfo` 零分配解析器），提供纯 CP、零外部依赖、零堆分配的 x86_64 与 ARM64 CPU 拓扑、主频、缓存与指令集标志位解析库。

## 导入方式

```cp
import {
    CpuBlock,
    CpuSummary,
    ProcCpuInfoIter,
    summarizeCpuInfo,
    hasCpuFlag,
} from "cap:proc_cpuinfo";
```

## 核心类型与 API

### `CpuBlock`

```cp
pub struct CpuBlock {
    processor: u32;
    physical_id: u32;
    cpu_cores: u32;
    model_name: []const u8;
    vendor_id: []const u8;
    cpu_mhz_x1000: u64;
    cache_kb: u64;
    flags: []const u8;

    fn hasFlag(flag: []const u8): bool;
}
```

- 兼容 x86_64（`model name` / `vendor_id` / `flags` / `cpu MHz` / `cache size` / `physical id` / `cpu cores`）与 ARM64（`Features` / `CPU implementer` / `Model` / `Hardware`）。
- `cpu_mhz_x1000` 以整数毫赫兹表示主频（如 `2899.998` → `2899998`），零浮点误差；`cache_kb` 自动换算 `KB`/`MB`/`GB` 为 KB。

### `CpuSummary`

```cp
pub struct CpuSummary {
    logical_cpus: u32;
    physical_cores: u32;
    model_name: []const u8;
    cache_kb: u64;
}
```

### 迭代器与工具函数

| API | 签名 | 说明 |
| --- | --- | --- |
| `ProcCpuInfoIter.init` | `fn init(content: []const u8): ProcCpuInfoIter` | 初始化零分配 `processor` 块迭代器 |
| `ProcCpuInfoIter.next` | `fn next(*_): CpuBlock?` | 逐块解析每个 `processor` 段落（支持空行分隔与紧凑连续块） |
| `summarizeCpuInfo` | `fn summarizeCpuInfo(content: []const u8): CpuSummary` | 统计逻辑核心数与按 `physical id` 去重累加的物理核心数（无 `cpu cores` 时回退为逻辑核心数） |
| `hasCpuFlag` | `fn hasCpuFlag(flags: []const u8, flag: []const u8): bool` | 按空白分隔精确匹配指令集标志位（如 `avx` 不会误匹配 `avx2`） |

## 使用示例

```cp
import { summarizeCpuInfo, hasCpuFlag, ProcCpuInfoIter } from "cap:proc_cpuinfo";

fn example(cpuinfo_text: []const u8) {
    const summary = summarizeCpuInfo(cpuinfo_text);
    * = summary.logical_cpus;
    * = summary.physical_cores;

    let it = ProcCpuInfoIter.init(cpuinfo_text);
    if (cpu0 := it.next()) {
        * = hasCpuFlag(cpu0.flags, "avx2");
    }
}
```
