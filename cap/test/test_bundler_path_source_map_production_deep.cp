import { test, expect, expect_eq } from "cap:test";
import {
    PATH_SRC_MAP_BUCKETS,
    PATH_SRC_MAP_MAX_LOAD,
    PATH_SRC_INVALID_INDEX,
    BundlerModulePath,
    PathToSourceIndexMap,
} from "cap:bundler";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("bundler path_source_map: basic put, get, overwrite & reverseLookup", fn() {
    let map = PathToSourceIndexMap.init();
    expect(map.isEmpty());
    expect_eq(map.count(), 0);

    const ov1 = map.put("src/index.ts", 1) catch {
        expect(false);
        return;
    };
    expect(!ov1);
    expect_eq(map.count(), 1);
    expect(map.contains("src/index.ts"));

    if (v := map.get("src/index.ts")) {
        expect_eq(v, 1);
    } else {
        expect(false);
    }

    // 覆写已有键不增加计数
    const ov2 = map.put("src/index.ts", 42) catch {
        expect(false);
        return;
    };
    expect(ov2);
    expect_eq(map.count(), 1);
    if (v2 := map.get("src/index.ts")) {
        expect_eq(v2, 42);
    } else {
        expect(false);
    }

    if (rev := map.reverseLookup(42)) {
        expect(rev.isFileNamespace());
        expect(strEq(rev.text, "src/index.ts"));
    } else {
        expect(false);
    }
    expect(map.reverseLookup(999) == null);
});

test("bundler path_source_map: plugin namespace isolation & formatting", fn() {
    let map = PathToSourceIndexMap.init();
    const p_file = BundlerModulePath.file("config.json");
    const p_virt = BundlerModulePath.withNamespace("virtual", "config.json");
    const p_env = BundlerModulePath.withNamespace("env", "config.json");

    expect(p_file.isFileNamespace());
    expect(!p_file.isVirtual());
    expect(p_virt.isVirtual());
    expect(!p_file.eql(p_virt));
    expect(!p_virt.eql(p_env));

    * = map.putPath(p_file, 10) catch {
        expect(false);
        return;
    };
    * = map.putPath(p_virt, 20) catch {
        expect(false);
        return;
    };
    * = map.putPath(p_env, 30) catch {
        expect(false);
        return;
    };
    expect_eq(map.count(), 3);

    if (v1 := map.getPath(p_file)) {
        expect_eq(v1, 10);
    } else {
        expect(false);
    }
    if (v2 := map.getPath(p_virt)) {
        expect_eq(v2, 20);
    } else {
        expect(false);
    }
    if (v3 := map.getPath(p_env)) {
        expect_eq(v3, 30);
    } else {
        expect(false);
    }

    let buf: [64]u8 = [0; 64];
    const n1 = p_file.formatInto(buf[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(buf[0..n1], "config.json"));

    const n2 = p_virt.formatInto(buf[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(buf[0..n2], "virtual:config.json"));
});

test("bundler path_source_map: atomic getOrPut & getOrPutPath semantics", fn() {
    let map = PathToSourceIndexMap.init();
    const r1 = map.getOrPut("src/app.tsx", 1) catch {
        expect(false);
        return;
    };
    expect(!r1.found_existing);
    expect_eq(r1.source_index, 1);

    const r2 = map.getOrPut("src/app.tsx", 99) catch {
        expect(false);
        return;
    };
    expect(r2.found_existing);
    expect_eq(r2.source_index, 1);
    expect_eq(map.count(), 1);

    const virt = BundlerModulePath.withNamespace("bun", "sqlite");
    const r3 = map.getOrPutPath(virt, 2) catch {
        expect(false);
        return;
    };
    expect(!r3.found_existing);
    expect_eq(r3.source_index, 2);
    expect_eq(map.count(), 2);
});

test("bundler path_source_map: tombstone-safe remove, re-insert & clear", fn() {
    let map = PathToSourceIndexMap.init();
    * = map.put("a.ts", 1) catch {
        expect(false);
        return;
    };
    * = map.put("b.ts", 2) catch {
        expect(false);
        return;
    };
    * = map.put("c.ts", 3) catch {
        expect(false);
        return;
    };

    expect(map.remove("b.ts"));
    expect(!map.remove("b.ts"));
    expect_eq(map.count(), 2);
    expect(map.get("b.ts") == null);
    // 确认其余键探测链完整
    if (va := map.get("a.ts")) {
        expect_eq(va, 1);
    } else {
        expect(false);
    }
    if (vc := map.get("c.ts")) {
        expect_eq(vc, 3);
    } else {
        expect(false);
    }

    // 复用墓碑槽位重新插入
    * = map.put("b.ts", 22) catch {
        expect(false);
        return;
    };
    expect_eq(map.count(), 3);
    if (vb := map.get("b.ts")) {
        expect_eq(vb, 22);
    } else {
        expect(false);
    }

    map.clear();
    expect(map.isEmpty());
    expect(map.get("a.ts") == null);
});

test("bundler path_source_map: empty path & buffer-too-small error guards", fn() {
    let map = PathToSourceIndexMap.init();
    let ok1 = true;
    * = map.put("", 1) catch {
        ok1 = false;
    };
    expect(!ok1);

    let ok2 = true;
    * = map.getOrPut("", 1) catch {
        ok2 = false;
    };
    expect(!ok2);

    expect(map.get("") == null);
    expect(!map.remove(""));

    const p = BundlerModulePath.withNamespace("virtual", "very-long-module-name.ts");
    let tiny: [5]u8 = [0; 5];
    let ok_fmt = true;
    * = p.formatInto(tiny[0..]) catch {
        ok_fmt = false;
    };
    expect(!ok_fmt);
});

test("bundler path_source_map: constants & max-load capacity guard", fn() {
    expect_eq(PATH_SRC_MAP_BUCKETS, 256);
    expect_eq(PATH_SRC_MAP_MAX_LOAD, 192);
    expect_eq(PATH_SRC_INVALID_INDEX, 0xFFFF_FFFF);

    let map = PathToSourceIndexMap.init();
    // 构造不同长度切片填满 PATH_SRC_MAP_MAX_LOAD (192)
    let raw: [256]u8 = [0; 256];
    let k: usize = 0;
    while (k < 256) {
        raw[k] = 'a';
        k += 1;
    }

    let i: usize = 0;
    while (i < PATH_SRC_MAP_MAX_LOAD) {
        const key_slice = raw[0 .. i + 1];
        * = map.put(key_slice, @intCast(i)) catch {
            expect(false);
            return;
        };
        i += 1;
    }
    expect_eq(map.count(), PATH_SRC_MAP_MAX_LOAD);

    // 超过最大负载返回 MapFull
    let ok_full = true;
    * = map.put(raw[0 .. PATH_SRC_MAP_MAX_LOAD + 1], 999) catch {
        ok_full = false;
    };
    expect(!ok_full);
});
