# cap:fstab_parse

> 源码吸收自 `/data/dev/bun/src/sys.zig`（POSIX `/etc/fstab` 与 `/proc/mounts` 规范），提供纯 CP、零外部依赖、零堆分配的 6 字段挂载表解析器与挂载选项查询库。

## 导入方式

```cp
import { MountEntry, MountIter, unescapeMountPath, findMountForPath } from "cap:fstab_parse";
```

## 核心类型与 API

### `MountEntry`

字段：`device`、`mount_point`、`fs_type`、`options_csv`、`dump_freq: u32`、`pass_no: u32`。

- `MountEntry.parseLine(raw): MountEntry?`：解析 4~6 空白分隔字段（跳过空行与 `#` 注释，省略 `dump_freq`/`pass_no` 默认为 0）。
- `hasOption(opt): bool`：按逗号匹配选项名（兼容 `key=val`）。
- `getOptionValue(key): ([]const u8)?`：提取 `key=val` 选项的值。
- `isReadOnly(): bool`：判定只读挂载（若同时出现 `ro`/`rw` 以后出现为准）。

### `MountIter` / `unescapeMountPath` / `findMountForPath`

- `MountIter.init(content)` / `next(*_): MountEntry?`：流式迭代合法挂载条目。
- `unescapeMountPath(raw, out): ([]const u8)?`：还原 `/proc/mounts` 八进制转义（`\040`、`\011`、`\012`、`\134` 等）。
- `findMountForPath(content, abs_path): MountEntry?`：按最长路径前缀匹配目标挂载点。

## 使用示例

```cp
import { findMountForPath, unescapeMountPath } from "cap:fstab_parse";

fn example() {
    const mounts = "/dev/sda1 / ext4 rw,relatime 0 1\n/dev/sdb1 /data xfs ro,noexec 0 2\n";
    if (m := findMountForPath(mounts, "/data/dev/cp")) {
        * = m.isReadOnly(); // true
    }
    let buf: [64]u8 = undefined;
    * = unescapeMountPath("/mnt/my\\040drive", &buf); // "/mnt/my drive"
}
```
