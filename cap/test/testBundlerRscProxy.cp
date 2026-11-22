import { test, expect } from "cap:test";
import {
    RSC_PROXY_MAX_EXPORTS,
    RscTaskKind,
    RscTargetPlatform,
    RscFrameworkConfig,
    RscProxyModuleMeta,
    rscTargetForTask,
    formatRscProdModuleKey,
    formatRscServerCallErrorMsg,
    formatClientEntryWrapper,
    RscProxyGenerator,
} from "cap:bundler";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    const limit = haystack.len - needle.len + 1;
    for (i of 0..limit) {
        let ok = true;
        for (j of 0..needle.len) {
            if (haystack[i + j] != needle[j]) {
                ok = false;
                break;
            }
        }
        if (ok) return true;
    }
    return false;
}

test("Dev-mode ClientReferenceProxy generates exact default and named registerClientReference calls", () => {
    let gen = RscProxyGenerator.init(
        RscFrameworkConfig{
            is_dev_server: true,
            server_runtime_import: "react-server-dom-webpack/server",
            server_register_client_reference: "registerClientReference",
        },
        RscProxyModuleMeta{
            pretty_path: "src/Counter.tsx",
            source_index: 7,
        },
    );
    * = gen.addExport("default") catch unreachable;
    * = gen.addExport("CounterButton") catch unreachable;
    expect(gen.exportSlice().len == 2);

    let buf: [2048]u8 = [0; 2048];
    const n = gen.generateProxyCode(buf[0..]) catch unreachable;
    const code = buf[0..n];

    expect(containsSubstr(code, "import { registerClientReference } from \"react-server-dom-webpack/server\";\n"));
    expect(containsSubstr(code, "export default registerClientReference(() => { throw new Error(\"Attempted to call the default export of src/Counter.tsx from the server"));
    expect(containsSubstr(code, ", \"src/Counter.tsx\", \"default\");\n"));
    expect(containsSubstr(code, "export const CounterButton = registerClientReference(() => { throw new Error(\"Attempted to call CounterButton() from the server but CounterButton is on the client."));
    expect(containsSubstr(code, ", \"src/Counter.tsx\", \"CounterButton\");\n"));
});

test("Prod-mode module key formatting and proxy code replacement placeholder", () => {
    let kbuf: [64]u8 = [0; 64];
    expect(strEq(kbuf[0..(formatRscProdModuleKey(0, 0, kbuf[0..]) catch unreachable)], "0S00000000"));
    expect(strEq(kbuf[0..(formatRscProdModuleKey(0x1a2b, 42, kbuf[0..]) catch unreachable)], "1a2bS00000042"));
    expect(strEq(kbuf[0..(formatRscProdModuleKey(0xdeadbeef, 123456789, kbuf[0..]) catch unreachable)], "deadbeefS123456789"));

    let gen = RscProxyGenerator.init(
        RscFrameworkConfig{
            is_dev_server: false,
            bundle_unique_key: 0xabcd,
        },
        RscProxyModuleMeta{
            pretty_path: "src/Modal.tsx",
            source_index: 99,
        },
    );
    * = gen.addExport("Modal") catch unreachable;

    let buf: [1024]u8 = [0; 1024];
    const n = gen.generateProxyCode(buf[0..]) catch unreachable;
    const code = buf[0..n];
    expect(containsSubstr(code, ", \"abcdS00000099\", \"Modal\");\n"));
});

test("ClientEntryWrapper bare import formatting and target platform mapping", () => {
    expect(rscTargetForTask(.ClientReferenceProxy) == .Server);
    expect(rscTargetForTask(.ClientEntryWrapper) == .Browser);

    let buf: [128]u8 = [0; 128];
    const n1 = formatClientEntryWrapper("./entry-client.tsx", buf[0..]) catch unreachable;
    expect(strEq(buf[0..n1], "import \"./entry-client.tsx\";\n"));

    // Escapes quotes and backslashes safely
    const n2 = formatClientEntryWrapper("dir\\entry\"1.tsx", buf[0..]) catch unreachable;
    expect(strEq(buf[0..n2], "import \"dir\\\\entry\\\"1.tsx\";\n"));
});

test("formatRscServerCallErrorMsg produces exact React Flight diagnostics", () => {
    let buf: [512]u8 = [0; 512];
    const n_def = formatRscServerCallErrorMsg("app/Client.tsx", "default", buf[0..]) catch unreachable;
    expect(strEq(
        buf[0..n_def],
        "Attempted to call the default export of app/Client.tsx from the server, but it's on the client. It's not possible to invoke a client function from the server, it can only be rendered as a Component or passed to props of a Client Component.",
    ));

    const n_named = formatRscServerCallErrorMsg("app/Client.tsx", "useWidget", buf[0..]) catch unreachable;
    expect(strEq(
        buf[0..n_named],
        "Attempted to call useWidget() from the server but useWidget is on the client. It's not possible to invoke a client function from the server, it can only be rendered as a Component or passed to props of a Client Component.",
    ));
});

test("Custom framework runtime import/register function and zero-export module", () => {
    let gen = RscProxyGenerator.init(
        RscFrameworkConfig{
            is_dev_server: true,
            server_runtime_import: "@acme/rsc-runtime/server",
            server_register_client_reference: "__regClient",
        },
        RscProxyModuleMeta{
            pretty_path: "src/EmptyClient.tsx",
            source_index: 1,
        },
    );

    let buf: [256]u8 = [0; 256];
    const n = gen.generateProxyCode(buf[0..]) catch unreachable;
    expect(strEq(buf[0..n], "import { __regClient } from \"@acme/rsc-runtime/server\";\n"));
});

test("Boundary guards: duplicate/invalid exports, missing config, capacity, and tiny buffers", () => {
    let gen = RscProxyGenerator.init(
        RscFrameworkConfig{ is_dev_server: true },
        RscProxyModuleMeta{ pretty_path: "src/A.tsx", source_index: 1 },
    );

    // Empty export rejected
    let saw_empty = false;
    * = gen.addExport("") catch {
        saw_empty = true;
    };
    expect(saw_empty);

    // Invalid JS ident rejected
    let saw_invalid = false;
    * = gen.addExport("123bad-ident") catch {
        saw_invalid = true;
    };
    expect(saw_invalid);

    // Duplicate export rejected
    * = gen.addExport("Foo") catch unreachable;
    let saw_dup = false;
    * = gen.addExport("Foo") catch {
        saw_dup = true;
    };
    expect(saw_dup);

    // Tiny output buffer rejected
    let tiny: [8]u8 = [0; 8];
    let saw_small = false;
    * = gen.generateProxyCode(tiny[0..]) catch {
        saw_small = true;
    };
    expect(saw_small);

    // Empty runtime / register fn / module path rejected
    let bad_rt = RscProxyGenerator.init(
        RscFrameworkConfig{ server_runtime_import: "" },
        RscProxyModuleMeta{ pretty_path: "a.tsx" },
    );
    let saw_bad_rt = false;
    * = bad_rt.generateProxyCode(tiny[0..]) catch {
        saw_bad_rt = true;
    };
    expect(saw_bad_rt);

    let bad_path = RscProxyGenerator.init(
        RscFrameworkConfig{},
        RscProxyModuleMeta{ pretty_path: "" },
    );
    let saw_bad_path = false;
    * = bad_path.generateProxyCode(tiny[0..]) catch {
        saw_bad_path = true;
    };
    expect(saw_bad_path);

    let saw_empty_wrapper = false;
    * = formatClientEntryWrapper("", tiny[0..]) catch {
        saw_empty_wrapper = true;
    };
    expect(saw_empty_wrapper);

    // Capacity guard
    let cap_gen = RscProxyGenerator.init(
        RscFrameworkConfig{ is_dev_server: true },
        RscProxyModuleMeta{ pretty_path: "src/Many.tsx" },
    );
    const names: [64][]const u8 = [
        "e00", "e01", "e02", "e03", "e04", "e05", "e06", "e07",
        "e08", "e09", "e10", "e11", "e12", "e13", "e14", "e15",
        "e16", "e17", "e18", "e19", "e20", "e21", "e22", "e23",
        "e24", "e25", "e26", "e27", "e28", "e29", "e30", "e31",
        "e32", "e33", "e34", "e35", "e36", "e37", "e38", "e39",
        "e40", "e41", "e42", "e43", "e44", "e45", "e46", "e47",
        "e48", "e49", "e50", "e51", "e52", "e53", "e54", "e55",
        "e56", "e57", "e58", "e59", "e60", "e61", "e62", "e63",
    ];
    for (i of 0..RSC_PROXY_MAX_EXPORTS) {
        * = cap_gen.addExport(names[i]) catch unreachable;
    }
    let saw_overflow = false;
    * = cap_gen.addExport("e64") catch {
        saw_overflow = true;
    };
    expect(saw_overflow);
});
