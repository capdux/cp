# `cap:hprof` — 零分配 JVM HPROF 二进制堆转储解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:binfmt`，导入请使用 `import { ... } from "cap:binfmt"`。

> **模块路径**：`lib/hprof/`（导入方式：`import { ... } from "cap:binfmt";`）  
> **特性**：100% 纯 CP、零堆分配、大端序完整遵循 JVM HPROF 1.0.1 / 1.0.2 二进制规范，支持 32-bit 与 64-bit 对象 ID。

---

## 一、 核心常量与枚举

- **顶层 Record Tag**：`TAG_UTF8 (0x01)`、`TAG_LOAD_CLASS (0x02)`、`TAG_UNLOAD_CLASS (0x03)`、`TAG_STACK_FRAME (0x04)`、`TAG_STACK_TRACE (0x05)`、`TAG_ALLOC_SITES (0x06)`、`TAG_HEAP_SUMMARY (0x07)`、`TAG_START_THREAD (0x0A)`、`TAG_END_THREAD (0x0B)`、`TAG_HEAP_DUMP (0x0C)`、`TAG_CPU_SAMPLES (0x0D)`、`TAG_CONTROL_SETTINGS (0x0E)`、`TAG_HEAP_DUMP_SEGMENT (0x1C)`、`TAG_HEAP_DUMP_END (0x2C)`
- **Heap Dump 子记录 Tag**：`SUB_ROOT_UNKNOWN (0xFF)`、`SUB_ROOT_JNI_GLOBAL (0x01)`、`SUB_ROOT_JNI_LOCAL (0x02)`、`SUB_ROOT_JAVA_FRAME (0x03)`、`SUB_ROOT_STICKY_CLASS (0x05)`、`SUB_ROOT_THREAD_OBJ (0x08)`、`SUB_CLASS_DUMP (0x20)`、`SUB_INSTANCE_DUMP (0x21)`、`SUB_OBJ_ARRAY_DUMP (0x22)`、`SUB_PRIM_ARRAY_DUMP (0x23)`
- **`HprofBasicType`**：`object=2`, `boolean=4`, `char=5`, `float=6`, `double=7`, `byte=8`, `short=9`, `int=10`, `long=11`, `unknown=0`

---

## 二、 核心 API

- `isHprof(data)` / `parseHprofHeader(data): HprofHeader?`
- `hprofBasicTypeSize(t: HprofBasicType, id_size: u32): u8?` / `decodeHprofBasicType(code: u8): HprofBasicType`
- `HprofRecordIter.init(data, header)` / `HprofRecordIter.fromData(data)` / `next(*_): HprofRecord?`
- `parseHprofUtf8(body, id_size): HprofUtf8?`
- `parseHprofLoadClass(body, id_size): HprofLoadClass?`
- `parseHprofHeapSummary(body): HprofHeapSummary?`
- `summarizeHprof(data): HprofStats?`

---

## 三、 使用示例

```typescript
import {
    parseHprofHeader,
    HprofRecordIter,
    parseHprofUtf8,
    summarizeHprof,
    TAG_UTF8,
} from "cap:binfmt";

fn inspectHprof(bytes: []const u8) {
    if (hdr := parseHprofHeader(bytes)) {
        let it = HprofRecordIter.init(bytes, hdr);
        while (rec := it.next()) {
            if (rec.tag == TAG_UTF8) {
                if (u := parseHprofUtf8(rec.body, hdr.id_size)) {
                    * = u.text.len;
                }
            }
        }
    }
    if (stats := summarizeHprof(bytes)) {
        * = stats.total_records;
    }
}
```
