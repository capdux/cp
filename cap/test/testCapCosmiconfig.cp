import { test, expect_eq } from "cap:test";
import {
    LOADER_UNKNOWN,
    LOADER_JSON,
    LOADER_YAML,
    LOADER_JS,
    LOADER_PKG_JSON,
    DEFAULT_SEARCH_PLACE_COUNT,
    ExplorerOptions,
    SearchPlanner,
    CosmiConfigResult,
    detectLoader,
    formatSearchPlace,
    buildCandidatePath
} from "../../packages/cosmiconfig/src/index.cp";

fn main() {
    test("cosmiconfig: detectLoader classifies package.json, json, yaml, js, and rc files", () => {
        expect_eq(detectLoader("/app/package.json"), LOADER_PKG_JSON);
        expect_eq(detectLoader("/app/.prettierrc.json"), LOADER_JSON);
        expect_eq(detectLoader("/app/.prettierrc.yaml"), LOADER_YAML);
        expect_eq(detectLoader("/app/.prettierrc.yml"), LOADER_YAML);
        expect_eq(detectLoader("/app/.prettierrc"), LOADER_YAML);
        expect_eq(detectLoader("/app/.config/apprc"), LOADER_YAML);
        expect_eq(detectLoader("/app/app.config.js"), LOADER_JS);
        expect_eq(detectLoader("/app/app.config.cjs"), LOADER_JS);
        expect_eq(detectLoader("/app/app.config.mjs"), LOADER_JS);
        expect_eq(detectLoader("/app/config.toml"), LOADER_UNKNOWN);
    });

    test("cosmiconfig: formatSearchPlace generates all 8 standard search places in order", () => {
        let buf: [64]u8 = [0 as u8; 64];
        expect_eq(DEFAULT_SEARCH_PLACE_COUNT, 8 as usize);

        let n0 = formatSearchPlace("eslint", 0, buf[0..64]);
        let s0: string = buf[0..n0];
        expect_eq(s0, "package.json");

        let n1 = formatSearchPlace("eslint", 1, buf[0..64]);
        let s1: string = buf[0..n1];
        expect_eq(s1, ".eslintrc");

        let n2 = formatSearchPlace("eslint", 2, buf[0..64]);
        let s2: string = buf[0..n2];
        expect_eq(s2, ".eslintrc.json");

        let n3 = formatSearchPlace("eslint", 3, buf[0..64]);
        let s3: string = buf[0..n3];
        expect_eq(s3, ".eslintrc.yaml");

        let n4 = formatSearchPlace("eslint", 4, buf[0..64]);
        let s4: string = buf[0..n4];
        expect_eq(s4, ".eslintrc.yml");

        let n5 = formatSearchPlace("eslint", 5, buf[0..64]);
        let s5: string = buf[0..n5];
        expect_eq(s5, ".config/eslintrc");

        let n6 = formatSearchPlace("eslint", 6, buf[0..64]);
        let s6: string = buf[0..n6];
        expect_eq(s6, "eslint.config.js");

        let n7 = formatSearchPlace("eslint", 7, buf[0..64]);
        let s7: string = buf[0..n7];
        expect_eq(s7, "eslint.config.cjs");

        expect_eq(formatSearchPlace("eslint", 8, buf[0..64]), 0 as usize);
    });

    test("cosmiconfig: buildCandidatePath joins directories and root slash cleanly", () => {
        let buf: [128]u8 = [0 as u8; 128];

        let n1 = buildCandidatePath("/home/user/proj/", ".apprc.json", buf[0..128]);
        let p1: string = buf[0..n1];
        expect_eq(p1, "/home/user/proj/.apprc.json");

        let n2 = buildCandidatePath("/", "package.json", buf[0..128]);
        let p2: string = buf[0..n2];
        expect_eq(p2, "/package.json");

        let n3 = buildCandidatePath("/repo", ".config/apprc", buf[0..128]);
        let p3: string = buf[0..n3];
        expect_eq(p3, "/repo/.config/apprc");
    });

    test("cosmiconfig: SearchPlanner walks upward and stops at stop_dir", () => {
        let planner = SearchPlanner.new("/workspace/apps/web/src", "/workspace/apps");
        expect_eq(planner.dirCount(), 3 as usize);

        let d1 = planner.nextDir();
        expect_eq(d1 != null, true);
        expect_eq(d1.?, "/workspace/apps/web/src");

        let d2 = planner.nextDir();
        expect_eq(d2 != null, true);
        expect_eq(d2.?, "/workspace/apps/web");

        let d3 = planner.nextDir();
        expect_eq(d3 != null, true);
        expect_eq(d3.?, "/workspace/apps");

        expect_eq(planner.nextDir() == null, true);
    });

    test("cosmiconfig: SearchPlanner reaches root '/' when stop_dir is root", () => {
        let opts = ExplorerOptions.defaults("prettier");
        expect_eq(opts.module_name, "prettier");
        expect_eq(opts.package_prop, "prettier");
        expect_eq(opts.stop_dir, "/");
        expect_eq(opts.ignore_empty, true);

        let planner = SearchPlanner.new("/a/b/", opts.stop_dir);
        expect_eq(planner.dirCount(), 3 as usize);

        expect_eq(planner.nextDir().?, "/a/b");
        expect_eq(planner.nextDir().?, "/a");
        expect_eq(planner.nextDir().?, "/");
        expect_eq(planner.nextDir() == null, true);
    });

    test("cosmiconfig: CosmiConfigResult constructors record hit and notFound states", () => {
        let miss = CosmiConfigResult.notFound();
        expect_eq(miss.found, false);
        expect_eq(miss.is_empty, true);
        expect_eq(miss.loader_kind, LOADER_UNKNOWN);

        let hit = CosmiConfigResult.hit("/repo/myapp.config.cjs", false);
        expect_eq(hit.found, true);
        expect_eq(hit.is_empty, false);
        expect_eq(hit.filepath, "/repo/myapp.config.cjs");
        expect_eq(hit.loader_kind, LOADER_JS);
    });
}
