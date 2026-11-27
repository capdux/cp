# @web/i18n@1.0.0 (Agent Quickref)

> Official Accept-Language / query / cookie locale negotiation & zero-heap translation plugin for cap:web | License: MIT
> Import: `import { ... } from "@web/i18n";` | Full types: `src/index.d.cp`

## Functions
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
