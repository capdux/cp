[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# is-potential-custom-element-name

纯 CP 实现的零分配 WHATWG HTML 自定义元素名称（`PotentialCustomElementName`）校验与清洗库。

- **源包对齐**：对标 `npm` `is-potential-custom-element-name`（MIT，mathiasbynens/is-potential-custom-element-name）
- **包版本**：`1.0.0`
- **内存模型**：全程零堆分配，直接进行 UTF-8 码点解码与栈缓冲区清洗。

## 安装与导入

```bash
cap add is-potential-custom-element-name
```

```cp
import {
    RESERVED_CUSTOM_ELEMENT_NAMES_COUNT,
    PcenError,
    PcenValidationResult,
    isValidCustomElementStart,
    isPcenChar,
    reservedCustomElementNameAt,
    isReservedCustomElementName,
    validateCustomElementName,
    isPotentialCustomElementName,
    sanitizeCustomElementName,
} from "is-potential-custom-element-name";
```

## 核心 API

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `RESERVED_CUSTOM_ELEMENT_NAMES_COUNT` | `const RESERVED_CUSTOM_ELEMENT_NAMES_COUNT: usize` | WHATWG 规范保留的自定义元素名数量（`8`，如 `annotation-xml`、`font-face` 等）。 |
| `PcenError` | `enum PcenError { Ok, Empty, TooShort, InvalidStartChar, MissingHyphen, ReservedName, InvalidUtf8, InvalidChar }` | 校验失败的结构化错误枚举。 |
| `PcenValidationResult` | `class PcenValidationResult { valid: bool, err: PcenError, error_offset: usize }` | 详细校验结果，含失败原因与出错字节偏移 `error_offset`。 |
| `isValidCustomElementStart` | `fn isValidCustomElementStart(b: u8): bool` | 判断字节是否为合法的 ASCII 小写起始字符（`'a'..'z'`）。 |
| `isPcenChar` | `fn isPcenChar(cp: u32): bool` | 判断 Unicode 码点 `cp` 是否符合 WHATWG `PCENChar` 规则。 |
| `reservedCustomElementNameAt` | `fn reservedCustomElementNameAt(idx: usize): string?` | 按索引获取 WHATWG 保留名，越界返回 `null`。 |
| `isReservedCustomElementName` | `fn isReservedCustomElementName(name: string): bool` | 判断 `name` 是否命中 8 个 WHATWG 保留名之一。 |
| `validateCustomElementName` | `fn validateCustomElementName(name: string): PcenValidationResult` | 执行完整规范校验并返回错误码及精确字节位置。 |
| `isPotentialCustomElementName` | `fn isPotentialCustomElementName(name: string): bool` | 布尔校验函数，行为与 npm `is-potential-custom-element-name` 一致。 |
| `sanitizeCustomElementName` | `fn sanitizeCustomElementName(input: string, out: []u8): usize` | 将任意字符串清洗为合法的自定义元素名并写入 `out`，返回写入字节数。 |

## 使用示例

### 1. 布尔校验与错误诊断

```cp
import { isPotentialCustomElementName, validateCustomElementName, PcenError } from "is-potential-custom-element-name";

fn validateDemo(): void {
    const ok = isPotentialCustomElementName("x-card");     // true
    const bad = isPotentialCustomElementName("font-face"); // false（保留名）

    const res = validateCustomElementName("My-Widget");
    // res.valid == false, res.err == PcenError.InvalidStartChar, res.error_offset == 0
}
```

### 2. 清洗任意字符串为合法自定义元素名

```cp
import { sanitizeCustomElementName } from "is-potential-custom-element-name";

fn sanitizeDemo(): void {
    let mut out: [64]u8 = [0; 64];
    const n = sanitizeCustomElementName("123_Widget!", out[0..]);
    // out[0..n] 保证符合 WHATWG PotentialCustomElementName 规范
}
```

## 与源包差异与安全亮点

- **精确诊断定位**：`validateCustomElementName` 返回 `PcenError` 枚举与字节偏移 `error_offset`，便于编译器与框架输出精准报错。
- **零分配名称清洗**：新增 `sanitizeCustomElementName`，直接在栈缓冲区生成合规标签名。
- **无正则开销**：纯 UTF-8 解码与区间判断，彻底避免正则表达式编译和堆分配。
