# cap:properties_parse

> 源码吸收自 `/data/dev/bun/src/ini.zig`（Java `.properties` 零分配解析器），提供纯 CP、零外部依赖、零堆分配的 Java `.properties` 配置文件流式解析器与键值查询接口。

## 导入方式

```cp
import {
    PropertyEntry,
    PropertiesIter,
    getProperty,
} from "cap:properties_parse";
```

## 核心类型与 API

### `PropertyEntry`

```cp
pub struct PropertyEntry {
    key: []const u8;
    value: []const u8;
    line_no: usize;
}
```

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `key` | `[]const u8` | 反转义后的键名，切片指向调用方提供的 `key_buf` |
| `value` | `[]const u8` | 拼接续行并反转义后的值，切片指向调用方提供的 `val_buf` |
| `line_no` | `usize` | 属性条目起始行号（从 `1` 开始，跨行续行保留起始行号） |

### `PropertiesIter`

| 方法 | 签名 | 说明 |
| --- | --- | --- |
| `init` | `fn init(content: []const u8): PropertiesIter` | 初始化零分配流式迭代器（自动跳过 UTF-8 BOM） |
| `next` | `fn next(*_, key_buf: []u8, val_buf: []u8): PropertyEntry?` | 迭代下一条属性：跳过空行与 `#`/`!` 注释行；支持 `=`、`:` 与空白分隔符；支持 key 内 `\=`/`\:`/`\ ` 转义；支持行尾奇数个 `\` 逻辑续行并剥离续行前导空白；支持 `\n`/`\r`/`\t`/`\f`/`\\`/`\uXXXX`（含 UTF-16 代理对）反转义为 UTF-8 |

### 快捷查询函数

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `getProperty` | `fn getProperty(content: []const u8, target_key: []const u8, val_buf: []u8): ([]const u8)?` | 扫描整段 `.properties` 文本获取 `target_key` 最终值（同名后者覆盖前者），不存在或缓冲不足返回 `null` |

## 使用示例

```cp
import { PropertiesIter, getProperty } from "cap:properties_parse";

fn example() {
    const cfg = "# app config\napp.name = cp-lang\ngreeting: \\u4F60\\u597D\\\n   world\n";
    let kbuf: [64]u8 = undefined;
    let vbuf: [128]u8 = undefined;

    let it = PropertiesIter.init(cfg);
    while (entry := it.next(&kbuf, &vbuf)) {
        * = entry.key;
        * = entry.value;
    }

    if (val := getProperty(cfg, "app.name", &vbuf)) {
        * = val; // "cp-lang"
    }
}
```
