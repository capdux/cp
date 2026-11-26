# @cap/natsort

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:natural-compare` ([https://github.com/litejs/natural-compare-lite](https://github.com/litejs/natural-compare-lite))（原库名 `natural-compare` → CP 包名 `@cap/natsort`）

零堆分配的 CP 自然序（Alphanumeric / Natural Sort）字符串比较与原地排序库 — 洁净室 MIT 实现，对标 npm `natural-compare` / `natsort` 与 Rust `natord` / `alphanumeric-sort`。

Zero-allocation natural / alphanumeric string comparison and in-place sorting for CP — clean-room MIT parity with npm `natural-compare` / `natsort` and Rust `natord` / `alphanumeric-sort`.

## 特性 / Features

- **任意长度大整数不溢出 / Arbitrary-length integers without overflow**：跳过前导零后先比较有效数字位数再逐位比较，支持远超 `u64::MAX` 的超长数字串。
- **严格全序与确定性 / Strict Total Order**：规范化自然序相同时按原始字节序进行稳定 tie-break（例如 `"a001" < "a1"`，`"File2" < "file2"`）。
- **零堆分配原地排序 / Zero-allocation In-place Sorting**：纯 CP 实现（小切片插入排序 + 大切片堆排序），$O(1)$ 额外空间，最坏 $O(n \log n)$ 时间复杂度。

## 安装 / Install

```bash
cap add @cap/natsort
```

## API 参考 / API Reference

| 函数 / Function | 签名 / Signature | 说明 / Description |
| --- | --- | --- |
| `naturalCompare` | `(a: string, b: string) -> i8` | 大小写敏感自然序比较，返回 `-1` / `0` / `1` |
| `naturalCompareIgnoreCase` | `(a: string, b: string) -> i8` | ASCII 大小写无关自然序比较，相等时按原字节 tie-break |
| `naturalLess` | `(a: string, b: string) -> bool` | 判断 `a` 是否在自然序上严格小于 `b` |
| `naturalLessIgnoreCase` | `(a: string, b: string) -> bool` | 判断 `a` 是否在大小写无关自然序上严格小于 `b` |
| `naturalEq` | `(a: string, b: string) -> bool` | 判断 `a` 与 `b` 在自然序比较下是否相等 |
| `sortNatural` | `(items: []string)` | 原地升序自然排序 |
| `sortNaturalDesc` | `(items: []string)` | 原地降序自然排序 |
| `sortNaturalIgnoreCase` | `(items: []string)` | 原地 ASCII 大小写无关升序自然排序 |
| `isSortedNatural` | `(items: []string) -> bool` | 检查切片是否已按升序自然序排列 |

## 使用示例 / Usage

```cp
import {
    naturalCompare,
    naturalLess,
    sortNatural,
    sortNaturalDesc,
    sortNaturalIgnoreCase,
    isSortedNatural,
} from "@cap/natsort";

// 比较文件名与版本号 / Compare filenames & versions
const c1 = naturalCompare("file2.txt", "file10.txt"); // -1
const c2 = naturalCompare("v1.2.9", "v1.2.10");       // -1

// 超大整数段不溢出 / Arbitrary-length numeric segments (> u64::MAX)
const ok = naturalLess(
    "id_999999999999999999999999999999",
    "id_1000000000000000000000000000000",
); // true

// 原地自然排序 / In-place natural sort
let files: [4]string = ["file10.txt", "file2.txt", "file20.txt", "file1.txt"];
sortNatural(files[0..4]);
// files => ["file1.txt", "file2.txt", "file10.txt", "file20.txt"]
const sorted = isSortedNatural(files[0..4]); // true
```

## License

MIT
