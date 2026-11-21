# `cap:sdf` — MDL Molfile V2000 & SDF 结构数据文件零分配解析库

`cap:sdf` 提供纯 CP、零堆分配的化学信息学 MDL CTfile V2000（`.mol`）与多分子 Structure-Data File（`.sdf`）解析能力，支持固定列宽 Counts Line、Atom Block、Bond Block、`M  END` 后 `> <PROP_NAME>` 多行属性块以及 `$$$$` 多分子记录流。

## 导入方式

```cp
import {
    SdfMolHeader,
    SdfAtom,
    SdfBond,
    SdfProperty,
    SdfStats,
    SdfAtomIter,
    SdfBondIter,
    SdfPropIter,
    SdfRecordIter,
    isSdf,
    parseMolHeader,
    findSdfProp,
    summarizeSdf,
} from "cap:sdf";
```

## 数据结构

### `SdfMolHeader`
3 行头块 + 第 4 行 V2000 Counts Line（零拷贝切片）：
- `title: []const u8` — 第 1 行分子名称
- `program_line: []const u8` — 第 2 行程序/时间戳/维度信息
- `comment: []const u8` — 第 3 行注释
- `atom_count: u32` — 列 `0..3` 原子数
- `bond_count: u32` — 列 `3..6` 化学键数
- `is_chiral: bool` — 列 `12..15` 手性标志（`1` 为 `true`）
- `version: []const u8` — 列 `34..39` 版本号（严格 `"V2000"`）
- `atoms_offset: usize` / `bonds_offset: usize` — Atom Block 与 Bond Block 起始字节偏移

### `SdfAtom`
Atom Block 每行定宽原子记录：
- `x: f64`, `y: f64`, `z: f64` — 列 `0..10`, `10..20`, `20..30` 坐标
- `symbol: []const u8` — 列 `31..34` 元素符号（去空白）
- `mass_diff: i8` — 列 `34..36` 同位素质量差
- `charge_code: u8` — 列 `36..39` V2000 电荷码
- 方法 `formalCharge(): i8` — 将电荷码 `1..7` 映射为 `+3/+2/+1/0/-1/-2/-3`（`0` 为 `0`）

### `SdfBond`
Bond Block 每行定宽化学键记录：
- `atom1: u32`, `atom2: u32` — 列 `0..3`, `3..6`（1-based 原子序号，严格校验 `1 <= idx <= atom_count`）
- `bond_type: u8` — 列 `6..9`（`1`=单键, `2`=双键, `3`=三键, `4`=芳香键）
- `stereo: u8` — 列 `9..12`（`0`=无立体, `1`=楔形向上, `4`=任意, `6`=虚线向下）

### `SdfProperty` & `SdfStats`
- `SdfProperty`：`name: []const u8`, `value: []const u8`（零拷贝支持多行值，自动去首尾空行）
- `SdfStats`：`molecule_count: u32`, `total_atoms: u32`, `total_bonds: u32`, `chiral_count: u32`

## 核心函数与迭代器

- `isSdf(data: []const u8): bool` — 快速判断是否为合法 V2000 Molfile/SDF
- `parseMolHeader(mol_slice: []const u8): SdfMolHeader?` — 解析 3 行头块与 V2000 Counts Line
- `SdfAtomIter.init(mol_slice, header)` / `.next()` — 精确迭代 `atom_count` 行原子（含 `malformed: bool` 截断/畸形标志）
- `SdfBondIter.init(mol_slice, header)` / `.next()` — 精确迭代 `bond_count` 行化学键并校验原子索引不越界（含 `malformed: bool`）
- `SdfPropIter.init(record_slice)` / `.next()` — 迭代 `M  END` 后至 `$$$$` 前的全部 `> <PROP_NAME>` 数据项
- `findSdfProp(record_slice, name): ([]const u8)?` — 按属性名快速零分配查询
- `SdfRecordIter.init(data)` / `.next()` — 按 `$$$$` 分隔符零拷贝迭代多分子记录切片
- `summarizeSdf(data): SdfStats` — 单遍汇总多分子统计信息

## 快速示例

```cp
import { SdfRecordIter, parseMolHeader, SdfAtomIter, findSdfProp } from "cap:sdf";

fn scanLibrary(sdf_bytes: []const u8) {
    let recs = SdfRecordIter.init(sdf_bytes);
    while (rec := recs.next()) {
        if (hdr := parseMolHeader(rec)) {
            let atoms = SdfAtomIter.init(rec, hdr);
            while (atom := atoms.next()) {
                * = atom.formalCharge();
            }
            if (smiles := findSdfProp(rec, "SMILES")) {
                * = smiles;
            }
        }
    }
}
```
