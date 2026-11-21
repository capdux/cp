# `cap:aof` — 纯 CP Redis AOF 命令日志零分配流式解析库

## 概述

`cap:aof` 提供纯 CP、零外部依赖、零堆分配的 Redis Append-Only File（AOF）RESP Multi-Bulk 命令流解析库，完整支持：
- **格式探测**：`isAofMixedRdb` 探测混合 RDB+AOF 的 `"REDIS"` 前导，`isAofResp` 探测标准 RESP `'*'` 与 Redis 7.0 `'#'` 注释流；
- **二进制安全命令迭代**：`AofIter` + `AofArgIter` 零拷贝解析 `*<argc>\r\n` 与二进制安全 `$<len>\r\n<payload>\r\n`（负载内允许任意 `\r\n` 与 `\0`）；
- **Redis 7.0 `#TS:` 时间戳关联与截断尾安全修复**：自动绑定 `#TS:<unix_sec>` 到下一条命令，并在遇到截断尾时保留上一条合法命令末尾的 `consumedBytes()`。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `isAofMixedRdb(data)` / `isAofResp(data)` | 探测混合 RDB+AOF 前导与 RESP/注释 AOF 流 |
| `AofArgIter` | 零拷贝命令参数流式迭代器 |
| `AofCommand` | 单条命令视图：`arg_count`、`raw_args_slice`、`timestamp`、`byte_offset`、`byte_len`；只读方法：`args()`、`name()`、`argAt(idx)` |
| `AofIter` | 流式 AOF 迭代器：`init(data)`、`next(*_): AofCommand?`、`had_error`、`consumedBytes()` |
| `AofStats` / `summarizeAof(data)` | 单遍统计命令数、总参数数、安全消费字节数与是否出现截断尾 |

## 示例

```cp
import { isAofResp, AofIter } from "cap:aof";

fn countSetCommands(aof_data: []const u8): u64 {
    if (!isAofResp(aof_data)) return 0;
    let it = AofIter.init(aof_data);
    let n: u64 = 0;
    while (cmd := it.next()) {
        if (name := cmd.name()) {
            if (name.len == 3 && name[0] == 'S' && name[1] == 'E' && name[2] == 'T') {
                n += 1;
            }
        }
    }
    return n;
}
```
