# `cap:tagged_ptr` — 64 位高位标签指针与紧凑标签联合体

> 溯源：从 `/data/dev/bun/src/ptr/tagged_pointer.cp` 吸收并规范化为纯 CP 标准库模块。

`cap:tagged_ptr` 利用 64 位虚拟地址空间的高 15 位（低 49 位指针地址 + 高 15 位无符号标签），提供严格 **8 字节** 的标签指针与多类型紧凑指针联合体：

- **`TaggedPointer`**：49 位指针地址 + 15 位标签（`0..=32767`），支持零开销位级编解码、原地修改标签不破坏地址、`null` 指针携带非零标签及安全 `getPtr` 解包。
- **`TaggedPtr2<A, B>` & `TaggedPtr4<A, B, C, D>`**：8 字节强类型多分支指针联合体，类型不匹配或为空时 `asA/B/C/D()` 严格返回 `null`。

---

## 快速导入

```cp
import {
    TaggedPointer,
    TaggedPtr2,
    TaggedPtr4,
    PTR_BITS,
    TAG_BITS,
    PTR_MASK,
    MAX_TAG,
} from "cap:tagged_ptr";
```

---

## API 参考

### 1. `TaggedPointer` (`@sizeOf == 8`)

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `empty` / `nullWithTag` | `static fn empty()` / `static fn nullWithTag(tag_val: u15)` | 构造空指针（可携带非零 15 位标签） |
| `fromAddress` | `static fn fromAddress(addr: usize, tag_val: u15): TaggedPointer` | 从地址与标签构造 |
| `fromPtr` / `fromConstPtr` | `static fn fromPtr(comptime T: type, ptr: *T!, tag_val: u15)` | 从裸指针与标签构造 |
| `fromU64` / `toU64` | `static fn fromU64(raw: u64)` / `fn toU64(): u64` | 64 位原始位模式互转 |
| `address` / `tag` / `isNull` | `fn address(): usize` / `fn tag(): u15` / `fn isNull(): bool` | 提取低 49 位地址、高 15 位标签或判空 |
| `setTag` / `withTag` | `fn setTag(*_, new_tag: u15)` / `fn withTag(new_tag: u15)` | 修改标签且不破坏低 49 位地址 |
| `setAddress` | `fn setAddress(*_, new_addr: usize)` | 修改地址且完整保留高 15 位标签 |
| `getPtr` / `getConstPtr` | `fn getPtr(comptime T: type): (*T!)?` | 安全还原类型化裸指针（地址为 0 返回 `null`） |

### 2. `TaggedPtr2<A, B>` & `TaggedPtr4<A, B, C, D>` (`@sizeOf == 8`)

- 构造：`empty()`、`fromA(ptr)`、`fromB(ptr)`（`TaggedPtr4` 另含 `fromC` / `fromD`）、`fromU64(raw)`
- 判定：`isEmpty()`、`isA()`、`isB()`（`isC()` / `isD()`）、`tag()`、`toU64()`
- 安全解包：`asA(): (*A!)?`、`asB(): (*B!)?`（类型不匹配返回 `null`）
- 修改：`setA(*_, ptr)`、`setB(*_, ptr)`、`clear(*_)`

---

## 使用示例

```cp
import { TaggedPointer, TaggedPtr2 } from "cap:tagged_ptr";

struct TextNode { len: usize }
struct NumNode { val: i64 }

fn demo() {
    let t = TextNode{ len: 12 };
    let n = NumNode{ val: 99 };

    let tp = TaggedPointer.fromPtr(TextNode, &t, 7);
    if (p := tp.getPtr(TextNode)) {
        unsafe { p.len = 24; }
    }

    let u = TaggedPtr2<TextNode, NumNode>.fromB(&n);
    if (np := u.asB()) {
        unsafe { np.val += 1; }
    }
}
```
