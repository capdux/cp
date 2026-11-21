# `cap:char_freq` — 标识符字符频率统计与压缩短名生成器

源自 Bun `js_parser/ast/CharFreq.cp`，纯 CP 零堆分配实现 64 桶 ASCII 标识符字符频率统计（`a-z`/`A-Z`/`0-9`/`_`/`$`）、批量合并与按频次排序的 Base-54/64 短标识符生成器（`NameMinifier`），用于代码压缩器提升 gzip/brotli 压缩率。

## 导出符号

- 常量：`CHAR_FREQ_COUNT`(64)、`HEAD_CHAR_COUNT`(54)、`TAIL_CHAR_COUNT`(64)
- `CharAndCount`、`NameMinifier`（`default`/`numberToName`）
- `CharFreq`（`init`/`scan`/`include`/`countOf`/`compile`）
