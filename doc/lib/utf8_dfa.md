# `cap:utf8_dfa` — 无分支 UTF-8 DFA 解码器与 ASCII 码点谓词

> 参考来源：/data/dev/bun/src/unicode/uucode_lib/src/utf8.cp | 吸收日期：2026-11-22

源自 Bun `unicode/uucode_lib/src/{utf8,ascii}.cp`，纯 CP 零依赖实现 Bjoern Hoehrmann 经典双表 UTF-8 有限状态自动机解码器与 `u21` ASCII 无分支判定/大小写转换工具。

## 导出符号

- DFA 核心：`UTF8_ACCEPT`、`UTF8_REJECT`、`REPLACEMENT_CHAR`、`DecodeStep`、`decodeByte`、`isDoneDecoding`
- 流式迭代器与校验：`Utf8Iterator`（`init`/`next`/`peek`/`remaining`/`reset`）、`isValidUtf8`、`countCodepoints`
- ASCII 码点谓词：`isAscii`、`isDigit`、`isUpper`、`isLower`、`isAlphabetic`、`isAlphanumeric`、`isHex`、`isControl`、`isPrint`、`isWhitespace`、`toUpper`、`toLower`

## 上游行为契约与安全校验边界

1. **无分支 O(1) 字节级自动机（Bjoern Hoehrmann DFA）**：
   - `decodeByte(state, codepoint, byte)` 仅需两次查表即可完成单字节转移，天然杜绝过长编码（overlong）、UTF-16 代理区（`U+D800..U+DFFF`）与超 `U+10FFFF` 非法序列；状态归零至 `UTF8_ACCEPT` 表示完整解码一个合法码点，进入 `UTF8_REJECT` 表示非法字节序列。
2. **流式恢复与替代字符**：
   - `Utf8Iterator` 在遇到非法序列时输出 `REPLACEMENT_CHAR`（`U+FFFD`）并安全前进，永不死循环；`isValidUtf8` 与 `countCodepoints` 均单遍零堆分配完成，适合高频协议校验与分词前置检查。
