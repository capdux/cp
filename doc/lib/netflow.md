# `cap:netflow` — 零分配 Cisco NetFlow v5 & RFC 3954 NetFlow v9 解析库

> **模块路径**：`lib/netflow/`（入口 `lib/netflow/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整支持 Cisco NetFlow v5（24B 报头 + 48B Flow Record 迭代）与 RFC 3954 NetFlow v9（20B 报头 + FlowSet / Template / Options Template / Data / FieldSpec 零拷贝嵌套迭代）。

---

## 一、导入方式

```cp
import {
    NETFLOW_UDP_PORT,
    NETFLOW_V5,
    NETFLOW_V9,
    NF5_HEADER_SIZE,
    NF5_RECORD_SIZE,
    NF9_HEADER_SIZE,
    NF9_FLOWSET_HEADER_SIZE,
    NF9_FLOWSET_TEMPLATE,
    NF9_FLOWSET_OPTIONS_TEMPLATE,
    NF9_FLOWSET_DATA_MIN,
    Nf5Header,
    Nf5Record,
    Nf5RecordIter,
    Nf5Packet,
    Nf9Header,
    Nf9FieldSpec,
    Nf9FieldSpecIter,
    Nf9TemplateRecord,
    Nf9TemplateIter,
    Nf9FlowSet,
    Nf9FlowSetIter,
    Nf9Packet,
    peekNetflowVersion,
    parseNetflowV5,
    parseNetflowV9,
    isNetflow,
} from "cap:netflow";
```

---

## 二、核心 API

- `peekNetflowVersion(data): u16?` — 零拷贝预读前 2B 大端版本号（`5` / `9`）。
- `isNetflow(data): bool` — 快速判定是否为合法 NetFlow v5 或 v9 报文。
- `parseNetflowV5(data): Nf5Packet?` — 严格校验 `version == 5`、`1 <= count <= 30` 与 `24 + count * 48 <= data.len`；`pkt.records()` 迭代 48B `Nf5Record`。
- `parseNetflowV9(data): Nf9Packet?` — 严格校验 `version == 9` 与 `data.len >= 20`；`pkt.flowsets()` 迭代 `Nf9FlowSet`（`isTemplate()`, `isOptionsTemplate()`, `isData()`, `templates()` -> `Nf9TemplateRecord.fields()`）。

---

## 三、使用示例

```cp
import { parseNetflowV9 } from "cap:netflow";

fn inspectNf9(pkt: []const u8) {
    if (nf9 := parseNetflowV9(pkt)) {
        let fs_it = nf9.flowsets();
        while (fs := fs_it.next()) {
            if (fs.isTemplate()) {
                let t_it = fs.templates();
                while (tpl := t_it.next()) {
                    let f_it = tpl.fields();
                    while (spec := f_it.next()) {
                        * = spec.field_type;
                        * = spec.field_len;
                    }
                }
            }
        }
    }
}
```
