// Type definitions for CP

import { App, AppMethod, Context, Response, Plugin, PluginScope } from "../../../../lib/web/app.cp";
export class I18nConfig {
    default_locale: string,
    supported_locales: string,
    query_param: string,
    cookie_name: string,
}

export class I18nEntry {
    locale: string,
    key: string,
    val: string,
}

export fn i18nAsciiLower(c: u8): u8;
export fn i18nEqlIgnoreCase(a: string, b: string): bool;
export fn i18nStrEql(a: string, b: string): bool;
export fn i18nMatchLocale(supported: string, candidate: string): ?string;
export fn i18nNegotiateHeader(supported: string, accept_lang: string, fallback: string): string;
export fn i18nResetCatalog();
export fn i18nAddTranslation(locale: string, key: string, value: string): bool;
export fn i18nTranslate(locale: string, key: string): string;
export fn i18nDeriveHook(ctx: &mut Context): bool;
export fn createI18nPlugin(cfg: I18nConfig): Plugin;
export fn registerI18n(app: &mut App, cfg: I18nConfig): &mut App;
