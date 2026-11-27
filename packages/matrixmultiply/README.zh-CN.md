[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# matrixmultiply

Zero-heap general dense matrix multiplication (sgemm / dgemm / matmulF64) with arbitrary row and column strides

| 元信息 | 值 |
| --- | --- |
| 包名 | `matrixmultiply` |
| 版本 | `0.3.10` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add matrixmultiply
```

```cp
import { mmulMin, sgemm, dgemm, matmulF64 } from "matrixmultiply";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `mmulMin` | `(a: usize, b: usize): usize` | 调用 `mmulMin` 并返回 `usize`。 |
| `sgemm` | `(m: usize, k: usize, n: usize, alpha: f32, a: &[]f32, rsa: usize, csa: usize, b: &[]f32, rsb: usize, csb: usize, beta: f32, c: []f32, rsc: usize, csc: usize)` | 就地执行 `sgemm` 操作。 |
| `dgemm` | `(m: usize, k: usize, n: usize, alpha: f64, a: &[]f64, rsa: usize, csa: usize, b: &[]f64, rsb: usize, csb: usize, beta: f64, c: []f64, rsc: usize, csc: usize)` | 就地执行 `dgemm` 操作。 |
| `matmulF64` | `(m: usize, k: usize, n: usize, a: &[]f64, b: &[]f64, c: []f64)` | 就地执行 `matmulF64` 操作。 |

## 最小调用示例

```cp
import { mmulMin } from "matrixmultiply";

pub fn main() {
    let result = mmulMin(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
