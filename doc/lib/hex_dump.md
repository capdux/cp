# `cap:hex_dump` — 零堆分配十六进制编解码与 `hexdump -C` 诊断格式化器

> 参考来源：/data/dev/bun/src/bun_core/fmt.cp | 吸收日期：2026-11-22

## 概述

`cap:hex_dump` 吸收自 Bun 核心格式化模块（`src/bun_core/fmt.cp`），提供纯 CP、零外部依赖、零堆分配的十六进制编解码（大小写编码、严格/宽松解码）与标准 `hexdump -C` 风格的 16 字节对齐十六进制+ASCII 对照诊断输出，适用于协议抓包、加密摘要展示与二进制解析调试。

## 行为契约与安全边界

- **严格解码 vs 宽松解码**：
  - `decodeHexStrict`：仅接受偶数长度的纯 `[0-9a-fA-F]` 字符；任何奇数长度、空白或非法字符均立即返回 `null`，适合加密密钥、哈希摘要与协议字段校验；
  - `decodeHex`：自动忽略空格、冒号 `:`、连字符 `-`、制表符与换行分隔符（如 MAC 地址 `AA:BB:CC:DD:EE:FF` 或 UUID 片段），若有效十六进制字符数为奇数或含其他非法字符仍返回 `null`。
- **Canonical `hexdump -C` 布局**：每行固定展示 16 字节（分前 8 字节 + 空格 + 后 8 字节），左侧 8 位十六进制偏移量，右侧 `|...|` 显示可见 ASCII（`0x20..0x7E`），不可见控制字节与非 ASCII 字节统一替换为 `.`，杜绝终端控制序列注入。
- **缓冲容量精确校验**：所有编码、解码与格式化函数在写入前检查 `out.len`（如编码需 `>= 2 * bytes.len`），不足时原子返回 `null` 而不截断或越界。

## 导入方式

```cp
import {
    encodeHexLower,
    encodeHexUpper,
    decodeHexStrict,
    decodeHex,
    formatHexDumpLine,
    formatHexDump,
} from "cap:hex_dump";
```

## 核心 API

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `encodeHexLower` | `fn encodeHexLower(bytes: []const u8, out: []u8): ([]const u8)?` | 小写十六进制编码 |
| `encodeHexUpper` | `fn encodeHexUpper(bytes: []const u8, out: []u8): ([]const u8)?` | 大写十六进制编码 |
| `decodeHexStrict` | `fn decodeHexStrict(hex: []const u8, out: []u8): ([]const u8)?` | 严格解码（拒绝奇数长度与非法字符） |
| `decodeHex` | `fn decodeHex(hex: []const u8, out: []u8): ([]const u8)?` | 宽松解码（忽略空格/冒号/连字符/换行分隔符） |
| `formatHexDumpLine` | `fn formatHexDumpLine(bytes: []const u8, base_offset: usize, out: []u8): ([]const u8)?` | 单行最多 16 字节 `hexdump -C` 格式 |
| `formatHexDump` | `fn formatHexDump(bytes: []const u8, out: []u8): ([]const u8)?` | 多行完整 `hexdump -C` 格式 |

## 使用示例

```cp
import { encodeHexLower, decodeHex, formatHexDump } from "cap:hex_dump";

fn example() {
    let hex_buf: [32]u8 = undefined;
    * = encodeHexLower("CP", &hex_buf); // "4350"

    let bin_buf: [16]u8 = undefined;
    * = decodeHex("43:50-21", &bin_buf); // "CP!"

    let dump_buf: [256]u8 = undefined;
    * = formatHexDump("Hello World!", &dump_buf);
}
```
