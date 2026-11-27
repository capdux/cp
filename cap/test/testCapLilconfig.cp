import { test, expect_eq } from "cap:test";
import {
    LOADER_UNKNOWN,
    LOADER_JSON,
    LOADER_YAML,
    LOADER_JS,
    LOADER_PKG_JSON
} from "../../packages/cosmiconfig/src/index.cp";
import {
    LIL_DEFAULT_PLACE_COUNT,
    LilconfigOptions,
    LilExplorer,
    formatLilSearchPlace,
    isSupportedByDefault
} from "../../packages/lilconfig/src/index.cp";

fn main() {
    test("lilconfig: formatLilSearchPlace produces 6 compact places without YAML", () => {
        let buf: [64]u8 = [0 as u8; 64];
        expect_eq(LIL_DEFAULT_PLACE_COUNT, 6 as usize);

        let n0 = formatLilSearchPlace("postcss", 0, buf[0..64]);
        let s0: string = buf[0..n0];
        expect_eq(s0, "package.json");

        let n1 = formatLilSearchPlace("postcss", 1, buf[0..64]);
        let s1: string = buf[0..n1];
        expect_eq(s1, ".postcssrc");

        let n2 = formatLilSearchPlace("postcss", 2, buf[0..64]);
        let s2: string = buf[0..n2];
        expect_eq(s2, ".postcssrc.json");

        let n3 = formatLilSearchPlace("postcss", 3, buf[0..64]);
        let s3: string = buf[0..n3];
        expect_eq(s3, ".postcssrc.js");

        let n4 = formatLilSearchPlace("postcss", 4, buf[0..64]);
        let s4: string = buf[0..n4];
        expect_eq(s4, ".postcssrc.cjs");

        let n5 = formatLilSearchPlace("postcss", 5, buf[0..64]);
        let s5: string = buf[0..n5];
        expect_eq(s5, "postcss.config.js");

        expect_eq(formatLilSearchPlace("postcss", 6, buf[0..64]), 0 as usize);
    });

    test("lilconfig: isSupportedByDefault allows JSON/JS/pkg/noExt and rejects YAML", () => {
        expect_eq(isSupportedByDefault("/app/package.json"), true);
        expect_eq(isSupportedByDefault("/app/.postcssrc.json"), true);
        expect_eq(isSupportedByDefault("/app/.postcssrc"), true);
        expect_eq(isSupportedByDefault("/app/postcss.config.js"), true);
        expect_eq(isSupportedByDefault("/app/postcss.config.cjs"), true);
        expect_eq(isSupportedByDefault("/app/.postcssrc.yaml"), false);
        expect_eq(isSupportedByDefault("/app/.postcssrc.yml"), false);
        expect_eq(isSupportedByDefault("/app/config.toml"), false);
    });

    test("lilconfig: LilExplorer resolves built-in loaders and rejects unregistered YAML", () => {
        let opts = LilconfigOptions.defaults("stylelint");
        let explorer = LilExplorer.new("/repo/src", opts);

        expect_eq(explorer.resolveLoader("/repo/package.json"), LOADER_PKG_JSON);
        expect_eq(explorer.resolveLoader("/repo/.stylelintrc.json"), LOADER_JSON);
        expect_eq(explorer.resolveLoader("/repo/.stylelintrc"), LOADER_JSON);
        expect_eq(explorer.resolveLoader("/repo/stylelint.config.js"), LOADER_JS);
        expect_eq(explorer.resolveLoader("/repo/.stylelintrc.yaml"), LOADER_UNKNOWN);
        expect_eq(explorer.resolveLoader("/repo/.stylelintrc.yml"), LOADER_UNKNOWN);
    });

    test("lilconfig: LilExplorer supports custom loader registration and override", () => {
        let opts = LilconfigOptions.defaults("myapp");
        let explorer = LilExplorer.new("/repo/src", opts);

        expect_eq(explorer.registerLoader(".yaml", LOADER_YAML), true);
        expect_eq(explorer.registerLoader(".toml", 10 as u8), true);
        expect_eq(explorer.registerLoader("noExt", LOADER_YAML), true);

        expect_eq(explorer.resolveLoader("/repo/.myapprc.yaml"), LOADER_YAML);
        expect_eq(explorer.resolveLoader("/repo/myapp.toml"), 10 as u8);
        expect_eq(explorer.resolveLoader("/repo/.myapprc"), LOADER_YAML);

        // Overwriting existing extension updates loader_id without consuming a new slot
        expect_eq(explorer.registerLoader(".toml", 11 as u8), true);
        expect_eq(explorer.custom_count, 3 as usize);
        expect_eq(explorer.resolveLoader("/repo/myapp.toml"), 11 as u8);
    });

    test("lilconfig: LilExplorer delegates upward traversal to cosmiconfig SearchPlanner", () => {
        let opts = LilconfigOptions.defaults("tailwind");
        opts.stop_dir = "/workspace";
        let explorer = LilExplorer.new("/workspace/apps/docs", opts);

        expect_eq(explorer.nextSearchDir().?, "/workspace/apps/docs");
        expect_eq(explorer.nextSearchDir().?, "/workspace/apps");
        expect_eq(explorer.nextSearchDir().?, "/workspace");
        expect_eq(explorer.nextSearchDir() == null, true);
    });

    test("lilconfig: candidatePath and makeResult reuse cosmiconfig helpers", () => {
        let opts = LilconfigOptions.defaults("babel");
        let explorer = LilExplorer.new("/proj/sub", opts);
        let buf: [128]u8 = [0 as u8; 128];

        let n = explorer.candidatePath("/proj/sub", 5, buf[0..128]);
        let path: string = buf[0..n];
        expect_eq(path, "/proj/sub/babel.config.js");

        let res = explorer.makeResult(path, false);
        expect_eq(res.found, true);
        expect_eq(res.is_empty, false);
        expect_eq(res.loader_kind, LOADER_JS);
    });
}
