# cap:charset_label

> 源码吸收自 `/data/dev/bun/src/bun.js/webcore/encoding.zig`（WHATWG Encoding Standard §4.2），提供纯 CP、零外部依赖、零堆分配的字符集标签规范化与 BOM 嗅探库。

## 导入方式

```cp
import {
    CharsetKind,
    resolveCharsetLabel,
    canonicalCharsetName,
    isUtf8Compatible,
    isAsciiSuperset,
    sniffBomCharset,
} from "cap:charset_label";
```

## 核心类型与 API

- `CharsetKind`：`utf8`、`utf16le`、`utf16be`、`windows1252`、`iso8859_2`、`gbk`、`gb18030`、`big5`、`shift_jis`、`euc_jp`、`euc_kr`、`koi8_r`、`windows1251`、`ibm866`。
- `resolveCharsetLabel(label): CharsetKind?`：去首尾 ASCII 空白、大小写无关解析 WHATWG 主流别名。
- `canonicalCharsetName(kind): []const u8`：返回标准 WHATWG 名称。
- `isUtf8Compatible(label): bool` 与 `isAsciiSuperset(kind): bool`。
- `sniffBomCharset(bytes): CharsetKind?`：嗅探 UTF-8 / UTF-16BE / UTF-16LE BOM。

## 使用示例

```cp
import { resolveCharsetLabel, canonicalCharsetName, sniffBomCharset } from "cap:charset_label";

fn example() {
    if (k := resolveCharsetLabel("  GB2312  ")) {
        * = canonicalCharsetName(k); // "gbk"
    }
    const bom: [3]u8 = [0xEF, 0xBB, 0xBF];
    * = sniffBomCharset(&bom); // CharsetKind.utf8
}
```
