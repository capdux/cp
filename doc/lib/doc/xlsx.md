# `cap:xlsx` — 零分配 OOXML SpreadsheetML `.xlsx` 解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:doc`，导入请使用 `import { ... } from "cap:doc"`。

> **模块路径**：`lib/xlsx/`（入口 `lib/xlsx/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整覆盖 Excel A1 单元格引用编解码、`xl/workbook.xml` 工作表清单迭代、`xl/sharedStrings.xml` 共享字符串与富文本合并反转义，以及 `xl/worksheets/sheet*.xml` 单元格流式迭代。

---

## 一、导入方式

```cp
import {
    MAX_XLSX_COL,
    MAX_XLSX_ROW,
    XlsxCellRef,
    XlsxSheetInfo,
    XlsxCellType,
    XlsxCell,
    XlsxSheetIter,
    XlsxSharedStringIter,
    XlsxCellIter,
    parseCellRef,
    formatColName,
    extractSharedString,
} from "cap:doc";
```

---

## 二、核心数据结构

| 类型 | 字段 / 枚举值 | 说明 |
| :--- | :--- | :--- |
| `XlsxCellRef` | `col: u32`, `row: u32` | 0-based 列号 (`0..=16383`) 与行号 (`0..=1048575`) |
| `XlsxSheetInfo` | `name: []const u8`, `sheet_id: u32`, `r_id: []const u8`, `is_hidden: bool` | 工作表清单项（兼容 `hidden` 与 `veryHidden`） |
| `XlsxCellType` | `number`, `shared_string`, `inline_string`, `formula_str`, `bool_val`, `error_val`, `unknown` | OOXML `<c t="...">` 单元格类型枚举 |
| `XlsxCell` | `ref: XlsxCellRef`, `cell_type: XlsxCellType`, `style_idx: u32`, `raw_value: []const u8`, `formula: []const u8` | 零拷贝单元格视图；附 `asSharedStringIdx(): u32?`、`asBool(): bool?`、`asF64(): f64?` |

---

## 三、核心 API

### 1. A1 单元格引用编解码
- `parseCellRef(cell_ref: []const u8): XlsxCellRef?`  
  将 `"A1"` -> `(0, 0)`、`"Z10"` -> `(25, 9)`、`"AA1"` -> `(26, 0)`、`"XFD1048576"` -> `(16383, 1048575)`。严格校验大写字母、无前导零行号及 Excel 上界。
- `formatColName(col: u32, out: []u8): usize?`  
  将 0-based 列号格式化为 Excel 列名（`"A"`..`"XFD"`），写入 `out` 并返回字节数；越界或缓冲不足返回 `null`。

### 2. 工作簿清单迭代 (`xl/workbook.xml`)
- `XlsxSheetIter.init(workbook_xml: []const u8): XlsxSheetIter` / `next(*_): XlsxSheetInfo?`  
  零拷贝遍历 `<sheet name="..." sheetId="..." r:id="..." state="..."/>`。

### 3. 共享字符串表迭代 (`xl/sharedStrings.xml`)
- `XlsxSharedStringIter.init(sst_xml: []const u8): XlsxSharedStringIter` / `next(*_): ([]const u8)?`  
  遍历每个 `<si>...</si>` 的内部 XML 切片。
- `extractSharedString(si_xml: []const u8, out: []u8): usize?`  
  合并 `<si>` 内的普通 `<t>` 或富文本 `<r><t>` 文本（自动跳过 `<rPh>` 拼音注音），并反转义 XML 实体 `&amp;/&lt;/&gt;/&quot;/&apos;/&#N;/&#xH;` 写入 `out`。

### 4. 工作表单元格流式迭代 (`xl/worksheets/sheet*.xml`)
- `XlsxCellIter.init(sheet_xml: []const u8): XlsxCellIter` / `next(*_): XlsxCell?`  
  流式迭代 `<sheetData>` 内全部 `<c r="..." t="..." s="...">...</c>`（自动兼容自闭合空单元格 `<c r="B2"/>` 以及 `<f>`/`<v>`/`<is><t>` 子元素）。

---

## 四、使用示例

```cp
import {
    parseCellRef,
    formatColName,
    XlsxCellIter,
} from "cap:doc";

const ref_opt = parseCellRef("AA10");
if (r := ref_opt) {
    let col_buf: [4]u8 = [0; 4];
    const n = formatColName(r.col, col_buf[0..]).?; // "AA" (n == 2)
    * = n;
}

const sheet =
    "<worksheet><sheetData><row r=\"1\">" ++
    "<c r=\"A1\" t=\"s\"><v>0</v></c>" ++
    "<c r=\"B1\" s=\"2\"><f>SUM(C1:C5)</f><v>42.5</v></c>" ++
    "</row></sheetData></worksheet>";

let it = XlsxCellIter.init(sheet);
while (cell := it.next()) {
    * = cell;
}
```
