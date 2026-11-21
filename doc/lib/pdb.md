# `cap:pdb` — RCSB Protein Data Bank (`.pdb`) 零分配 3D 结构解析库

`cap:pdb` 提供纯 CP、零堆分配的 RCSB Protein Data Bank (PDB v3.3) 80 列定宽生物大分子 3D 结构解析能力，支持 `HEADER`、`TITLE`、`EXPDTA`、`ATOM`、`HETATM`、`MODEL`/`ENDMDL`、`CONECT` 与 `END` 记录，兼容 `\r\n` 与 `\n` 换行及 54 列精简格式。

## 导入方式

```cp
import {
    PdbHeader,
    PdbAtom,
    PdbConect,
    PdbStats,
    PdbAtomIter,
    PdbConectIter,
    isPdb,
    parsePdbHeader,
    parsePdbAtomLine,
    parsePdbConectLine,
    summarizePdb,
} from "cap:pdb";
```

## 数据结构

### `PdbHeader`
零拷贝引用原始缓冲区的头部元数据：
- `classification: []const u8` — `HEADER` 列 11..50 蛋白分类
- `dep_date: []const u8` — `HEADER` 列 51..59 提交日期（如 `"30-APR-81"`）
- `pdb_id: []const u8` — `HEADER` 列 63..66 四字符 PDB ID（如 `"1CRN"`）
- `title: []const u8` — 首个 `TITLE` 行内容（列 11..80）
- `expdta: []const u8` — `EXPDTA` 实验方法（如 `"X-RAY DIFFRACTION"`、`"SOLUTION NMR"`）

### `PdbAtom`
PDB v3.3 80 列定宽 `ATOM  ` / `HETATM` 记录：
- `is_hetatm: bool` — 是否为 `HETATM` 异源原子
- `serial: i32` — 原子序号（列 7..11）
- `name: []const u8` — 原子名（列 13..16 去空白，如 `"CA"`）
- `alt_loc: u8` — 构象交替标识（列 17）
- `res_name: []const u8` — 残基名（列 18..20 去空白，如 `"THR"`、`"HOH"`）
- `chain_id: u8` — 链标识（列 22，如 `'A'`）
- `res_seq: i32` — 残基序号（列 23..26）
- `x: f64`, `y: f64`, `z: f64` — 3D 笛卡尔坐标（列 31..54，单位 Å）
- `occupancy: f64` — 占有率（列 55..60，缺省 `1.0`）
- `temp_factor: f64` — B-factor 温度因子（列 61..66，缺省 `0.0`）
- `element: []const u8` — 元素符号（列 77..78 去空白）
- `charge: []const u8` — 原子电荷（列 79..80 去空白）
- `model_num: u32` — 所属 `MODEL` 编号（迭代时自动填充，单行解析默认为 `0`）
- 方法 `distanceTo(other: PdbAtom): f64` — 计算两原子间 3D 欧氏距离（单位 Å）

### `PdbConect`
`CONECT` 共价键连接记录（定长栈数组，零堆分配）：
- `serial: i32` — 中心原子序号（列 7..11）
- `bonds: [4]i32` — 最多 4 个共价键合原子序号（列 12..31）
- `bond_count: u8` — 实际解析到的键合原子数量（`0..4`）

### `PdbStats`
单遍结构统计汇总：
- `atom_count: u32` — `ATOM  ` 数量
- `hetatm_count: u32` — `HETATM` 数量
- `model_count: u32` — 模型数量（显式 `MODEL` 计数，无 `MODEL` 但含原子时为 `1`）
- `chain_count: u32` — 唯一非空链 ID 数量（基于栈上 256 字节查找表 O(1) 去重）
- `min_x / max_x / min_y / max_y / min_z / max_z: f64` — 3D 轴对齐包围盒
- `centroid_x / centroid_y / centroid_z: f64` — 几何质心坐标

## 核心函数与迭代器

- `isPdb(data: []const u8): bool` — 快速判断缓冲区是否以合法 PDB 记录开头
- `parsePdbHeader(data: []const u8): PdbHeader?` — 提取 `HEADER`、`TITLE`、`EXPDTA` 元数据
- `parsePdbAtomLine(raw_line: []const u8): PdbAtom?` — 按 PDB v3.3 严格定宽列解析单条 `ATOM  `/`HETATM`（至少 54 列，兼容 80 列）
- `parsePdbConectLine(raw_line: []const u8): PdbConect?` — 解析单条 `CONECT` 共价键记录
- `PdbAtomIter.init(data)` / `iter.next()` — 零分配逐行迭代全部 `ATOM`/`HETATM`，自动更新 `model_num`，遇 `END` 停止
- `PdbConectIter.init(data)` / `iter.next()` — 零分配逐行迭代全部 `CONECT` 记录，遇 `END` 停止
- `summarizePdb(data: []const u8): PdbStats` — 单遍统计原子数、链数、模型数、3D 包围盒与质心

## 快速示例

```cp
import { parsePdbHeader, PdbAtomIter, summarizePdb } from "cap:pdb";

fn inspectStructure(pdb_bytes: []const u8) {
    if (hdr := parsePdbHeader(pdb_bytes)) {
        // hdr.pdb_id == "1CRN", hdr.expdta == "X-RAY DIFFRACTION"
        * = hdr.pdb_id;
    }

    let it = PdbAtomIter.init(pdb_bytes);
    while (atom := it.next()) {
        * = atom.name;
    }

    const stats = summarizePdb(pdb_bytes);
    * = stats.centroid_x;
}
```
