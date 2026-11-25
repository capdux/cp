// Regression Test Suite for `@web/i18n`
import { App, AppMethod, Context, Request, Response } from "../../../../lib/web/app.cp";
import {
    I18nConfig,
    registerI18n,
    i18nAddTranslation,
    i18nTranslate,
    i18nResetCatalog,
} from "../src/index.cp";

fn strEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn hGreet(ctx: *Context): Response {
    const loc = ctx.derived("locale") ?? "en";
    return Response.text(200, i18nTranslate(loc, "hello"));
}

fn main() {
    i18nResetCatalog();
    * = i18nAddTranslation("en", "hello", "Hello");
    * = i18nAddTranslation("zh", "hello", "NiHao");
    * = i18nAddTranslation("ja", "hello", "Konnichiwa");

    let app = App.init();
    * = registerI18n(*app, I18nConfig{ default_locale: "en", supported_locales: "en,zh,ja" });
    * = app.get("/greet", hGreet);

    // 1. Accept-Language header negotiation ("zh-CN,zh;q=0.9,en;q=0.8" -> "zh")
    let r1_req = Request.init(AppMethod.GET, "/greet");
    * = r1_req.withHeader("Accept-Language", "zh-CN,zh;q=0.9,en;q=0.8");
    const r1 = app.dispatch(r1_req);
    if (!strEql(r1.body, "NiHao") || !strEql(r1.header("Content-Language") ?? "", "zh")) {
        @panic("Test 1 failed: Accept-Language zh negotiation");
    }

    // 2. Query ?lang=ja overrides Accept-Language
    let r2_req = Request.init(AppMethod.GET, "/greet?lang=ja");
    * = r2_req.withHeader("Accept-Language", "zh-CN");
    const r2 = app.dispatch(r2_req);
    if (!strEql(r2.body, "Konnichiwa") || !strEql(r2.header("Content-Language") ?? "", "ja")) {
        @panic("Test 2 failed: query ?lang override");
    }

    // 3. Fallback to default_locale when unsupported
    let r3_req = Request.init(AppMethod.GET, "/greet");
    * = r3_req.withHeader("Accept-Language", "pt-BR");
    const r3 = app.dispatch(r3_req);
    if (!strEql(r3.body, "Hello") || !strEql(r3.header("Content-Language") ?? "", "en")) {
        @panic("Test 3 failed: fallback to en");
    }

    @print("PASS @web/i18n (3/3 tests)");
}
