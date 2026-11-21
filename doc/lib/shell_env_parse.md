# cap:shell_env_parse

> 源码吸收自 `/data/dev/bun/src/env_loader.zig`（POSIX Shell 环境变量赋值解析器），提供纯 CP、零外部依赖、零堆分配的 POSIX Shell `export KEY=VAL` / `.profile` 风格环境变量赋值行流式解析器与键值查询接口。

## 导入方式

```cp
import {
    ShellEnvEntry,
    ShellEnvIter,
    getShellEnv,
} from "cap:shell_env_parse";
```

## 核心类型与 API

### `ShellEnvEntry`

```cp
pub struct ShellEnvEntry {
    key: []const u8;
    value: []const u8;
    is_exported: bool;
    line_no: usize;
}
```

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `key` | `[]const u8` | 合法 POSIX 环境变量名（`[A-Za-z_][A-Za-z0-9_]*`），零拷贝指向输入切片 |
| `value` | `[]const u8` | 去引号、反转义并剥离行尾注释后的值，切片指向调用方提供的 `val_buf` |
| `is_exported` | `bool` | 该赋值行是否带有 `export` 前缀 |
| `line_no` | `usize` | 赋值语句起始行号（从 `1` 开始，跨行引号保留起始行号） |

### `ShellEnvIter`

| 方法 | 签名 | 说明 |
| --- | --- | --- |
| `init` | `fn init(content: []const u8): ShellEnvIter` | 初始化零分配流式迭代器（自动跳过 UTF-8 BOM） |
| `next` | `fn next(*_, val_buf: []u8): ShellEnvEntry?` | 迭代下一条有效赋值：跳过空行、`#` 注释行与 `unset` 行；识别可选 `export` 前缀；支持无引号值（剥离空白后 `#` 注释）、单引号字面量 `'...'` 与双引号转义 `"..."`（含 `\n`/`\r`/`\t`/`\"`/`\\`/`\$` 及行续接） |

### 快捷查询函数

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `getShellEnv` | `fn getShellEnv(content: []const u8, key: []const u8, val_buf: []u8): ([]const u8)?` | 扫描整段文本查找 `key` 最终生效值：同名后者覆盖前者；若遇到 `unset KEY`（含多参数 `unset K1 K2` 与 `unset -v KEY`）则清空前值返回 `null`；若缓冲不足返回 `null` |

## 使用示例

```cp
import { ShellEnvIter, getShellEnv } from "cap:shell_env_parse";

fn example() {
    const profile = "export APP_ENV=production # prod\nAPI_MSG=\"hello\\nworld\"\nunset OLD_KEY\n";
    let buf: [128]u8 = undefined;

    let it = ShellEnvIter.init(profile);
    while (entry := it.next(&buf)) {
        * = entry.key;
        * = entry.value;
    }

    if (env_val := getShellEnv(profile, "APP_ENV", &buf)) {
        * = env_val; // "production"
    }
}
```
