# cap:shell_quote

> 源码吸收自 `/data/dev/bun/src/shell/shell.zig`（POSIX Shell 引号转义与命令行分词器），提供纯 CP、零外部依赖、零堆分配的 POSIX Shell 安全转义与参数拆分库。

## 导入方式

```cp
import { needsQuoting, quotePosix, joinArgs, ShellArgIter } from "cap:shell_quote";
```

## 核心 API

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `needsQuoting` | `fn needsQuoting(s: []const u8): bool` | 空串或含非 `[A-Za-z0-9_@%+=:,./-]` 字符返回 `true` |
| `quotePosix` | `fn quotePosix(s: []const u8, out: []u8): ([]const u8)?` | POSIX 单引号安全转义（空串 `''`，内部 `'` 转 `'\''`） |
| `joinArgs` | `fn joinArgs(args: []const []const u8, out: []u8): ([]const u8)?` | 安全转义多个参数并以空格拼接 |
| `ShellArgIter` | `init(cmdline)` / `next(*_, out): ([]const u8)?` / `hasError(): bool` | 支持单引号、双引号内 `\"`/`\\`/`\$`/`\`` 转义、非引号反斜杠与相邻拼接，未闭合引号置位错误并返回 `null` |

## 使用示例

```cp
import { joinArgs, ShellArgIter } from "cap:shell_quote";

fn example() {
    let cmd_buf: [128]u8 = undefined;
    const args: [3][]const u8 = ["echo", "it's ok", "a b"];
    if (cmd := joinArgs(&args, &cmd_buf)) {
        let it = ShellArgIter.init(cmd);
        let tok: [64]u8 = undefined;
        while (it.next(&tok) != null) {}
    }
}
```
