# `cap:netcdf` — 零分配 Unidata NetCDF (CDF-1 / CDF-2 / CDF-5) 解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:binfmt`，导入请使用 `import { ... } from "cap:binfmt"`。

> **模块路径**：`lib/netcdf/`（导入方式：`import { ... } from "cap:binfmt";`）  
> **特性**：100% 纯 CP、零堆分配、严格 4 字节大端对齐填充、完整支持 CDF-1 (Classic)、CDF-2 (64-bit Offset) 与 CDF-5 (64-bit Data)。

---

## 一、 核心常量与枚举

| 符号 | 值 / 成员 | 说明 |
| :--- | :--- | :--- |
| `NC_MAGIC` | `"CDF"` | 3 字节魔数前缀（第 4 字节为版本号 1/2/5） |
| `NC_ABSENT` / `NC_DIMENSION` / `NC_VARIABLE` / `NC_ATTRIBUTE` | `0` / `10` / `11` / `12` | 三段节标签 |
| `NC_UNLIMITED` | `0` | 无限记录维度长度哨兵 |
| `NetcdfFormat` | `classic=1`, `offset64=2`, `cdf5=5`, `unknown=0` | 容器版本枚举 |
| `NetcdfType` | `byte=1`, `char=2`, `short=3`, `int=4`, `float=5`, `double=6`, `ubyte=7`, `ushort=8`, `uint=9`, `int64=10`, `uint64=11`, `unknown=0` | 标量数据类型枚举 |

---

## 二、 核心数据结构与迭代器

- **`NetcdfHeader`**：`format`, `numrecs`, `dim_count`, `global_attr_count`, `var_count`, `dims_offset`, `attrs_offset`, `vars_offset`, `header_end_offset`
- **`NetcdfDim` & `NetcdfDimIter`**：`name`, `length`, `is_unlimited`
- **`NetcdfAttr` & `NetcdfAttrIter`**：`name`, `nc_type`, `count`, `raw_values`（零拷贝未填充值切片）
- **`NetcdfVar` & `NetcdfVarIter`**：`name`, `ndims`, `nc_type`, `vsize`, `begin_offset`, `attr_count`

---

## 三、 核心函数

- `detectNetcdfFormat(data: []const u8): NetcdfFormat` / `isNetcdf(data: []const u8): bool`
- `netcdfTypeSize(t: NetcdfType): u8` / `decodeNetcdfType(code: u32): NetcdfType`
- `parseNetcdfHeader(data: []const u8): NetcdfHeader?`

---

## 四、 使用示例

```typescript
import {
    parseNetcdfHeader,
    NetcdfDimIter,
    NetcdfAttrIter,
    NetcdfVarIter,
} from "cap:binfmt";

fn inspectNetcdf(bytes: []const u8) {
    if (hdr := parseNetcdfHeader(bytes)) {
        let dims = NetcdfDimIter.init(bytes, hdr);
        while (d := dims.next()) {
            * = d.length;
        }
        let attrs = NetcdfAttrIter.init(bytes, hdr);
        while (a := attrs.next()) {
            * = a.raw_values.len;
        }
        let vars = NetcdfVarIter.init(bytes, hdr);
        while (v := vars.next()) {
            * = v.begin_offset;
        }
    }
}
```
