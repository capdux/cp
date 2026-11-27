[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# robots-parser

Zero-allocation RFC 9309 robots.txt parser and URL allow/disallow evaluator for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:robots-parser` ([https://github.com/samclarke/robots-parser](https://github.com/samclarke/robots-parser))（原库名 `robots-parser` → CP 包名 `robots-parser`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `robots-parser` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add robots-parser
```

```cp
import {
    isSpaceOrTab,
    trimSpaces,
    startsWithIgnoreCase,
    stripBom,
    cleanLine,
    findColon,
    patMatches,
    parseDelaySeconds,
} from "robots-parser";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isSpaceOrTab` | `(c: u8): bool` | 执行 `isSpaceOrTab` 条件判定，返回 `bool`。 |
| `trimSpaces` | `(s: &[]u8): &[]u8` | 调用 `trimSpaces` 并返回 `&[]u8`。 |
| `startsWithIgnoreCase` | `(haystack: &[]u8, prefix: &[]u8): bool` | 调用 `startsWithIgnoreCase` 并返回 `bool`。 |
| `stripBom` | `(src: &[]u8): &[]u8` | 调用 `stripBom` 并返回 `&[]u8`。 |
| `cleanLine` | `(raw: &[]u8): &[]u8` | 调用 `cleanLine` 并返回 `&[]u8`。 |
| `findColon` | `(line: &[]u8): usize` | 查询或读取目标数据，返回 `usize`。 |
| `patMatches` | `(pattern: &[]u8, path: &[]u8): bool` | 调用 `patMatches` 并返回 `bool`。 |
| `parseDelaySeconds` | `(val: &[]u8): i64` | 通过 `parseDelaySeconds` 解析输入数据，返回 `i64`。 |
| `uaMatchScore` | `(ruleUa: &[]u8, targetUa: &[]u8): i64` | 调用 `uaMatchScore` 并返回 `i64`。 |
| `isAllowed` | `(robots_txt: &[]u8, user_agent: &[]u8, path: &[]u8): bool` | 执行 `isAllowed` 条件判定，返回 `bool`。 |
| `crawlDelay` | `(robots_txt: &[]u8, user_agent: &[]u8): i64` | 调用 `crawlDelay` 并返回 `i64`。 |
| `firstSitemap` | `(robots_txt: &[]u8): &[]u8` | 调用 `firstSitemap` 并返回 `&[]u8`。 |

## 最小调用示例

```cp
import { isSpaceOrTab } from "robots-parser";

pub fn main() {
    let result = isSpaceOrTab(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
