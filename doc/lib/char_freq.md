# `cap:char_freq` — 标识符字符频率统计与压缩短名生成器

> 参考来源：/data/dev/bun/src/js_parser/ast/CharFreq.cp | 吸收日期：2026-11-22

## 概述

`cap:char_freq` 吸收自 Bun JS/TS 压缩器的标识符频率统计模块（`src/js_parser/ast/CharFreq.cp`），纯 CP 零堆分配实现 64 桶 ASCII 标识符字符频率统计（`a-z`、`A-Z`、`0-9`、`_`、`$`）、跨文件批量合并以及按频次稳定排序的 Base-54/64 短标识符生成器（`NameMinifier`），用于代码压缩器提升最终产物的 gzip / brotli 压缩率。

## 上游算法原理与行为契约

- **压缩率优化原理**：gzip / brotli 的 Huffman 编码会为输入中高频出现的字符分配更短的比特码。在代码压缩器对局部变量重命名之前，先扫描整个源码/AST 统计 64 个合法 JS 标识符字符的出现频次，再将高频字符优先分配给最短的 1–2 字符变量名，可显著提升压缩率且零运行期开销。
- **64 桶映射与首尾字符集分离**：
  - 全部 64 个尾字符（`TAIL_CHAR_COUNT = 64`）：`a-z`（26）+ `A-Z`（26）+ `0-9`（10）+ `_` + `$`；
  - 54 个合法首字符（`HEAD_CHAR_COUNT = 54`）：不含数字 `0-9`（因标识符不能以数字开头）；
  - `NameMinifier.numberToName` 使用 Base-54（首字符）+ Base-64（后续字符）将序号 $0, 1, 2, \dots$ 确定性映射为最短合法标识符。
- **增量扫描与并行合并**：
  - `scan(text, delta)` 支持任意正负 `delta: i32`（`delta == 0` 直接短路跳过），既可用于正向累计源码字符，也可在剔除未引用符号时传入负 `delta` 扣除频次；
  - `include(other)` 将多个并行解析线程产出的 `CharFreq` 按桶累加合并，天然适配多线程打包器；
  - `compile()` 采用确定性排序规则：**频次降序 → 原始索引升序 → ASCII 码升序**，保证相同输入在任意平台产生完全一致的标识符序列。

## 导出符号与 API

### 常量

- `CHAR_FREQ_COUNT: usize = 64`
- `HEAD_CHAR_COUNT: usize = 54`
- `TAIL_CHAR_COUNT: usize = 64`

### 结构体与方法

- `CharAndCount`：`{ char: u8, count: i32, index: usize }`
- `NameMinifier`：
  - `static fn default(): NameMinifier` — 返回默认字母表顺序的压缩名生成器；
  - `fn numberToName(index: usize, buf: []u8): []const u8` — 将非负序号编码为最短合法标识符写入 `buf`。
- `CharFreq`：
  - `static fn init(): CharFreq` — 初始化全零 64 桶计数器；
  - `fn scan(*_, text: []const u8, delta: i32)` — 扫描文本并按 `delta` 更新 64 桶计数，忽略非标识符字符；
  - `fn include(*_, other: CharFreq)` — 累加合并另一份频率表；
  - `fn countOf(ch: u8): i32` — 查询单个字符的当前频次（非标识符字符返回 `0`）；
  - `fn compile(): NameMinifier` — 按频次降序编译生成最优 `NameMinifier`。

## 边界注意事项

1. **非标识符字符忽略**：空白、标点、UTF-8 多字节字符在 `scan` 中自动跳过，不影响计数。
2. **输出缓冲长度**：`numberToName` 所需最大字节数极小（64 位序号最多约 11 字节），建议传入至少 16 字节栈缓冲。

## 使用示例

```cp
import { CharFreq, NameMinifier } from "cap:char_freq";

let freq = CharFreq.init();
freq.scan("function compute(value) { return value * value; }", 1);
const minifier = freq.compile();

let buf: [16]u8 = undefined;
const first_name = minifier.numberToName(0, buf[0..]); // 最高频合法首字符
* = first_name;
* = NameMinifier.default();
```
