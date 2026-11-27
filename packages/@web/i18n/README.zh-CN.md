[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/i18n

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official Accept-Language / query / cookie locale negotiation & zero-heap translation plugin for cap:web

## 安装与导入

```bash
cap add @web/i18n
```

```cp
import { i18nAsciiLower } from "@web/i18n";
```

## 核心导出 API

- `export fn i18nAsciiLower(c: u8): u8;`
- `export fn i18nEqlIgnoreCase(a: string, b: string): bool;`
- `export fn i18nStrEql(a: string, b: string): bool;`
- `export fn i18nMatchLocale(supported: string, candidate: string): string?;`
- `export fn i18nNegotiateHeader(supported: string, accept_lang: string, fallback: string): string;`
- `export fn i18nResetCatalog();`
- `export fn i18nAddTranslation(locale: string, key: string, value: string): bool;`
- `export fn i18nTranslate(locale: string, key: string): string;`
- `export fn i18nDeriveHook(ctx: &mut Context): bool;`
- `export fn createI18nPlugin(cfg: I18nConfig): Plugin;`
- `export fn registerI18n(app: &mut App, cfg: I18nConfig): &mut App;`
