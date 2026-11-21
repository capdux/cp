# `cap:proc_cmdline` — POSIX `/proc/<pid>/cmdline` 与 `environ` 零分配解析库

> 来源：吸收自 `/data/dev/bun/src/sys.zig`（POSIX NUL-Delimited cmdline & environ Parser）

`cap:proc_cmdline` 提供纯 CP、零外部依赖、零堆分配的 POSIX NUL (`\0`) 分隔命令行与环境变量缓冲区解析、查询及可读命令行格式化能力。

## 核心类型与函数

### `NulArgIter`、`countArgs` 与 `argAt`

```cp
pub struct NulArgIter {
    static fn init(buf: []const u8): NulArgIter
    fn next(*_): ([]const u8)?
}

pub fn countArgs(buf: []const u8): usize
pub fn argAt(buf: []const u8, index: usize): ([]const u8)?
```

- 按 `0x00` 切分参数，同时兼容末尾带或不带 `\0`，完整保留中间空参数（如 `"a\x00\x00b\x00"` 产出 `"a"`, `""`, `"b"`）。

### `ProcEnvEntry`、`ProcEnvIter` 与 `findProcEnv`

```cp
pub struct ProcEnvEntry {
    key: []const u8 = "",
    value: []const u8 = "",
}

pub struct ProcEnvIter {
    static fn init(buf: []const u8): ProcEnvIter
    fn next(*_): ProcEnvEntry?
}

pub fn findProcEnv(buf: []const u8, key: []const u8): ([]const u8)?
```

- `ProcEnvIter`：按 `\0` 切分后按首个 `=` 拆为 `key` 与 `value`（保留 `value` 内部的 `=`），自动跳过无 `=` 或空 `key` 的畸形项。
- `findProcEnv`：精确匹配环境变量名，遵循 POSIX `getenv` 语义返回**首次匹配**的值。

### `formatCmdlineDisplay`

```cp
pub fn formatCmdlineDisplay(buf: []const u8, out: []u8): ([]const u8)?
```

- 将 NUL 分隔的 argv 渲染为空格分隔的人类可读命令行；空参数或含空格/引号/Shell 元字符的参数自动采用 POSIX 单引号转义（内部 `'` 转为 `'\''`）；输出缓冲区不足返回 `null`。

## 使用示例

```cp
import { countArgs, argAt, findProcEnv, formatCmdlineDisplay } from "cap:proc_cmdline";

const cmd = "/usr/bin/cap\x00run\x00hello world\x00";
// countArgs(cmd) == 3, argAt(cmd, 2).? == "hello world"
let buf: [128]u8 = [0; 128];
const disp = formatCmdlineDisplay(cmd, buf[0..]).?; // "/usr/bin/cap run 'hello world'"
```
