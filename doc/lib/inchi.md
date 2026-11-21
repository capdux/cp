# `cap:inchi` — IUPAC InChI v1.06 分层结构与 27 字符 InChIKey 零分配解析校验库

`cap:inchi` 提供纯 CP、零堆分配的 IUPAC International Chemical Identifier（`InChI=1S/...` / `InChI=1/...`）分层拆解、Hill 记法分子式元素计数迭代以及 27 字符 `InChIKey` 严格格式校验与解码能力。

## 导入方式

```cp
import {
    INCHI_PREFIX,
    INCHI_KEY_LEN,
    InchiLayerKind,
    InchiLayer,
    InchiParsed,
    FormulaElement,
    InchiKey,
    InchiLayerIter,
    FormulaIter,
    isInchi,
    parseInchi,
    parseInchiKey,
    isValidInchiKey,
} from "cap:inchi";
```

## 常量与枚举

- `INCHI_PREFIX = "InChI="`
- `INCHI_KEY_LEN = 27`
- `InchiLayerKind`：`formula`、`connections` (`/c`)、`hydrogens` (`/h`)、`charge` (`/q`)、`protons` (`/p`)、`double_bond_stereo` (`/b`)、`tetrahedral_stereo` (`/t`)、`stereo_parity` (`/m`)、`stereo_type` (`/s`)、`isotopic` (`/i`)、`fixed_h` (`/f`)、`reconnected` (`/r`)、`unknown`

## 数据结构

### `InchiParsed`
零拷贝提取全部核心分层切片：
- `version: []const u8`（如 `"1S"` / `"1"`）、`is_standard: bool`
- `formula: []const u8`（分子式主层）
- `connections`, `hydrogens`, `charge`, `protons`, `double_bond_stereo`, `tetrahedral_stereo`, `stereo_parity`, `stereo_type`, `isotopic`, `fixed_h`, `reconnected: []const u8`
- `layer_count: u32`

### `InchiLayer` & `InchiLayerIter`
- `InchiLayer`：`prefix: u8`（主分子式层为 `0`）、`kind: InchiLayerKind`、`content: []const u8`
- `InchiLayerIter.init(s)` / `.next()` — 逐层零分配遍历全部 `/...` 子层

### `FormulaElement` & `FormulaIter`
- `FormulaElement`：`symbol: []const u8`, `count: u32`
- `FormulaIter.init(formula)` / `.next()` — 解析 Hill 记法单组分（`"C9H8O4"`、`"NaCl"`）与多组分分子式（`"2H2O.NaCl"`，自动乘入组分前导系数）

### `InchiKey`
严格拆解 27 字符 `AAAAAAAAAAAAAA-BBBBBBBBFV-P`：
- `connectivity_block: []const u8`（14 位大写字母）
- `stereo_block: []const u8`（8 位大写字母）
- `flag_char: u8`（`'S'`=Standard / `'N'`=Non-standard）、`is_standard: bool`
- `version_char: u8`（v1 为 `'A'`）
- `protonation_char: u8`（`'A'..'Z'`，`'N'`=中性、`'O'`=+1、`'M'`=-1）
- 方法 `protonationDelta(): i8` — 返回 `protonation_char - 'N'`

## 核心函数

- `isInchi(s: []const u8): bool`
- `parseInchi(s: []const u8): InchiParsed?`
- `parseInchiKey(key: []const u8): InchiKey?`
- `isValidInchiKey(key: []const u8): bool`

## 快速示例

```cp
import { parseInchi, FormulaIter, parseInchiKey } from "cap:inchi";

fn inspectChemical(inchi: []const u8, key: []const u8) {
    if (p := parseInchi(inchi)) {
        let fit = FormulaIter.init(p.formula);
        while (elem := fit.next()) {
            * = elem.count;
        }
    }
    if (k := parseInchiKey(key)) {
        * = k.protonationDelta();
    }
}
```
