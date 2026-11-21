# cap:proc_mounts

> 源码吸收自 `/data/dev/bun/src/sys.zig`（Linux `/proc/mounts` 零分配解析器），提供纯 CP、零外部依赖、零堆分配的挂载表流式解析、八进制路径转义还原、选项查询与最长前缀挂载点匹配库。

## 导入方式

```cp
import {
    MountEntry,
    ProcMountsIter,
    unescapeMountPath,
    findMountForPath,
} from "cap:proc_mounts";
```

## 核心类型与 API

### `MountEntry`

```cp
pub struct MountEntry {
    source: []const u8;
    target: []const u8;
    fs_type: []const u8;
    options: []const u8;
    dump_freq: u32;
    pass_num: u32;

    fn hasOption(key: []const u8): bool;
    fn getOptionValue(key: []const u8): ([]const u8)?;
    fn isReadOnly(): bool;
}
```

### 迭代器与工具函数

| API | 签名 | 说明 |
| --- | --- | --- |
| `ProcMountsIter.init` | `fn init(content: []const u8): ProcMountsIter` | 初始化零分配 `/proc/mounts` 迭代器 |
| `ProcMountsIter.next` | `fn next(*_): MountEntry?` | 跳过空行、`#` 注释行与畸形行，解析标准 6 列挂载条目 |
| `unescapeMountPath` | `fn unescapeMountPath(raw: []const u8, out: []u8): ([]const u8)?` | 解码内核八进制转义（`\040` 空格、`\011` Tab、`\012` 换行、`\134` 反斜杠），缓冲不足返回 `null` |
| `findMountForPath` | `fn findMountForPath(content: []const u8, path: []const u8): MountEntry?` | 按最长路径边界前缀匹配对应挂载点（正确区分 `/var` 与 `/var2`，支持叠层后覆盖与含八进制转义的挂载点） |

## 使用示例

```cp
import { findMountForPath, unescapeMountPath } from "cap:proc_mounts";

fn example(mounts_text: []const u8) {
    if (m := findMountForPath(mounts_text, "/var/log/syslog")) {
        * = m.fs_type;
        * = m.isReadOnly();
        * = m.getOptionValue("size");
    }
    let buf: [128]u8 = undefined;
    * = unescapeMountPath("/mnt/my\\040drive", &buf); // "/mnt/my drive"
}
```
