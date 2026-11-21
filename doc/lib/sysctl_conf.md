# cap:sysctl_conf

> 源码吸收自 `/data/dev/bun/src/sys.zig`（Linux `/etc/sysctl.conf` 零分配解析器），提供纯 CP、零外部依赖、零堆分配的内核参数配置流式解析、键名规范化与后定义覆盖查询库。

## 导入方式

```cp
import {
    SysctlEntry,
    SysctlIter,
    normalizeSysctlKey,
    sysctlKeysEqual,
    getSysctlValue,
} from "cap:sysctl_conf";
```

## 核心类型与 API

### `SysctlEntry`

```cp
pub struct SysctlEntry {
    key: []const u8;
    value: []const u8;
    ignore_errors: bool;
}
```

### 迭代器与工具函数

| API | 签名 | 说明 |
| --- | --- | --- |
| `SysctlIter.init` | `fn init(content: []const u8): SysctlIter` | 初始化零分配迭代器（自动跳过 UTF-8 BOM） |
| `SysctlIter.next` | `fn next(*_): SysctlEntry?` | 跳过空行与 `#`/`;` 注释行，按首个 `=` 切分，识别行首 `-` 忽略错误前缀并裁剪首尾空白 |
| `normalizeSysctlKey` | `fn normalizeSysctlKey(key: []const u8, out: []u8): ([]const u8)?` | 将 `/` 规范化为 `.`，校验非法空段或空白，缓冲不足返回 `null` |
| `sysctlKeysEqual` | `fn sysctlKeysEqual(a: []const u8, b: []const u8): bool` | 零分配等价比较，自动将 `/` 与 `.` 视为等价分隔符 |
| `getSysctlValue` | `fn getSysctlValue(content: []const u8, key: []const u8): ([]const u8)?` | 按后定义覆盖前定义语义返回最后一个匹配键的值（跨 `.`/`/` 等价匹配） |

## 使用示例

```cp
import { getSysctlValue, normalizeSysctlKey } from "cap:sysctl_conf";

fn example() {
    const conf = "# kernel\n-net/ipv4/ip_forward = 1\nnet.core.somaxconn = 4096\n";
    * = getSysctlValue(conf, "net.ipv4.ip_forward"); // "1"

    let buf: [64]u8 = undefined;
    * = normalizeSysctlKey("net/ipv4/ip_forward", &buf); // "net.ipv4.ip_forward"
}
```
