[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/i18n

Official Accept-Language / query / cookie locale negotiation & zero-heap translation plugin for cap:web

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/i18n` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/i18n
```

```cp
import {
    I18nConfig,
    I18nEntry,
    i18nAsciiLower,
    i18nEqlIgnoreCase,
    i18nStrEql,
    i18nMatchLocale,
    i18nNegotiateHeader,
    i18nResetCatalog,
} from "@web/i18n";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `I18nConfig` | `class` | 字段: `default_locale: string, supported_locales: string, query_param: string, cookie_name: string` |
| `I18nEntry` | `class` | 字段: `locale: string, key: string, val: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `i18nAsciiLower` | `(c: u8): u8` | 调用 `i18nAsciiLower` 并返回 `u8`。 |
| `i18nEqlIgnoreCase` | `(a: string, b: string): bool` | 调用 `i18nEqlIgnoreCase` 并返回 `bool`。 |
| `i18nStrEql` | `(a: string, b: string): bool` | 调用 `i18nStrEql` 并返回 `bool`。 |
| `i18nMatchLocale` | `(supported: string, candidate: string): string?` | 调用 `i18nMatchLocale` 并返回 `string?`。 |
| `i18nNegotiateHeader` | `(supported: string, accept_lang: string, fallback: string): string` | 调用 `i18nNegotiateHeader` 并返回 `string`。 |
| `i18nResetCatalog` | `()` | 就地执行 `i18nResetCatalog` 操作。 |
| `i18nAddTranslation` | `(locale: string, key: string, value: string): bool` | 调用 `i18nAddTranslation` 并返回 `bool`。 |
| `i18nTranslate` | `(locale: string, key: string): string` | 调用 `i18nTranslate` 并返回 `string`。 |
| `i18nDeriveHook` | `(ctx: &mut Context): bool` | 调用 `i18nDeriveHook` 并返回 `bool`。 |
| `createI18nPlugin` | `(cfg: I18nConfig): Plugin` | 创建或初始化实例，返回 `Plugin`。 |
| `registerI18n` | `(app: &mut App, cfg: I18nConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { i18nEqlIgnoreCase } from "@web/i18n";

pub fn main() {
    let result = i18nEqlIgnoreCase("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
