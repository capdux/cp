# CP 标准库：编译期元编程与位操作工具集 (`cap:meta`)

> 参考来源：`/data/dev/bun/src/meta/meta.cp`, `/data/dev/bun/src/meta/bits.cp`, `/data/dev/bun/src/meta/traits.cp` | 吸收日期：2026-11-22

## 一、 架构总览与设计准则

`cap:meta` 是 CP 语言官方内置的编译期类型内省（Type Traits）与无分支位操作标准库，吸收自 Bun `src/meta/`（`bits.zig`、`traits.zig`、`meta.zig`）。该库向编译器工具链、容器、内存分配器及底层协议解析器提供零运行时开销的位宽运算、内存对齐、整数高低位打包与编译期类型特征探测原语。

### 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/meta/bits.cp` | `// Source: /data/dev/bun/src/meta/bits.cp` | 2 的幂判定、页/对齐计算、硬件 `CLZ`/`CTZ`/`POPCNT`、位翻转/循环移位、`u16`/`u32`/`u64` 打包解包与打包位域辅助 |
| `lib/meta/traits.cp` | `// Source: /data/dev/bun/src/meta/traits.cp` | 编译期类型分类（`isInt`/`isFloat`/`isStruct`/`isEnum`/`isUnion`）、成员/声明探测、`sizeOf`/`alignOf`/`typeName`/`ChildType` |
| `lib/meta/index.cp` | `// Source: /data/dev/bun/src/meta/meta.cp` | `cap:meta` 统一聚合导出入口 |

---

## 二、 核心 API 与设计语义

### 1. 位操作、对齐与打包 (`bits.cp`)

- **2 的幂与对齐**：
  - `isPowerOfTwo(v)`：无分支判断 `v > 0 && (v & (v - 1)) == 0`。
  - `nextPowerOfTwo(v)`：返回大于等于 `v` 的最小 2 的幂（`v <= 1` 时返回 `1`）。
  - `alignUp(value, alignment)` / `alignDown(value, alignment)` / `isAligned(value, alignment)`：按 2 的幂边界向上/向下对齐与校验。
- **硬件位计数与变换**：
  - `bitCount(v)`（`@popCount`）、`leadingZeros(v)`（`@clz`）、`trailingZeros(v)`（`@ctz`）。
  - `reverseBits(v)`（`@bitReverse`）、`rotateLeft(v, shift)`、`rotateRight(v, shift)`。
- **高低位打包与解包**：
  - `packU16(hi, lo): u16` / `unpackU16(v): [u8, u8]`
  - `packU32(hi, lo): u32` / `unpackU32(v): [u16, u16]`
  - `packU64(hi, lo): u64` / `unpackU64(v): [u32, u32]`
- **紧凑位域辅助**：`asInt`、`fromInt`、`bitfieldIntersects`、`bitfieldAnd`、`bitfieldOr`、`bitfieldInvert`、`bitfieldInsert`、`bitfieldRemove`、`bitfieldContains`。

### 2. 编译期类型特征探测 (`traits.cp`)

- **类型分类谓词**（全部 `comptime T: type` 零开销编译期求值）：
  - `isInt(T)`, `isFloat(T)`, `isNumber(T)`, `isBool(T)`
  - `isSlice(T)`, `isPointer(T)`, `isSingleItemPtr(T)`, `isConstPtr(T)`, `isOptional(T)`, `isArray(T)`
  - `isStruct(T)`, `isEnum(T)`, `isUnion(T)`, `isContainer(T)`, `isExternContainer(T)`, `isIndexable(T)`, `isZigString(T)`, `isSimpleCopyType(T)`
- **布局与反射探测**：
  - `hasField(T, name)`, `hasDecl(T, name)`, `fieldCount(T)`
  - `sizeOf(T)`, `alignOf(T)`, `bitSizeOf(T)`, `typeName(T)`, `typeBaseName(fullname)`
  - `ChildType(T)`（提取指针/切片/数组/可选/向量/错误联合的底层元素类型）、`ItemType(T)`

---

## 三、 可运行代码示例

```typescript
import {
    isPowerOfTwo,
    nextPowerOfTwo,
    alignUp,
    alignDown,
    isAligned,
    bitCount,
    leadingZeros,
    trailingZeros,
    packU32,
    unpackU32,
    isInt,
    isStruct,
    isOptional,
    hasField,
    fieldCount,
    ChildType,
    typeName,
} from "cap:meta";

pub struct PacketHeader {
    magic: u32 = 0;
    flags: u16 = 0;
}

pub fn demoMeta() {
    // 1. 位宽与对齐计算
    const aligned = alignUp(4100, 4096); // 8192
    const ok = isPowerOfTwo(aligned) && isAligned(aligned, 4096);
    * = ok;
    const next_p2 = nextPowerOfTwo(1000); // 1024
    const ones = bitCount(0b101101);      // 4
    const lzc = leadingZeros(0x00FF as u16); // 8
    const tzc = trailingZeros(0b101000);     // 3
    * = next_p2 + ones + lzc + tzc;

    // 2. 整数高低位打包与解构
    const packed_u32 = packU32(0x1234, 0x5678);
    const [hi, lo] = unpackU32(packed_u32);
    * = hi + lo;

    // 3. 编译期类型探测
    comptime {
        if (!isInt(u64) || !isStruct(PacketHeader) || !isOptional(u32?)) {
            @compileError("trait check failed");
        }
        if (!hasField(PacketHeader, "magic") || fieldCount(PacketHeader) != 2) {
            @compileError("struct reflection check failed");
        }
        if (ChildType([]const u8) != u8) {
            @compileError("ChildType check failed");
        }
    }
    const name = typeName(PacketHeader); // "PacketHeader"
    * = name;
}
```

---

## 上游 Bun `meta` 行为契约与边界注意事项

1. **2 的幂对齐前置条件（`alignUp` / `alignDown` / `isAligned`）**：
   - `alignment` 必须为正 2 的幂（可用 `isPowerOfTwo(alignment)` 预检）；`nextPowerOfTwo(0)` 与 `nextPowerOfTwo(1)` 均返回 `1`，对接近整数最大值的输入应注意防止无符号上溢。
2. **零开销硬件位指令映射**：
   - `bitCount`（`popcnt`）、`leadingZeros`（`lzcnt`/`clz`）、`trailingZeros`（`tzcnt`/`ctz`）与 `rotateLeft`/`rotateRight` 直接编译为单周期 CPU 指令；注意对 `0` 调用 `leadingZeros`/`trailingZeros` 返回完整位宽（如 `u64` 为 `64`）。
3. **编译期反射零运行时开销（`traits.cp`）**：
   - `isInt`、`isStruct`、`hasField`、`fieldCount`、`sizeOf`、`alignOf`、`typeName` 与 `ChildType` 全部在编译期（`comptime`）折叠为常量，适用于泛型容器特化与零成本序列化代码生成。
