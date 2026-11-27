[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# cesu8

Zero-heap CESU-8 and Java Modified UTF-8 (MUTF-8) to/from standard UTF-8 transcoder

| 元信息 | 值 |
| --- | --- |
| 包名 | `cesu8` |
| 版本 | `1.1.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add cesu8
```

```cp
import { toCesu8, fromCesu8, isValidCesu8AsciiOrBmp } from "cesu8";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `toCesu8` | `(utf8: &[]u8, modified_nul: bool, out: []u8): i64` | 调用 `toCesu8` 并返回 `i64`。 |
| `fromCesu8` | `(cesu: &[]u8, modified_nul: bool, out: []u8): i64` | 调用 `fromCesu8` 并返回 `i64`。 |
| `isValidCesu8AsciiOrBmp` | `(utf8: &[]u8, modified_nul: bool): bool` | 执行 `isValidCesu8AsciiOrBmp` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isValidCesu8AsciiOrBmp } from "cesu8";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isValidCesu8AsciiOrBmp(out_buf[0..256], false);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
