[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ecdsa-sig-formatter

Zero-allocation ASN.1 DER <-> JOSE IEEE P1363 (r || s) ECDSA signature converter for ES256, ES384, and ES512 in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ecdsa-sig-formatter` ([https://github.com/Brightspace/node-ecdsa-sig-formatter](https://github.com/Brightspace/node-ecdsa-sig-formatter), `Apache-2.0`)

| 元信息 | 值 |
| --- | --- |
| 包名 | `ecdsa-sig-formatter` |
| 版本 | `1.0.0` |
| 许可证 | `Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ecdsa-sig-formatter
```

```cp
import {
    EcdsaAlg,
    paramBytes,
    esfParseDerInt,
    esfTrimLeadingZeros,
    derToJose,
    joseToDer,
} from "ecdsa-sig-formatter";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `EcdsaAlg` | `ES256, ES384, ES512` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `paramBytes` | `(alg: EcdsaAlg): usize` | 调用 `paramBytes` 并返回 `usize`。 |
| `esfParseDerInt` | `(der: &[]u8, pos: usize, limit: usize, param_len: usize, dst: []u8): i64` | 调用 `esfParseDerInt` 并返回 `i64`。 |
| `esfTrimLeadingZeros` | `(src: &[]u8): usize` | 调用 `esfTrimLeadingZeros` 并返回 `usize`。 |
| `derToJose` | `(der: &[]u8, alg: EcdsaAlg, out: []u8): i64` | 调用 `derToJose` 并返回 `i64`。 |
| `joseToDer` | `(jose: &[]u8, alg: EcdsaAlg, out: []u8): i64` | 调用 `joseToDer` 并返回 `i64`。 |

## 最小调用示例

```cp
import { paramBytes } from "ecdsa-sig-formatter";

pub fn main() {
    let result = paramBytes(/* alg: EcdsaAlg */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
