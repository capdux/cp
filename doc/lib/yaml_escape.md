# cap:yaml_escape

> 源码吸收自 `/data/dev/bun/src/yaml/yaml.zig`（YAML 1.2 规范），提供纯 CP、零外部依赖、零堆分配的 YAML 标量安全判定、单/双引号转义与反转义库。

## 导入方式

```cp
import {
    needsYamlQuotes,
    quoteYamlSingle,
    quoteYamlDouble,
    formatYamlScalar,
    unescapeYamlDouble,
} from "cap:yaml_escape";
```

## 核心 API

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `needsYamlQuotes` | `fn needsYamlQuotes(s: []const u8): bool` | 判定空串、保留字、数字外观、指示符与控制字符是否需引号 |
| `quoteYamlSingle` | `fn quoteYamlSingle(s: []const u8, out: []u8): ([]const u8)?` | 单引号转义（`'`→`''`），含控制字符返回 `null` |
| `quoteYamlDouble` | `fn quoteYamlDouble(s: []const u8, out: []u8): ([]const u8)?` | 双引号转义（`\"`/`\\`/`\0`/`\a`/`\b`/`\t`/`\n`/`\v`/`\f`/`\r`/`\e`/`\xHH`） |
| `formatYamlScalar` | `fn formatYamlScalar(s: []const u8, out: []u8): ([]const u8)?` | 无需引号原样输出，否则双引号安全转义 |
| `unescapeYamlDouble` | `fn unescapeYamlDouble(escaped: []const u8, out: []u8): ([]const u8)?` | 还原 YAML 1.2 双引号转义含 `\xHH`/`\uXXXX`/`\UXXXXXXXX` |

## 使用示例

```cp
import { formatYamlScalar, unescapeYamlDouble } from "cap:yaml_escape";

fn example() {
    let buf: [128]u8 = undefined;
    * = formatYamlScalar("hello-world", &buf); // "hello-world"
    * = formatYamlScalar("true", &buf);        // "\"true\""
    * = unescapeYamlDouble("\"\\u4E2D\\n\"", &buf); // "中\n"
}
```
