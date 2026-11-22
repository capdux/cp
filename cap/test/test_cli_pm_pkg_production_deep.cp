import { test, expect } from "cap:test";
import {
    PmPkgSubCommand,
    PmPkgKeyPath,
    PmPkgAssignment,
    classifyPmPkgValue,
} from "cap:cli";

fn eqBytes(a: []const u8, b: []const u8): bool {
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

test("cli pm_pkg: subcommand parsing and keyword mapping") {
    if (c := PmPkgSubCommand.fromString("get")) { expect(c == .get && eqBytes(c.asKeyword(), "get")); } else { expect(false); }
    if (c := PmPkgSubCommand.fromString("set")) { expect(c == .set && eqBytes(c.asKeyword(), "set")); } else { expect(false); }
    if (c := PmPkgSubCommand.fromString("delete")) { expect(c == .delete && eqBytes(c.asKeyword(), "delete")); } else { expect(false); }
    if (c := PmPkgSubCommand.fromString("fix")) { expect(c == .fix && eqBytes(c.asKeyword(), "fix")); } else { expect(false); }
    expect(PmPkgSubCommand.fromString("unknown") == null);
    expect(PmPkgSubCommand.fromString("") == null);
}

test("cli pm_pkg: dot, numeric index, bracket and quoted key path parsing") {
    const p1 = PmPkgKeyPath.parse("scripts.build")?;
    expect(p1.count() == 2);
    if (s0 := p1.get(0)) { expect(s0.kind == .property && eqBytes(s0.key, "scripts")); } else { expect(false); }
    if (s1 := p1.get(1)) { expect(s1.kind == .property && eqBytes(s1.key, "build")); } else { expect(false); }

    const p2 = PmPkgKeyPath.parse("contributors[0].name")?;
    expect(p2.count() == 3);
    if (s1 := p2.get(1)) { expect(s1.kind == .array_index && s1.index == 0 && s1.from_bracket); } else { expect(false); }
    if (s2 := p2.get(2)) { expect(s2.kind == .property && eqBytes(s2.key, "name")); } else { expect(false); }

    // workspaces.0 等价于 workspaces[0]
    const p3a = PmPkgKeyPath.parse("workspaces.0")?;
    const p3b = PmPkgKeyPath.parse("workspaces[0]")?;
    expect(p3a.eql(p3b));

    // 特殊字符与带点引号键
    const p4 = PmPkgKeyPath.parse("scripts[test:watch]")?;
    expect(p4.count() == 2);
    if (s1 := p4.get(1)) { expect(s1.kind == .property && eqBytes(s1.key, "test:watch")); } else { expect(false); }

    const p5 = PmPkgKeyPath.parse("exports[\"./sub.mod\"].import")?;
    expect(p5.count() == 3);
    if (s1 := p5.get(1)) { expect(s1.kind == .property && eqBytes(s1.key, "./sub.mod")); } else { expect(false); }

    // 多维下标
    const p6 = PmPkgKeyPath.parse("matrix[0][12].label")?;
    expect(p6.count() == 4);
    if (s2 := p6.get(2)) { expect(s2.kind == .array_index && s2.index == 12); } else { expect(false); }
    expect(p6.get(4) == null);
}

test("cli pm_pkg: assignment parsing with embedded equals and special bracket keys") {
    const a1 = PmPkgAssignment.parse("scripts.test=jest")?;
    expect(eqBytes(a1.key, "scripts.test"));
    expect(eqBytes(a1.value, "jest"));
    expect(a1.path.count() == 2);

    // 右侧值包含 '=' 时只按首个 '=' 拆分
    const a2 = PmPkgAssignment.parse("scripts[env:dev]=NODE_ENV=development bun run dev")?;
    expect(eqBytes(a2.key, "scripts[env:dev]"));
    expect(eqBytes(a2.value, "NODE_ENV=development bun run dev"));
    expect(a2.path.count() == 2);

    // 允许空值赋值（清空字符串字段）
    const a3 = PmPkgAssignment.parse("description=")?;
    expect(eqBytes(a3.key, "description"));
    expect(a3.value.len == 0);
}

test("cli pm_pkg: value classification in raw and --json modes") {
    expect((classifyPmPkgValue("true", false)?) == .raw_string);
    expect((classifyPmPkgValue("anything goes", false)?) == .raw_string);

    expect((classifyPmPkgValue("null", true)?) == .json_null);
    expect((classifyPmPkgValue("true", true)?) == .json_bool);
    expect((classifyPmPkgValue("false", true)?) == .json_bool);
    expect((classifyPmPkgValue("42", true)?) == .json_number);
    expect((classifyPmPkgValue("-3.14e+2", true)?) == .json_number);
    expect((classifyPmPkgValue("\"hello\"", true)?) == .json_string);
    expect((classifyPmPkgValue("[1, 2, 3]", true)?) == .json_array);
    expect((classifyPmPkgValue("{\"private\": true}", true)?) == .json_object);

    // --json 模式下拒绝畸形 JSON
    let bad_json: usize = 0;
    * = classifyPmPkgValue("", true) catch { bad_json += 1; .raw_string; };
    * = classifyPmPkgValue("01", true) catch { bad_json += 1; .raw_string; };
    * = classifyPmPkgValue("1.", true) catch { bad_json += 1; .raw_string; };
    * = classifyPmPkgValue("tru", true) catch { bad_json += 1; .raw_string; };
    expect(bad_json == 4);
}

test("cli pm_pkg: malformed paths and assignment guards") {
    let rejected: usize = 0;

    * = PmPkgKeyPath.parse("") catch { rejected += 1; PmPkgKeyPath.empty(); };
    * = PmPkgKeyPath.parse(".leading") catch { rejected += 1; PmPkgKeyPath.empty(); };
    * = PmPkgKeyPath.parse("trailing.") catch { rejected += 1; PmPkgKeyPath.empty(); };
    * = PmPkgKeyPath.parse("a..b") catch { rejected += 1; PmPkgKeyPath.empty(); };
    * = PmPkgKeyPath.parse("a[]") catch { rejected += 1; PmPkgKeyPath.empty(); };
    * = PmPkgKeyPath.parse("a[0") catch { rejected += 1; PmPkgKeyPath.empty(); };
    * = PmPkgKeyPath.parse("a[\"unterminated]") catch { rejected += 1; PmPkgKeyPath.empty(); };
    * = PmPkgKeyPath.parse("a]b") catch { rejected += 1; PmPkgKeyPath.empty(); };
    * = PmPkgKeyPath.parse("a[[0]]") catch { rejected += 1; PmPkgKeyPath.empty(); };
    * = PmPkgKeyPath.parse("a[0]extra") catch { rejected += 1; PmPkgKeyPath.empty(); };

    * = PmPkgAssignment.parse("no-equals-sign") catch { rejected += 1; PmPkgAssignment.parse("a=b")?; };
    * = PmPkgAssignment.parse("=missing-key") catch { rejected += 1; PmPkgAssignment.parse("a=b")?; };

    expect(rejected == 12);
}

test("cli pm_pkg: formatCanonical round-trip idempotency and buffer guard") {
    const p1 = PmPkgKeyPath.parse("exports[\"./sub.mod\"].targets[0].scripts[test:watch]")?;
    let buf: [128]u8 = [0; 128];
    const n1 = p1.formatCanonical(buf[0..])?;
    expect(eqBytes(buf[0..n1], "exports[\"./sub.mod\"].targets[0].scripts[test:watch]"));

    const p2 = PmPkgKeyPath.parse(buf[0..n1])?;
    expect(p1.eql(p2));

    let buf2: [128]u8 = [0; 128];
    const n2 = p2.formatCanonical(buf2[0..])?;
    expect(eqBytes(buf[0..n1], buf2[0..n2]));

    // 缓冲区不足防护
    let tiny: [8]u8 = [0; 8];
    const small_res = p1.formatCanonical(tiny[0..]) catch {
        expect(true);
        return;
    };
    * = small_res;
    expect(false);
}
