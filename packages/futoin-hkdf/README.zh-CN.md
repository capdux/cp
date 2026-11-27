[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# futoin-hkdf

Pure CP zero-allocation RFC 5869 HKDF-SHA256 key derivation function (RustCrypto hkdf parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:futoin-hkdf` ([https://github.com/futoin/util-js-hkdf](https://github.com/futoin/util-js-hkdf))（原库名 `futoin-hkdf` → CP 包名 `futoin-hkdf`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `futoin-hkdf` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add futoin-hkdf
```

```cp
import { hkdfStreamingHmac, hkdfExtract, hkdfExpand, hkdfDerive } from "futoin-hkdf";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `hkdfStreamingHmac` | `(prk32: []u8, prev_t: []u8, info: []u8, counter: u8, out32: []u8)` | 就地执行 `hkdfStreamingHmac` 操作。 |
| `hkdfExtract` | `(salt: []u8, ikm: []u8, out_prk32: []u8): bool` | 调用 `hkdfExtract` 并返回 `bool`。 |
| `hkdfExpand` | `(prk32: []u8, info: []u8, out_okm: []u8): bool` | 调用 `hkdfExpand` 并返回 `bool`。 |
| `hkdfDerive` | `(salt: []u8, ikm: []u8, info: []u8, out_okm: []u8): bool` | 调用 `hkdfDerive` 并返回 `bool`。 |

## 最小调用示例

```cp
import { hkdfExtract } from "futoin-hkdf";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let mut buf_2: [256]u8 = undefined;
    let result = hkdfExtract(out_buf[0..256], buf_1[0..256], buf_2[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
