[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# pkg-dir

Zero-heap Node.js/CP package root directory finder locating nearest ancestor package.json or cap.json (npm pkg-dir equivalent, reuses find-up & npm-run-path)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pkg-dir` ([https://github.com/sindresorhus/pkg-dir](https://github.com/sindresorhus/pkg-dir))

| 元信息 | 值 |
| --- | --- |
| 包名 | `pkg-dir` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add pkg-dir
```

```cp
import { pkdPush, packageDirectoryInList, capPackageDirectoryInList, packageDirectoryBounded } from "pkg-dir";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pkdPush` | `(out: []u8, pos: usize, s: string): usize` | 调用 `pkdPush` 并返回 `usize`。 |
| `packageDirectoryInList` | `(cwd: string, existing_files: []string, out: []u8): usize?` | 调用 `packageDirectoryInList` 并返回 `usize?`。 |
| `capPackageDirectoryInList` | `(cwd: string, existing_files: []string, out: []u8): usize?` | 调用 `capPackageDirectoryInList` 并返回 `usize?`。 |
| `packageDirectoryBounded` | `(cwd: string, stop_at: string, existing_files: []string, out: []u8): usize?` | 调用 `packageDirectoryBounded` 并返回 `usize?`。 |

## 最小调用示例

```cp
import { pkdPush } from "pkg-dir";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = pkdPush(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
