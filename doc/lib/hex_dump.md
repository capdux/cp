# cap:hex_dump

> 源码吸收自 `/data/dev/bun/src/fmt.zig`，提供纯 CP、零外部依赖、零堆分配的十六进制编解码与规范化 `hexdump -C` 诊断格式化器。

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
| --- | --- | --- |
| `encodeHexLower` | `fn encodeHexLower(bytes: []const u8, out: []u8): ([]const u8)?` | 小写十六进制编码 |
| `encodeHexUpper` | `fn encodeHexUpper(bytes: []const u8, out: []u8): ([]const u8)?` | 大写十六进制编码 |
| `decodeHexStrict` | `fn decodeHexStrict(hex: []const u8, out: []u8): ([]const u8)?` | 严格解码（拒绝奇数长度与非法字符） |
| `decodeHex` | `fn decodeHex(hex: []const u8, out: []u8): ([]const u8)?` | 宽松解码（忽略空格/冒号/连字符/换行分隔符） |
| `formatHexDumpLine` | `fn formatHexDumpLine(bytes: []const u8, base_offset: usize, out: []u8): ([]const u8)?` | 单行 16 字节 `hexdump -C` 格式 |
| `formatHexDump` | `fn formatHexDump(bytes: []const u8, out: []u8): ([]const u8)?` | 多行完整 `hexdump -C` 格式 |

## 使用示例

```cp
import { encodeHexLower, formatHexDump } from "cap:hex_dump";

fn example() {
    let hex_buf: [32]u8 = undefined;
    * = encodeHexLower("CP", &hex_buf); // "4350"

    let dump_buf: [256]u8 = undefined;
    * = formatHexDump("Hello World!", &dump_buf);
}
```
