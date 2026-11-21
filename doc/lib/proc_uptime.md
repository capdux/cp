# cap:proc_uptime

> 源码吸收自 `/data/dev/bun/src/sys.zig`（Linux `/proc/uptime` 零分配整数解析器），提供纯 CP、零外部依赖、零堆分配、零浮点误差的系统运行时间与多核空闲占比解析及可读格式化库。

## 导入方式

```cp
import {
    ProcUptime,
    parseProcUptime,
} from "cap:proc_uptime";
```

## 核心类型与 API

### `ProcUptime`

```cp
pub struct ProcUptime {
    uptime_ms: u64;
    idle_ms: u64;

    fn uptimeSeconds(): u64;
    fn idleSeconds(): u64;
    fn idleRatioPermille(cpu_count: u32): u32;
    fn formatHuman(out: []u8): ([]const u8)?;
}
```

| 方法 | 说明 |
| --- | --- |
| `uptimeSeconds()` | 返回系统运行时长整秒数 |
| `idleSeconds()` | 返回累计空闲时长整秒数 |
| `idleRatioPermille(cpu_count)` | 计算全核空闲占比千分比 `0..=1000`（`u128` 防溢出，`cpu_count == 0` 或 `uptime_ms == 0` 返回 `0`） |
| `formatHuman(out)` | 格式化为可读时长（如 `"3d 04h 12m 05s"`、`"4h 12m 05s"`、`"12m 05s"`、`"45s"`），缓冲不足返回 `null` |

### `parseProcUptime`

- `fn parseProcUptime(content: []const u8): ProcUptime?`
- 解析 `"<uptime_sec>[.<frac>] <idle_sec>[.<frac>]\n"`，支持 0~3+ 位小数自动补零/截断到毫秒；严格拦截缺失字段、多余字段、非法字符与 `u64` 溢出。

## 使用示例

```cp
import { parseProcUptime } from "cap:proc_uptime";

fn example() {
    if (up := parseProcUptime("274325.45 987654.12\n")) {
        let buf: [64]u8 = undefined;
        * = up.formatHuman(&buf);         // "3d 04h 12m 05s"
        * = up.idleRatioPermille(4);      // 900
    }
}
```
