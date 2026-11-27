[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# percent-encoding

> **源包溯源**：Clean-Room MIT 纯 CP 实现，对标 [`crates.io:percent-encoding`](https://github.com/servo/rust-url)（MIT）| CP 包版本：`1.0.0` | 零外部依赖

零堆分配的 WHATWG URL 标准百分号编解码库。内置编译期生成的 128 位 `AsciiSet` 位图、全套标准 URL 编码集、`@memcpy` 连续段块拷贝加速以及宽松/严格/表单 `+` 三种解码模式。

## 零成本迁移对照（Rust → CP）

直接沿用原包名 `"percent-encoding"`，无需改名即可完成迁移：

| Rust (`crates.io:percent-encoding`) | CP (`percent-encoding`) |
| :--- | :--- |
| `use percent_encoding::{utf8_percent_encode, percent_decode_str, NON_ALPHANUMERIC};` | `import { percentEncode, percentDecode, nonAlphanumericSet } from "percent-encoding";` |
| `utf8_percent_encode(s, NON_ALPHANUMERIC)` | `percentEncode(s, nonAlphanumericSet(), out)` |
| `percent_decode_str(s)` | `percentDecode(s, out)` / `percentDecodeStrict(s, out)` |
| `CONTROLS.add(b' ').remove(b'~')` | `let s = controlsSet(); s.add(32); s.remove(126);` |

## 核心 API

### `AsciiSet` 与预置 WHATWG 编码集

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `AsciiSet { bits: [4]u32 }` | 128 位 ASCII 位图（`empty()`、`add(*_, byte)`、`remove(*_, byte)`、`unionWith(other)`、`shouldPercentEncode(byte)`）。 |
| `controlsSet(): AsciiSet` | C0 控制字符（`0x00..=0x1F`）与 `0x7F`（DEL）。 |
| `fragmentSet(): AsciiSet` | `CONTROLS` + 空格、`"`、`<`、`>`、`` ` ``。 |
| `querySet(): AsciiSet` | `CONTROLS` + 空格、`"`、`#`、`<`、`>`。 |
| `pathSet(): AsciiSet` | `FRAGMENT` + `#`、`?`、`{`、`}`。 |
| `userinfoSet(): AsciiSet` | `PATH` + `/`、`:`、`;`、`=`、`@`、`[`、`\`、`]`、`^`、`\|`。 |
| `componentSet(): AsciiSet` | `USERINFO` + `$`、`%`、`&`、`+`、`,`。 |
| `nonAlphanumericSet(): AsciiSet` | 编码除 ASCII `0-9`、`A-Z`、`a-z` 外的所有字节。 |

### 编解码函数

| 签名 | 说明 |
| :--- | :--- |
| `percentEncodeLen(input: &[]u8, set: AsciiSet): usize` | 精确计算编码后字节数，便于预分配目标缓冲。 |
| `percentEncode(input: &[]u8, set: AsciiSet, out: []u8): usize` | 按 `set` 规则将 `input` 编码写入 `out`，返回写入字节数。 |
| `percentDecode(input: string, out: []u8): usize` | 宽松解码（非法 `%XX` 原样透传），返回写入字节数。 |
| `percentDecodeStrict(input: string, out: []u8): i64` | 严格解码；遇非法/截断 `%XX` 或缓冲不足返回 `-1`。 |
| `percentDecodePlus(input: string, out: []u8): usize` | `application/x-www-form-urlencoded` 解码（`+` 转空格，`%XX` 转字节）。 |

## 实战示例

### 基础 URL 编码与解码

```cp
import {
    componentSet,
    percentEncodeLen,
    percentEncode,
    percentDecode,
    percentDecodePlus,
} from "percent-encoding";

fn demoBasic() {
    const raw = "hello world & cp=100%";
    const set = componentSet();
    const need = percentEncodeLen(raw, set);

    let enc: [64]u8 = undefined;
    const n = percentEncode(raw, set, enc[0..]);
    // enc[0..n] == "hello%20world%20%26%20cp%3D100%25"

    let dec: [64]u8 = undefined;
    const m = percentDecode(enc[0..n], dec[0..]);
    // dec[0..m] == raw

    const f = percentDecodePlus("a+b%2Bc", dec[0..]);
    // dec[0..f] == "a b+c"
    * = need; * = m; * = f;
}
```

### 自定义 `AsciiSet` 与严格校验模式

```cp
import { querySet, percentEncode, percentDecodeStrict } from "percent-encoding";

fn demoCustomAndStrict() {
    let custom = querySet();
    custom.add(39);    // 额外编码 '\''
    custom.remove(35); // 保留 '#' 不转义

    let out: [64]u8 = undefined;
    const n = percentEncode("it's #1", custom, out[0..]);
    // out[0..n] == "it%27s%20#1"

    // 严格模式拦截非法转义序列，返回 -1
    const bad = percentDecodeStrict("abc%2G", out[0..]);
    // bad == -1
    * = n; * = bad;
}
```

## 性能与安全亮点

- **零堆分配**：全部编解码直写调用方提供的 `out: []u8` 缓冲区。
- **编译期查找表与位图**：`PE_HEX_LUT: [256]u8` 与 7 套 WHATWG `AsciiSet` 均在编译期生成，运行时 O(1) 查表零初始化成本。
- **`@memcpy` 连续段块拷贝**：扫描无需转义的连续字节段并一次性 `@memcpy`，大幅提升纯 ASCII URL 吞吐。
- **严格越界与格式校验**：`percentDecodeStrict` 对截断 `%`、非法十六进制字符与缓冲区不足统一返回 `-1`。

## 许可协议

MIT
