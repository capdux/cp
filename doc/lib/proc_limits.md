# cap:proc_limits

> 源码吸收自 `/data/dev/bun/src/sys.zig`（Linux `/proc/<pid>/limits` 零分配解析器），提供纯 CP、零外部依赖、零堆分配的进程软/硬资源限额表解析与快捷查询库。

## 导入方式

```cp
import {
    LimitValue,
    ProcLimitEntry,
    ProcLimitsIter,
    parseLimitValue,
    findProcLimit,
    maxOpenFiles,
} from "cap:proc_limits";
```

## 核心类型与 API

### `LimitValue` 与 `parseLimitValue`

```cp
pub struct LimitValue {
    is_unlimited: bool;
    value: u64;
}
```

- `fn parseLimitValue(s: []const u8): LimitValue?`：解析 `"unlimited"` 或无符号十进制 `u64`，非法值或溢出返回 `null`。

### `ProcLimitEntry`

```cp
pub struct ProcLimitEntry {
    name: []const u8;
    soft: LimitValue;
    hard: LimitValue;
    units: []const u8;
}
```

### 迭代器与查询函数

| API | 签名 | 说明 |
| --- | --- | --- |
| `ProcLimitsIter.init` | `fn init(content: []const u8): ProcLimitsIter` | 初始化零分配 `/proc/<pid>/limits` 迭代器 |
| `ProcLimitsIter.next` | `fn next(*_): ProcLimitEntry?` | 跳过表头与空行，按 `>= 2` 空格或 Tab 切分含单空格的名称与三列值，兼容无 Units 列的行 |
| `findProcLimit` | `fn findProcLimit(content: []const u8, name: []const u8): ProcLimitEntry?` | 大小写不敏感查找指定资源限额条目（如 `"Max open files"` / `"Max stack size"`） |
| `maxOpenFiles` | `fn maxOpenFiles(content: []const u8): LimitValue?` | 快捷提取 `"Max open files"` 的软上限 |

## 使用示例

```cp
import { findProcLimit, maxOpenFiles } from "cap:proc_limits";

fn example(limits_text: []const u8) {
    if (nofile := maxOpenFiles(limits_text)) {
        * = nofile.value; // 1048576
    }
    if (stack := findProcLimit(limits_text, "max stack size")) {
        * = stack.soft.value;
        * = stack.hard.is_unlimited;
    }
}
```
