# `cap:yaml_scalar` — YAML 1.2 Core Schema 标量解析与安全序列化库

> 来源：吸收自 `/data/dev/bun/src/interchange/yaml.cp`

`cap:yaml_scalar` 提供纯 CP、零外部依赖、零堆分配的 YAML 1.2 Core Schema 标量类型推断、单/双引号解转义及安全标量引号判定与序列化能力。

## 核心类型

### `YamlScalarKind` & `YamlScalar`

```cp
pub enum YamlScalarKind: u8 {
    null_val = 0,
    bool_val = 1,
    int_val = 2,
    float_val = 3,
    string_val = 4,

    fn asStr(): string
}

pub struct YamlScalar {
    kind: YamlScalarKind = .null_val,
    bool_v: bool = false,
    int_v: i64 = 0,
    float_v: f64 = 0.0,
    str_v: []const u8 = "",
}
```

## 核心函数

### `parseCoreScalar`

```cp
pub fn parseCoreScalar(raw: []const u8): YamlScalar
```

严格遵循 YAML 1.2 Core Schema（§10.3.2）：
- **Null**：空串、`~`、`null`、`Null`、`NULL` → `.null_val`
- **Bool**：`true` / `True` / `TRUE` / `false` / `False` / `FALSE` → `.bool_val`（`yes` / `no` / `on` / `off` 保持为 `.string_val`）
- **Int**：十进制 `[-+]?[0-9]+`、八进制 `0o[0-7]+`、十六进制 `0x[0-9a-fA-F]+` → `.int_val`（带 `i64` 溢出保护）
- **Float**：十进制浮点/科学计数法、`.inf` / `+.inf` / `-.inf` / `.Inf` / `.INF`、`.nan` / `.NaN` / `.NAN` → `.float_val`
- **String**：其余均保持 `.string_val`

### `unescapeDoubleQuoted` & `unescapeSingleQuoted`

```cp
pub fn unescapeDoubleQuoted(raw: []const u8, out: []u8): ([]const u8)?
pub fn unescapeSingleQuoted(raw: []const u8, out: []u8): ([]const u8)?
```

- `unescapeDoubleQuoted`：支持 `\n`、`\r`、`\t`、`\\`、`\"`、`\/`、`\0`、`\a`、`\b`、`\v`、`\f`、`\e`、`\ ` 与 `\xHH`；兼容带外层 `"` 或纯内部内容。
- `unescapeSingleQuoted`：将 `''` 解转义为 `'`；兼容带外层 `'` 或纯内部内容。

### `needsQuoting` & `formatSafeScalar`

```cp
pub fn needsQuoting(s: []const u8): bool
pub fn formatSafeScalar(s: []const u8, out: []u8): ([]const u8)?
```

- `needsQuoting`：检测空串、首尾空白、控制字符、YAML 前缀指示符（`-`, `?`, `:`, `[`, `{`, `#`, `&`, `*`, `!`, `|`, `>` 等）、内部 `": "` / `" #"`，以及会被 Core Schema 误解析为非字符串的值。
- `formatSafeScalar`：无需引号时原样输出，否则双引号包裹并转义特殊/控制字符。

## 使用示例

```cp
import { parseCoreScalar, formatSafeScalar, unescapeDoubleQuoted } from "cap:yaml_scalar";

const s = parseCoreScalar("0x2A"); // kind=.int_val, int_v=42
let buf: [128]u8 = [0; 128];
const safe = formatSafeScalar("true", buf[0..]).?; // "\"true\""
```
