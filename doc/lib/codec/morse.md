# `cap:morse` — ITU-R M.1677-1 国际摩尔斯电码与时序脉冲库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:codec`，导入请使用 `import { ... } from "cap:codec"`。

零堆分配（Zero-Alloc）的纯 CP 国际摩尔斯电码（ITU-R M.1677-1）编解码与 CW（Continuous Wave）键控脉冲时序生成库，适用于无线电 CW 键控、蜂鸣器/LED 信号调制、嵌入式告警与文本编解码场景。

## 核心特性

- **零堆分配**：全部文本编解码与脉冲序列生成均直接写入调用方缓冲区。
- **全量字符集**：支持 26 个英文字母（编码时大小写无关、解码返回大写 ASCII）、10 个数字 `0..9` 以及全部 18 个 ITU/国际标点 `. , ? ' ! / ( ) & : ; = + - _ " $ @`。
- **O(1) 二进制堆逆查**：`morseToChar` 通过二进制树位编码（`.` -> `0`，`-` -> `1`）进行单遍查表，严格拒绝非法符号、超长码字及未定义位模式。
- **标准 PARIS 时序模型**：内置 ITU-R M.1677-1 拍点常量（1/3/1/3/7）与 `wpmToDitMs`、`morseToPulses`、`morseDurationUnits`。

## 导出 API

### 协议常量

| 常量 | 类型 | 值 | 说明 |
| --- | --- | --- | --- |
| `DIT_UNITS` | `u8` | `1` | 点（dit）发信时长（1 单位） |
| `DAH_UNITS` | `u8` | `3` | 划（dah）发信时长（3 单位） |
| `INTRA_CHAR_GAP` | `u8` | `1` | 字符内部符号间静默（1 单位） |
| `INTER_CHAR_GAP` | `u8` | `3` | 同一单词字符间静默（3 单位） |
| `INTER_WORD_GAP` | `u8` | `7` | 单词间静默（7 单位） |
| `PARIS_WORD_UNITS` | `usize` | `50` | 标准 `"PARIS "` 字长（43 活动/字符间隔 + 7 词尾间隔） |
| `MAX_MORSE_CODE_LEN` | `usize` | `7` | 支持的最大单字符码字长度（`$` 为 7 符号 `...-..-`） |

### 类型定义

```cp
pub struct MorsePulse {
    mark: bool, // true = 发信/键按下/灯亮, false = 静默/键抬起/灯灭
    units: u8,  // 拍点单位数（1 / 3 / 7）
}
```

### 核心函数

- `wpmToDitMs(wpm: u32): u32?`
  按 PARIS 标准（50 dit 单位/词）将速度（WPM）换算为单 dit 毫秒数（`1200 / wpm`）；`wpm == 0` 返回 `null`。
- `charToMorse(ch: u8): ([]const u8)?`
  字符级正向查表（大小写无关），不支持字符返回 `null`。
- `morseToChar(code: []const u8): u8?`
  码字级逆向查表，返回大写 ASCII 字母/数字/标点；空串、长度 `> 7`、含非 `.`/`-` 字符或未定义码字返回 `null`。
- `morseEncode(text: []const u8, dst: []u8): usize?`
  将 ASCII 文本零分配编码为摩尔斯电码：字母间以单空格 `' '` 分隔，单词间以 `" / "` 分隔，自动折叠连续及首尾空白；遇不支持字符或 `dst` 容量不足返回 `null`。
- `morseDecode(morse: []const u8, dst: []u8): usize?`
  将摩尔斯电码零分配解码为大写 ASCII：支持 `" / "` 或 3+ 连续空格分隔单词、1~2 空格分隔字符；严格校验每个码字有效性与 `dst` 容量。
- `morseToPulses(text: []const u8, out: []MorsePulse): usize?`
  将文本直接展开为标准 ITU-R M.1677-1 `MorsePulse` 时序序列，返回写入条数；遇不支持字符或 `out` 溢出返回 `null`。
- `morseDurationUnits(text: []const u8): usize?`
  计算文本的精确拍点总时长（不含词尾间隔，如 `"PARIS"` 返回 `43`，加 `INTER_WORD_GAP` 为 `50`）；遇不支持字符返回 `null`。

## 使用示例

```cp
import std from "std";
import {
    INTER_WORD_GAP,
    MorsePulse,
    wpmToDitMs,
    morseEncode,
    morseDecode,
    morseToPulses,
    morseDurationUnits,
} from "cap:codec";

fn example() {
    let enc_buf: [128]u8 = std.mem.zeroes([128]u8);
    let dec_buf: [64]u8 = std.mem.zeroes([64]u8);

    if (n := morseEncode("SOS", enc_buf[0..])) {
        // enc_buf[0..n] == "... --- ..."
        * = morseDecode(enc_buf[0..n], dec_buf[0..]);
    }

    if (dit_ms := wpmToDitMs(20)) {
        // dit_ms == 60 ms
        let pulses: [64]MorsePulse = std.mem.zeroes([64]MorsePulse);
        if (cnt := morseToPulses("PARIS", pulses[0..])) {
            // cnt 脉冲总拍点 == morseDurationUnits("PARIS") == 43
            // 加上词尾 INTER_WORD_GAP (7) == 50 单位
            * = dit_ms;
            * = cnt;
            * = INTER_WORD_GAP;
        }
    }
}
```
