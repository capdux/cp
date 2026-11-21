# cap:ini_writer

> 源码吸收自 `/data/dev/bun/src/ini.zig`（INI 配置安全写入与转义器），提供纯 CP、零外部依赖、零堆分配的 INI / gitconfig 配置文件安全序列化与转义写入库。

## 导入方式

```cp
import {
    isValidIniKey,
    needsIniQuotes,
    appendIniSection,
    appendIniSubsection,
    appendIniPair,
    appendIniComment,
} from "cap:ini_writer";
```

## 核心 API

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `isValidIniKey` | `fn isValidIniKey(key: []const u8): bool` | 校验键名非空且不含 `=`、`\r`、`\n`、`[`、`]`、`;`、`#`、`NUL` 与首尾空白 |
| `needsIniQuotes` | `fn needsIniQuotes(val: []const u8): bool` | 判定值是否包含首尾空白或 `;`/`#`/`"`/`\`/`\r`/`\n`/`\t` 需要加引号转义 |
| `appendIniSection` | `fn appendIniSection(out: []u8, pos: *usize, section: []const u8): bool` | 写入 `[section]\n`，非法段名或缓冲不足时原子回滚 `pos` |
| `appendIniSubsection` | `fn appendIniSubsection(out: []u8, pos: *usize, section: []const u8, subsection: []const u8): bool` | 写入兼容 gitconfig 的 `[section "subsection"]\n`（自动转义 `"` 与 `\`），失败原子回滚 `pos` |
| `appendIniPair` | `fn appendIniPair(out: []u8, pos: *usize, key: []const u8, val: []const u8): bool` | 校验键名并写入 `key=val\n`，需要时自动双引号包裹并转义 `\"`/`\\`/`\n`/`\r`/`\t`，失败原子回滚 `pos` |
| `appendIniComment` | `fn appendIniComment(out: []u8, pos: *usize, comment: []const u8): bool` | 将单行或多行注释（支持 LF/CRLF）逐行添加 `# ` 前缀写入，失败原子回滚 `pos` |

## 使用示例

```cp
import {
    appendIniComment,
    appendIniSection,
    appendIniSubsection,
    appendIniPair,
} from "cap:ini_writer";

fn example() {
    let buf: [256]u8 = undefined;
    let pos: usize = 0;

    * = appendIniComment(&buf, *pos, "Generated config\nDo not edit");
    * = appendIniSection(&buf, *pos, "core");
    * = appendIniPair(&buf, *pos, "editor", "vim");
    * = appendIniSubsection(&buf, *pos, "remote", "origin");
    * = appendIniPair(&buf, *pos, "url", "https://example.com/repo.git");
}
```
