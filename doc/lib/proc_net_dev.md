# cap:proc_net_dev

> 源码吸收自 `/data/dev/bun/src/sys.zig`（Linux `/proc/net/dev` 网络接口流量统计解析器），提供纯 CP、零外部依赖、零堆分配的网络接口收发字节、包数、错误与丢包统计流式解析器。

## 导入方式

```cp
import {
    NetDevStats,
    ProcNetDevIter,
    findNetDev,
    sumNonLoopbackBytes,
} from "cap:proc_net_dev";
```

## 核心类型与 API

### `NetDevStats`

```cp
pub struct NetDevStats {
    iface: []const u8;
    rx_bytes: u64;
    rx_packets: u64;
    rx_errs: u64;
    rx_drop: u64;
    tx_bytes: u64;
    tx_packets: u64;
    tx_errs: u64;
    tx_drop: u64;

    fn totalBytes(): u64;   // rx_bytes + tx_bytes（饱和加法防溢出）
    fn totalErrors(): u64;  // rx_errs + tx_errs（饱和加法防溢出）
    fn isLoopback(): bool;  // iface == "lo"
}
```

### `ProcNetDevIter` 与查询函数

| API | 签名 | 说明 |
| --- | --- | --- |
| `ProcNetDevIter.init` | `fn init(content: []const u8): ProcNetDevIter` | 初始化零分配流式迭代器 |
| `ProcNetDevIter.next` | `fn next(*_): NetDevStats?` | 自动跳过表头、空行与畸形行，支持 `iface:12345` 冒号无空格粘连及别名接口，解析 16 列提取核心 8 项指标 |
| `findNetDev` | `fn findNetDev(content: []const u8, iface: []const u8): NetDevStats?` | 按接口名精确查找接口统计 |
| `sumNonLoopbackBytes` | `fn sumNonLoopbackBytes(content: []const u8): u64` | 汇总所有非 `lo` 接口的总收发字节数（饱和加法） |

## 使用示例

```cp
import { findNetDev, sumNonLoopbackBytes } from "cap:proc_net_dev";

fn example(proc_dev_text: []const u8) {
    if (eth0 := findNetDev(proc_dev_text, "eth0")) {
        * = eth0.totalBytes();
        * = eth0.totalErrors();
    }
    * = sumNonLoopbackBytes(proc_dev_text);
}
```
