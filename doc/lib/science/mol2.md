# `cap:mol2` — Tripos SYBYL `.mol2` 3D 分子结构与力场原子类型零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:science`，导入请使用 `import { ... } from "cap:science"`。

`cap:mol2` 提供纯 CP、零堆分配的 Tripos SYBYL `.mol2` 分子格式解析能力，支持 `@<TRIPOS>MOLECULE`、`@<TRIPOS>ATOM`、`@<TRIPOS>BOND`、`@<TRIPOS>SUBSTRUCTURE` 等区段定位、力场原子类型（`C.ar`、`O.2`、`N.am`）、部分电荷、芳香/酰胺化学键以及多分子拼接流解析。

## 导入方式

```cp
import {
    MOL2_RTI_MOLECULE,
    MOL2_RTI_ATOM,
    MOL2_RTI_BOND,
    MOL2_RTI_SUBSTRUCTURE,
    Mol2Header,
    Mol2Atom,
    Mol2Bond,
    Mol2Stats,
    Mol2AtomIter,
    Mol2BondIter,
    Mol2RecordIter,
    isMol2,
    parseMol2Header,
    parseMol2AtomLine,
    parseMol2BondLine,
    summarizeMol2,
} from "cap:science";
```

## 常量

- `MOL2_RTI_MOLECULE = "@<TRIPOS>MOLECULE"`
- `MOL2_RTI_ATOM = "@<TRIPOS>ATOM"`
- `MOL2_RTI_BOND = "@<TRIPOS>BOND"`
- `MOL2_RTI_SUBSTRUCTURE = "@<TRIPOS>SUBSTRUCTURE"`

## 数据结构

### `Mol2Header`
`@<TRIPOS>MOLECULE` 头部元数据及零拷贝区段切片：
- `name: []const u8` — 分子名称
- `atom_count / bond_count / subst_count / feat_count / set_count: u32` — 计数行统计
- `mol_type: []const u8` — 分子类型（如 `"SMALL"`、`"PROTEIN"`）
- `charge_type: []const u8` — 电荷模型（如 `"GASTEIGER"`、`"NO_CHARGES"`）
- `atom_section: []const u8` — `@<TRIPOS>ATOM` 正文零拷贝切片
- `bond_section: []const u8` — `@<TRIPOS>BOND` 正文零拷贝切片

### `Mol2Atom`
空白分隔原子记录（兼容 6 列精简格式至 9 列完整格式）：
- `atom_id: u32`, `atom_name: []const u8`
- `x: f64`, `y: f64`, `z: f64`
- `sybyl_type: []const u8` — SYBYL 力场原子类型（如 `"C.ar"`, `"O.2"`, `"N.am"`）
- `subst_id: u32`, `subst_name: []const u8` — 子结构 ID 与名称（缺省 `0` / `""`）
- `charge: f64`, `has_charge: bool` — 原子部分电荷（缺省 `0.0` / `false`）

### `Mol2Bond` & `Mol2Stats`
- `Mol2Bond`：`bond_id: u32`, `origin_atom: u32`, `target_atom: u32`, `bond_type: []const u8`（`"1"`/`"2"`/`"3"`/`"ar"`/`"am"`），附 `isAromatic(): bool`
- `Mol2Stats`：`molecule_count: u32`, `total_atoms: u32`, `total_bonds: u32`, `aromatic_bonds: u32`, `total_charge: f64`

## 核心函数与迭代器

- `isMol2(data: []const u8): bool` — 检测是否包含 `@<TRIPOS>MOLECULE`
- `parseMol2Header(mol_slice: []const u8): Mol2Header?` — 解析头部并定位 ATOM/BOND 区段切片
- `parseMol2AtomLine(line)` / `parseMol2BondLine(line)` — 单行原子与化学键解析
- `Mol2AtomIter.init(slice)` / `.next()` — 迭代原子记录（跳过空行与 `#` 注释，遇下一 `@<TRIPOS>` 停止）
- `Mol2BondIter.init(slice)` / `.next()` — 迭代化学键记录
- `Mol2RecordIter.init(data)` / `.next()` — 按 `@<TRIPOS>MOLECULE` 零拷贝切分多分子记录
- `summarizeMol2(data): Mol2Stats` — 单遍汇总分子数、原子数、键数、芳香键数与净电荷

## 快速示例

```cp
import { Mol2RecordIter, parseMol2Header, Mol2AtomIter, summarizeMol2 } from "cap:science";

fn analyzeLigands(mol2_bytes: []const u8) {
    let recs = Mol2RecordIter.init(mol2_bytes);
    while (rec := recs.next()) {
        if (hdr := parseMol2Header(rec)) {
            let ait = Mol2AtomIter.init(hdr.atom_section);
            while (atom := ait.next()) {
                * = atom.sybyl_type;
            }
        }
    }
    const stats = summarizeMol2(mol2_bytes);
    * = stats.total_charge;
}
```
