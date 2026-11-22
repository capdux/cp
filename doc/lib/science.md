# CP 标准库：科学计算、生物信息学、计算化学、控制与 3D 网格手册 (`cap:science` / `cap.science`)

## 一、 架构总览与设计准则

`cap:science` 是 CP 语言官方内置的零分配纯原生科学数据格式与工程控制标准库，覆盖：
1. **生物信息学与基因组学**（`fasta.cp`, `fastq.cp`, `sam.cp`, `vcf.cp`, `gff.cp`, `bed.cp`）
2. **计算化学与晶体/分子结构**（`pdb.cp`, `cif.cp`, `mol2.cp`, `sdf.cp`, `inchi.cp`）
3. **生物医学信号、LiDAR 点云与工业控制**（`edf.cp`, `las.cp`, `pid.cp`, `gcode.cp`）
4. **3D 科学网格与点云模型**（`stl.cp`, `obj.cp`, `ply.cp`）

全模块**零堆分配、零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:science"` 与 `import cap from "cap"; cap.science.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 核心职责 |
| :--- | :--- |
| `lib/science/fasta.cp` | NCBI/EMBL FASTA 序列解析与 IUPAC DNA 反向互补 |
| `lib/science/fastq.cp` | Sanger/Illumina FASTQ 测序记录解析与 Phred 质量统计（QC） |
| `lib/science/sam.cp` | SAM v1.6 序列比对解析与 CIGAR 长度计算 |
| `lib/science/vcf.cp` | HTSlib VCF 4.2/4.3 基因变异格式解析与统计 |
| `lib/science/gff.cp` | GFF3 / GTF2 基因组特征注释解析 |
| `lib/science/bed.cp` | UCSC BED3–BED12 基因组区间解析 |
| `lib/science/pdb.cp` | RCSB Protein Data Bank `.pdb` 蛋白质原子/连接结构解析 |
| `lib/science/cif.cp` | IUCr CIF 1.1 / mmCIF 晶体结构词法与表格循环解析 |
| `lib/science/mol2.cp` | Tripos SYBYL `.mol2` 3D 分子与化学键解析 |
| `lib/science/sdf.cp` | MDL Molfile V2000 与 SDF 结构数据文件解析 |
| `lib/science/inchi.cp` | IUPAC InChI v1.06 分层结构与 InChIKey 解析校验 |
| `lib/science/edf.cp` | European Data Format（EDF/EDF+）脑电/生理信号记录解析 |
| `lib/science/las.cp` | ASPRS LAS 1.2–1.4 LiDAR 激光雷达点云头、VLR 与点流解析 |
| `lib/science/pid.cp` | 工业级抗积分饱和 PID 控制器、Ziegler-Nichols 整定与变化率限幅 |
| `lib/science/gcode.cp` | RS-274 / Marlin / RepRap CNC 与 3D 打印 G-Code 词法与校验和 |
| `lib/science/stl.cp` | 3D STL 二进制与 ASCII 网格读写及包围盒计算 |
| `lib/science/obj.cp` | Wavefront 3D OBJ 顶点/法线/面片流式解析 |
| `lib/science/ply.cp` | Stanford PLY 3D 网格与点云头/元素流式解析 |
| `lib/science/index.cp` | `cap:science` 与 `cap.science` 统一零冲突公开导出入口（共 186 符号） |

---

## 二、 核心 API 索引

### 1. 生物信息学与基因组学
- **`fasta.cp`**：`FastaRecord`, `FastaStats`, `FastaIter`, `isFasta`, `dnaComplement`, `reverseComplementDna`, `summarizeFasta`
- **`fastq.cp`**：`PHRED_OFFSET_33`, `PHRED_OFFSET_64`, `FastqRecord`, `FastqStats`, `FastqIter`, `isFastq`, `summarizeFastq`
- **`sam.cp`**：`SAM_FLAG_*`, `CigarOp`, `SamHeaderLine`, `SamRecord`, `SamStats`, `CigarIter`, `SamHeaderIter`, `SamRecordIter`, `cigarRefLen`, `cigarQueryLen`, `findSamHeaderTag`, `parseSamLine`, `summarizeSam`
- **`vcf.cp`**：`VcfHeader`, `VcfRecord`, `VcfStats`, `isVcf`, `parseVcfHeader`, `VcfRecordIter`, `summarizeVcf`
- **`gff.cp`**：`GFF_PHASE_NONE`, `GffRecord`, `GffStats`, `GffIter`, `isGff3`, `parseGffLine`, `summarizeGff`
- **`bed.cp`**：`BedStrand`, `BedRecord`, `BedStats`, `BedIter`, `parseBedLine`, `summarizeBed`

### 2. 计算化学与分子结构
- **`pdb.cp`**：`PdbHeader`, `PdbAtom`, `PdbConect`, `PdbStats`, `PdbAtomIter`, `PdbConectIter`, `isPdb`, `parsePdbHeader`, `parsePdbAtomLine`, `parsePdbConectLine`, `summarizePdb`
- **`cif.cp`**：`CifTokenKind`, `CifToken`, `CifEntry`, `CifStats`, `isCif`, `CifLexer`, `CifPairIter`, `findCifTag`, `CifRow`, `CifRowIter`, `CifLoop`, `CifLoopIter`, `summarizeCif`
- **`mol2.cp`**：`MOL2_RTI_*`, `Mol2Header`, `Mol2Atom`, `Mol2Bond`, `Mol2Stats`, `Mol2AtomIter`, `Mol2BondIter`, `Mol2RecordIter`, `isMol2`, `parseMol2Header`, `parseMol2AtomLine`, `parseMol2BondLine`, `summarizeMol2`
- **`sdf.cp`**：`SdfMolHeader`, `SdfAtom`, `SdfBond`, `SdfProperty`, `SdfStats`, `SdfAtomIter`, `SdfBondIter`, `SdfPropIter`, `SdfRecordIter`, `isSdf`, `parseMolHeader`, `findSdfProp`, `summarizeSdf`
- **`inchi.cp`**：`INCHI_PREFIX`, `INCHI_KEY_LEN`, `InchiLayerKind`, `InchiLayer`, `InchiParsed`, `FormulaElement`, `InchiKey`, `InchiLayerIter`, `FormulaIter`, `isInchi`, `parseInchi`, `parseInchiKey`, `isValidInchiKey`

### 3. 生物医学信号、LiDAR、工业控制与 3D 网格
- **`edf.cp`**：`EDF_MAIN_HEADER_SIZE`, `EDF_SIGNAL_HEADER_SIZE`, `EdfHeader`, `EdfSignal`, `isEdf`, `parseEdfHeader`, `parseEdfSignal`, `edfRecordByteSize`, `readEdfDigitalSample`
- **`las.cp`**：`LAS_MAGIC`, `LasHeader`, `LasVlr`, `LasPoint`, `isLas`, `parseLasHeader`, `LasVlrIter`, `LasPointIter`
- **`pid.cp`**：`PidConfig`, `PidTerms`, `PidController`, `zieglerNicholsPid`, `rateLimit`
- **`gcode.cp`**：`GcodeWord`, `GcodeLine`, `GcodeStats`, `gcodeChecksum`, `verifyGcodeChecksum`, `GcodeWordIter`, `parseGcodeLine`, `GcodeIter`, `summarizeGcode`
- **`stl.cp` / `obj.cp` / `ply.cp`**：`detectStlFormat`, `parseStlBinaryHeader`, `writeStlBinary`, `computeStlBinaryBounds`, `ObjLineIter`, `summarizeObj`, `parsePlyHeader`, `PlyElementIter`

---

## 三、 使用示例

```typescript
import {
    PidConfig,
    PidController,
    reverseComplementDna,
    isValidInchiKey,
} from "cap:science";

pub fn demoScience() {
    // 1. 抗积分饱和 PID 控制步进
    let pid = PidController.init(PidConfig{ kp: 2.0, ki: 0.5, kd: 0.1, out_min: -10.0, out_max: 10.0 });
    const u = pid.update(1.0, 0.2, 0.01);
    * = u;

    // 2. DNA 反向互补
    let out: [4]u8 = [0; 4];
    * = reverseComplementDna("ATGC", &out);

    // 3. IUPAC InChIKey 校验
    * = isValidInchiKey("BSYNRYMUTXBXSQ-UHFFFAOYSA-N");
}
```
