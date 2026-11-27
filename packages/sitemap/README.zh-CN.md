[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# sitemap

Zero-heap Sitemaps.org 0.9 XML sitemap & sitemapindex generator and validator in pure CP (modeled after npm sitemap)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:sitemap` ([https://github.com/ekalinin/sitemap.js](https://github.com/ekalinin/sitemap.js))

| 元信息 | 值 |
| --- | --- |
| 包名 | `sitemap` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add sitemap
```

```cp
import {
    SitemapUrl,
    SitemapWriter,
    SitemapIndexWriter,
    freqName,
    isValidPriorityTenths,
} from "sitemap";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SitemapUrl` | `class` | 字段: `loc: &[]u8, lastmod: &[]u8, changefreq: u8, priority_tenths: i8` |
| `SitemapWriter` | `class` | 字段: `buf: []u8, pos: usize, url_count: usize, closed: bool, overflow: bool` · 方法: `init, writeRaw, writeXmlEscaped, addUrl, finish, urlCount, +2 项` |
| `SitemapIndexWriter` | `class` | 字段: `buf: []u8, pos: usize, entry_count: usize, closed: bool, overflow: bool` · 方法: `init, writeRaw, writeXmlEscaped, addSitemap, finish, entryCount, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `freqName` | `(f: u8): &[]u8` | 调用 `freqName` 并返回 `&[]u8`。 |
| `isValidPriorityTenths` | `(p: i8): bool` | 执行 `isValidPriorityTenths` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isValidPriorityTenths } from "sitemap";

pub fn main() {
    let result = isValidPriorityTenths(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
