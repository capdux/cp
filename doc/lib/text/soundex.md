# `cap:soundex` — 语音索引与姓名模糊匹配零分配套件

> [!NOTE]
> 本子模块已归并至统一主包 `cap:text`，导入请使用 `import { ... } from "cap:text"`。

零堆分配（Zero-Alloc）的纯 CP 经典语音编码算法库，集成四大工业标准语音索引算法：American Soundex (NARA/Knuth)、Metaphone (1990)、NYSIIS 与 Cologne Phonetics (Kölner Phonetik)，适用于姓名去重、搜索引擎模糊召回、数据库索引键与记录链接（Record Linkage）。

## 核心特性

- **零堆分配**：全程仅使用栈上固定缓冲与调用方输出切片，无任何动态内存分配。
- **American Soundex (NARA 官方标准)**：精确实现 `H`/`W` 透明邻接去重与元音分隔重置，通过全部 NARA 经典边界向量（`Ashcraft -> A261`、`Tymczak -> T522`、`Pfister -> P236`、`Honeyman -> H555`）。
- **Metaphone (Lawrence Philips 1990)**：完整覆盖开头静音对（`KN`/`GN`/`PN`/`AE`/`WR`）、尾静音 `MB`、`CH`/`SH`/`TH`（`'0'`）/`PH`/`GH`/`CK`/`DG`/`TCH`/`WH`/`X` 等标准英语发音规则。
- **NYSIIS**：完整实现前缀（`MAC`/`KN`/`K`/`PH`/`PF`/`SCH`）、后缀（`EE`/`IE`/`DT`/`RT`/`RD`/`NT`/`ND`）、主体转换、相邻去重与尾部清理。
- **Cologne Phonetics (Kölner Phonetik)**：严格遵循 Postel (1969) 德语/欧洲姓名语音编码规则，支持相邻去重与非首位 `'0'` 剥离。

## 导出 API

### 常量

- `SOUNDEX_LEN: usize = 4`
- `MAX_PHONETIC_WORD_LEN: usize = 256`

### 函数

- `soundex(word: []const u8): ([4]u8)?`：返回 4 字节定长 American Soundex 编码；无 ASCII 字母时返回 `null`。
- `soundexEquals(a: []const u8, b: []const u8): bool`：判断两词 Soundex 编码是否均有效且相等。
- `soundexDifference(a: []const u8, b: []const u8): u8`：SQL Server 风格相似度分值（`0..4`）。
- `metaphone(word: []const u8, out: []u8): usize?`：零分配计算 Metaphone 大写语音码。
- `nysiis(word: []const u8, out: []u8): usize?`：零分配计算 NYSIIS 大写语音码。
- `colognePhonetic(word: []const u8, out: []u8): usize?`：零分配计算 Kölner Phonetik 数字语音码。

## 使用示例

```cp
import std from "std";
import {
    soundex,
    soundexEquals,
    soundexDifference,
    metaphone,
    nysiis,
    colognePhonetic,
} from "cap:text";

fn example() {
    * = soundexEquals("Robert", "Rupert");     // true ("R163")
    * = soundexDifference("Robert", "Rubin");  // 2
    if (s := soundex("Ashcraft")) {
        // s == "A261"
        * = s;
    }

    let buf: [64]u8 = std.mem.zeroes([64]u8);
    * = metaphone("Knight", buf[0..]);         // "NT"
    * = nysiis("MacIntosh", buf[0..]);         // "MCANTA"
    * = colognePhonetic("Wikipedia", buf[0..]); // "3412"
}
```
