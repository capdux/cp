import { test, expect_eq } from "cap:test";
import {
    EnvPlatform,
    EnvPathsInput,
    envDataDir,
    envConfigDir,
    envCacheDir,
    envLogDir,
    envTempDir,
} from "../../packages/env-paths/src/index.cp";

fn main() {
    test("1. @cap/env-paths — Linux default XDG fallback paths", () => {
        const inp: EnvPathsInput = EnvPathsInput.linux("/home/alice");
        let buf: [256]u8 = [0; 256];

        const dlen: usize = envDataDir("myapp", inp, buf[0..256]);
        expect_eq(buf[0..dlen], "/home/alice/.local/share/myapp-cp");

        const clen: usize = envConfigDir("myapp", inp, buf[0..256]);
        expect_eq(buf[0..clen], "/home/alice/.config/myapp-cp");

        const kalen: usize = envCacheDir("myapp", inp, buf[0..256]);
        expect_eq(buf[0..kalen], "/home/alice/.cache/myapp-cp");

        const llen: usize = envLogDir("myapp", inp, buf[0..256]);
        expect_eq(buf[0..llen], "/home/alice/.local/state/myapp-cp");

        const tlen: usize = envTempDir("myapp", inp, buf[0..256]);
        expect_eq(buf[0..tlen], "/tmp/myapp-cp");
    });

    test("2. @cap/env-paths — Linux custom XDG_* overrides & trailing slash trim", () => {
        let inp: EnvPathsInput = EnvPathsInput.linux("/home/alice/");
        inp.xdg_data_home = "/mnt/data/";
        inp.xdg_config_home = "/etc/xdg-custom";
        inp.xdg_cache_home = "/var/cache/alice/";
        inp.xdg_state_home = "/var/state/alice";
        inp.tmpdir = "/run/tmp/";

        let buf: [256]u8 = [0; 256];
        expect_eq(buf[0..envDataDir("cli", inp, buf[0..256])], "/mnt/data/cli-cp");
        expect_eq(buf[0..envConfigDir("cli", inp, buf[0..256])], "/etc/xdg-custom/cli-cp");
        expect_eq(buf[0..envCacheDir("cli", inp, buf[0..256])], "/var/cache/alice/cli-cp");
        expect_eq(buf[0..envLogDir("cli", inp, buf[0..256])], "/var/state/alice/cli-cp");
        expect_eq(buf[0..envTempDir("cli", inp, buf[0..256])], "/run/tmp/cli-cp");
    });

    test("3. @cap/env-paths — macOS ~/Library conventions", () => {
        let inp: EnvPathsInput = EnvPathsInput.macos("/Users/bob");
        inp.tmpdir = "/var/folders/tmp";

        let buf: [256]u8 = [0; 256];
        expect_eq(buf[0..envDataDir("demo", inp, buf[0..256])], "/Users/bob/Library/Application Support/demo-cp");
        expect_eq(buf[0..envConfigDir("demo", inp, buf[0..256])], "/Users/bob/Library/Preferences/demo-cp");
        expect_eq(buf[0..envCacheDir("demo", inp, buf[0..256])], "/Users/bob/Library/Caches/demo-cp");
        expect_eq(buf[0..envLogDir("demo", inp, buf[0..256])], "/Users/bob/Library/Logs/demo-cp");
        expect_eq(buf[0..envTempDir("demo", inp, buf[0..256])], "/var/folders/tmp/demo-cp");
    });

    test("4. @cap/env-paths — Windows default AppData fallback", () => {
        const inp: EnvPathsInput = EnvPathsInput.windows("C:\\Users\\eve");
        let buf: [256]u8 = [0; 256];

        expect_eq(buf[0..envDataDir("tool", inp, buf[0..256])], "C:\\Users\\eve\\AppData\\Local\\tool-cp\\Data");
        expect_eq(buf[0..envConfigDir("tool", inp, buf[0..256])], "C:\\Users\\eve\\AppData\\Roaming\\tool-cp\\Config");
        expect_eq(buf[0..envCacheDir("tool", inp, buf[0..256])], "C:\\Users\\eve\\AppData\\Local\\tool-cp\\Cache");
        expect_eq(buf[0..envLogDir("tool", inp, buf[0..256])], "C:\\Users\\eve\\AppData\\Local\\tool-cp\\Log");
        expect_eq(buf[0..envTempDir("tool", inp, buf[0..256])], "C:\\Users\\eve\\AppData\\Local\\Temp\\tool-cp");
    });

    test("5. @cap/env-paths — Windows explicit LOCALAPPDATA / APPDATA / TEMP", () => {
        let inp: EnvPathsInput = EnvPathsInput.new(EnvPlatform.Windows, "C:\\Users\\eve");
        inp.local_app_data = "D:\\LocalApp\\";
        inp.app_data = "D:\\RoamingApp";
        inp.tmpdir = "E:\\Temp\\";

        let buf: [256]u8 = [0; 256];
        expect_eq(buf[0..envDataDir("app", inp, buf[0..256])], "D:\\LocalApp\\app-cp\\Data");
        expect_eq(buf[0..envConfigDir("app", inp, buf[0..256])], "D:\\RoamingApp\\app-cp\\Config");
        expect_eq(buf[0..envCacheDir("app", inp, buf[0..256])], "D:\\LocalApp\\app-cp\\Cache");
        expect_eq(buf[0..envLogDir("app", inp, buf[0..256])], "D:\\LocalApp\\app-cp\\Log");
        expect_eq(buf[0..envTempDir("app", inp, buf[0..256])], "E:\\Temp\\app-cp");
    });

    test("6. @cap/env-paths — empty suffix & small output buffer clamping", () => {
        let inp: EnvPathsInput = EnvPathsInput.linux("/home/u");
        inp.suffix = "";

        let buf: [64]u8 = [0; 64];
        const n: usize = envConfigDir("pkg", inp, buf[0..64]);
        expect_eq(buf[0..n], "/home/u/.config/pkg");

        let tiny: [10]u8 = [0; 10];
        const tn: usize = envConfigDir("pkg", inp, tiny[0..10]);
        expect_eq(tn, 10 as usize);
        expect_eq(tiny[0..10], "/home/u/.c");
    });
}
