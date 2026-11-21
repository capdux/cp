# `cap:smi` — 零分配 Daylight SMILES / OpenSMILES 分子线性表示解析标准库

`cap:smi` 是纯 CP、零堆分配的化学信息学 Daylight SMILES / OpenSMILES 解析与验证库，支持有机子集原子（含 `Cl`/`Br` 双字母卤素与芳香小写原子）、括号原子（同位素/手性/显式氢/形式电荷/原子映射类）、单位与 `%NN` 双位环闭合、分支平衡与分子图合法性单遍统计。

---

## 快速导入

```typescript
import {
    SmiTokenKind,
    SmiBracketAtom,
    SmiToken,
    SmiStats,
    SmiLexer,
    parseBracketAtom,
    analyzeSmiles,
    isValidSmiles,
} from "cap:smi";
```

---

## 核心数据结构

### `SmiTokenKind`
词法单元枚举：`atom`、`bracket_atom`、`bond`、`branch_open`、`branch_close`、`ring_closure`、`dot`。

### `SmiBracketAtom`
括号原子 `[isotope? symbol chiral? hcount? charge? (:class)?]` 解析结果：
- `isotope: u16`（`0` 表示未指定）
- `symbol: []const u8`（元素符号零拷贝切片）
- `is_aromatic: bool`
- `chiral_count: u8`（`@`=1，`@@`=2）
- `h_count: u8`（`H`=1，`H2`=2...）
- `charge: i8`（支持 `+`/`-`/`++`/`--`/`+3`/`-2` 等）
- `atom_class: u16`（`:N` 原子映射编号）

### `SmiToken` & `SmiLexer`
- `SmiToken`：`kind: SmiTokenKind`, `text: []const u8`, `ring_num: u8`, `is_aromatic: bool`。
- `SmiLexer.init(smiles)` 构造零分配词法流，调用 `next(*_)` 逐词元扫描；遇非法字符、未闭合 `[` 或非法 `%` 环号置 `had_error = true`。

### `SmiStats`
单遍分子图校验与统计结果：
- `atom_count: u32`、`aromatic_atom_count: u32`、`bond_count: u32`、`ring_count: u32`
- `branch_depth_max: u32`、`fragment_count: u32`、`chiral_centers: u32`、`net_charge: i32`
- `valid: bool`

---

## 核心函数

| 函数签名 | 说明 |
| :--- | :--- |
| `parseBracketAtom(raw: []const u8): SmiBracketAtom?` | 解析括号原子内容（支持内部切片 `"13C@@H"` 或带方括号切片 `"[13C@@H]"`） |
| `analyzeSmiles(smiles: []const u8): SmiStats` | 单遍零分配校验括号平衡、`[100]bool` 环闭合配平、无悬空键与空分支并输出统计 |
| `isValidSmiles(smiles: []const u8): bool` | 快速判定 SMILES 分子串是否合法 |

---

## 使用示例

```typescript
import { analyzeSmiles, parseBracketAtom } from "cap:smi";

fn inspectAspirin(): u32 {
    const stats = analyzeSmiles("CC(=O)Oc1ccccc1C(=O)O");
    return if (stats.valid) stats.atom_count else 0;
}
```
