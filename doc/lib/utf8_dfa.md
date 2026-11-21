# `cap:utf8_dfa` — 无分支 UTF-8 DFA 解码器与 ASCII 码点谓词

源自 Bun `unicode/uucode_lib/src/{utf8,ascii}.cp`，纯 CP 零依赖实现 Bjoern Hoehrmann 经典双表 UTF-8 有限状态自动机解码器与 `u21` ASCII 无分支判定/大小写转换工具。

## 导出符号

- DFA 核心：`UTF8_ACCEPT`、`UTF8_REJECT`、`REPLACEMENT_CHAR`、`DecodeStep`、`decodeByte`、`isDoneDecoding`
- 流式迭代器与校验：`Utf8Iterator`（`init`/`next`/`peek`/`remaining`/`reset`）、`isValidUtf8`、`countCodepoints`
- ASCII 码点谓词：`isAscii`、`isDigit`、`isUpper`、`isLower`、`isAlphabetic`、`isAlphanumeric`、`isHex`、`isControl`、`isPrint`、`isWhitespace`、`toUpper`、`toLower`
