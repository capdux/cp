import { test, expect_eq } from "cap:test";
import {
    LogLevel,
    FilterDirective,
    EnvFilter,
    parseLogLevel,
} from "../../packages/@cap/env-filter/src/index.cp";

fn main() {
    test("1. @cap/env-filter — parseLogLevel case-insensitive & invalid", () => {
        expect_eq(parseLogLevel("off") == LogLevel.Off, true);
        expect_eq(parseLogLevel("ERROR") == LogLevel.Error, true);
        expect_eq(parseLogLevel("WaRn") == LogLevel.Warn, true);
        expect_eq(parseLogLevel("  info  ") == LogLevel.Info, true);
        expect_eq(parseLogLevel("Debug") == LogLevel.Debug, true);
        expect_eq(parseLogLevel("TRACE") == LogLevel.Trace, true);
        expect_eq(parseLogLevel("verbose") == null, true);
        expect_eq(parseLogLevel("") == null, true);
    });

    test("2. @cap/env-filter — hierarchical RUST_LOG spec & longest-prefix match", () => {
        let f = EnvFilter.parse("warn,my_app=info,my_app::net=debug,quiet_mod=off");
        expect_eq(f.directiveCount(), 4);

        expect_eq(f.maxLevelFor("my_app::net::tcp") == LogLevel.Debug, true);
        expect_eq(f.enabled("my_app::net::tcp", LogLevel.Debug), true);
        expect_eq(f.enabled("my_app::net::tcp", LogLevel.Trace), false);

        expect_eq(f.maxLevelFor("my_app::db") == LogLevel.Info, true);
        expect_eq(f.enabled("my_app::db", LogLevel.Info), true);
        expect_eq(f.enabled("my_app::db", LogLevel.Debug), false);

        expect_eq(f.maxLevelFor("quiet_mod") == LogLevel.Off, true);
        expect_eq(f.enabled("quiet_mod::inner", LogLevel.Error), false);

        expect_eq(f.maxLevelFor("third_party::http") == LogLevel.Warn, true);
        expect_eq(f.enabled("third_party::http", LogLevel.Warn), true);
        expect_eq(f.enabled("third_party::http", LogLevel.Info), false);
    });

    test("3. @cap/env-filter — :: boundary alignment prevents false prefix match", () => {
        let f = EnvFilter.parse("error,my_app=trace");
        expect_eq(f.maxLevelFor("my_app") == LogLevel.Trace, true);
        expect_eq(f.maxLevelFor("my_app::worker") == LogLevel.Trace, true);
        expect_eq(f.maxLevelFor("my_app2") == LogLevel.Error, true);
        expect_eq(f.maxLevelFor("my_application::core") == LogLevel.Error, true);
        expect_eq(f.maxLevelFor("my_app:single") == LogLevel.Error, true);
    });

    test("4. @cap/env-filter — bare module name defaults to Trace & whitespace tolerance", () => {
        let f = EnvFilter.parse("  my_crate::parser , , other=WARN , ");
        expect_eq(f.directiveCount(), 2);
        expect_eq(f.maxLevelFor("my_crate::parser") == LogLevel.Trace, true);
        expect_eq(f.enabled("my_crate::parser::lex", LogLevel.Trace), true);
        expect_eq(f.maxLevelFor("other") == LogLevel.Warn, true);
        expect_eq(f.maxLevelFor("unrelated") == LogLevel.Off, true);
        expect_eq(f.enabled("unrelated", LogLevel.Error), false);
    });

    test("5. @cap/env-filter — duplicate target overrides previous level", () => {
        let f = EnvFilter.parse("warn,my_app=debug,info,my_app=error");
        expect_eq(f.directiveCount(), 2);
        expect_eq(f.maxLevelFor("my_app") == LogLevel.Error, true);
        expect_eq(f.maxLevelFor("anything_else") == LogLevel.Info, true);
        expect_eq(f.enabled("anything_else", LogLevel.Off), false);
    });

    test("6. @cap/env-filter — programmatic addDirective & descending order inspection", () => {
        let f = EnvFilter.empty();
        expect_eq(f.directiveCount(), 0);
        expect_eq(f.addDirective("a", LogLevel.Warn), true);
        expect_eq(f.addDirective("a::b::c", LogLevel.Trace), true);
        expect_eq(f.addDirective("a::b", LogLevel.Info), true);
        expect_eq(f.directiveCount(), 3);

        if (d0 := f.getDirective(0)) {
            expect_eq(d0.target.len, 7);
            expect_eq(d0.level == LogLevel.Trace, true);
        } else {
            expect_eq(true, false);
        }
        expect_eq(f.maxLevelFor("a::b::c::d") == LogLevel.Trace, true);
        expect_eq(f.maxLevelFor("a::b::x") == LogLevel.Info, true);
        expect_eq(f.maxLevelFor("a::z") == LogLevel.Warn, true);
    });
}
