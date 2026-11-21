# cap:xml_escape

> 源码吸收自 `/data/dev/bun/src/string/immutable.zig`（XML 1.0 Fifth Edition 规范），提供纯 CP、零外部依赖、零堆分配的 XML 文本/属性转义、CDATA 安全切分与实体反转义库。

## 导入方式

```cp
import {
    isValidXmlChar,
    escapeXmlText,
    escapeXmlAttr,
    wrapXmlCdata,
    unescapeXml,
} from "cap:xml_escape";
```

## 核心 API

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `isValidXmlChar` | `fn isValidXmlChar(cp: u32): bool` | XML 1.0 §2.2 合法字符码点校验（拒绝 NUL/控制符/代理项/FFFE/FFFF） |
| `escapeXmlText` | `fn escapeXmlText(src: []const u8, out: []u8): ([]const u8)?` | 转义 `&`/`<`/`>`（含 `]]>` 中的 `>`） |
| `escapeXmlAttr` | `fn escapeXmlAttr(src: []const u8, out: []u8): ([]const u8)?` | 转义 `&`/`<`/`>`/`"`/`'` 与 `\r`/`\n`/`\t` |
| `wrapXmlCdata` | `fn wrapXmlCdata(src: []const u8, out: []u8): ([]const u8)?` | 包裹 `<![CDATA[...]]>` 并安全拆分内部 `]]>` |
| `unescapeXml` | `fn unescapeXml(src: []const u8, out: []u8): ([]const u8)?` | 还原 5 个预定义实体及十进制/十六进制字符引用并校验 `isValidXmlChar` |

## 使用示例

```cp
import { escapeXmlAttr, wrapXmlCdata, unescapeXml } from "cap:xml_escape";

fn example() {
    let buf: [128]u8 = undefined;
    * = escapeXmlAttr("a & \"b\"\n", &buf);  // "a &amp; &quot;b&quot;&#xA;"
    * = wrapXmlCdata("x]]>y", &buf);         // "<![CDATA[x]]]]><![CDATA[>y]]>"
    * = unescapeXml("&lt;&#x41;&#66;&gt;", &buf); // "<AB>"
}
```
