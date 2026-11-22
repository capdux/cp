# `cap:smile` — 零分配 Jackson Smile 1.0 二进制 JSON 解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:binfmt`，导入请使用 `import { ... } from "cap:binfmt"`。

> **模块路径**：`lib/smile/`（导入方式：`import { ... } from "cap:binfmt";`）  
> **特性**：100% 纯 CP、零堆分配、完整遵循 FasterXML Jackson Smile 1.0 规范（4B 魔数头、7-bit+6-bit 末字节 VInt、ZigZag 有符号整数、7-bit 编码 f32/f64、值流与对象键流解析）。

---

## 一、 核心常量

- `SMILE_HEADER_MAGIC = ":)\n"`、`SMILE_HEADER_SIZE = 4`
- 标志位：`SMILE_FLAG_SHARED_NAMES (0x01)`、`SMILE_FLAG_SHARED_VALUES (0x02)`、`SMILE_FLAG_RAW_BINARY (0x04)`
- 终止符：`SMILE_END_OF_STRING (0xFC)`、`SMILE_END_OF_CONTENT (0xFF)`

---

## 二、 核心 API

- `isSmile(data)` / `parseSmileHeader(data): SmileHeader?`
- `readSmileVInt(data, pos): SmileVInt?` — 大端 7-bit 前置字节 + 6-bit 末字节（最高位 `0x80` 置位）变长整数解码
- `smileZigZagDecode(v: u64): i64` — ZigZag 有符号解码
- `SmileReader.init(data)`：
  - `next(*_): SmileToken?` — 迭代字面量、小整数（-16..15）、VInt32/64、f32/f64、Tiny/Small/Long ASCII & Unicode 字符串、短/长共享值引用、数组/对象边界及文档结束
  - `readKey(*_): SmileKeyToken?` — 对象键模式读取器（空键、短/长键引用、短 ASCII/Unicode 键、`0xFC` 终止长键、`0xFB` 对象结束）

---

## 三、 使用示例

```typescript
import { SmileReader, SmileTokenKind, SmileKeyKind } from "cap:binfmt";

fn inspectSmile(bytes: []const u8) {
    let r = SmileReader.init(bytes);
    if (tok := r.next()) {
        if (tok.kind == SmileTokenKind.start_object) {
            if (k := r.readKey()) {
                if (k.kind == SmileKeyKind.short_ascii) {
                    * = k.text.len;
                }
            }
        }
    }
}
```
