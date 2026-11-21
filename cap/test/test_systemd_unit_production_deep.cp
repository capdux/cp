// Source: cp/cap/test/test_systemd_unit_production_deep.cp (Production deep tests for cap:systemd_unit)

import { test, expect_eq } from "cap:test";
import {
    UnitEntry,
    UnitIter,
    getUnitValue,
    countSectionKey,
} from "cap:systemd_unit";

test("systemd_unit: full .service parsing with BOM, #/; comments, sections and backslash continuation") {
    const svc =
        "\xEF\xBB\xBF" ++
        "# Cap daemon service\n" ++
        "; Semicolon comment\n" ++
        "[Unit]\n" ++
        "Description=Cap Agent Runtime\n" ++
        "After=network.target\n" ++
        "\n" ++
        "[Service]\n" ++
        "Type=simple\n" ++
        "ExecStart=/usr/bin/cap \\\n" ++
        "# comment inside continuation\n" ++
        "  run \\\n" ++
        "  --port 8080\n" ++
        "Environment=NODE_ENV=production\n" ++
        "Environment=LOG_LEVEL=info\n" ++
        "[Install]\n" ++
        "WantedBy=multi-user.target\n";

    let it = UnitIter.init(svc);
    let buf: [256]u8 = [0; 256];

    const e1 = it.next(buf[0..]).?;
    expect_eq(std.mem.eql(u8, e1.section, "Unit"), true);
    expect_eq(std.mem.eql(u8, e1.key, "Description"), true);
    expect_eq(std.mem.eql(u8, e1.value, "Cap Agent Runtime"), true);
    expect_eq(e1.line_no, 4);

    const e2 = it.next(buf[0..]).?;
    expect_eq(std.mem.eql(u8, e2.key, "After"), true);

    const e3 = it.next(buf[0..]).?;
    expect_eq(std.mem.eql(u8, e3.section, "Service"), true);
    expect_eq(std.mem.eql(u8, e3.key, "Type"), true);
    expect_eq(std.mem.eql(u8, e3.value, "simple"), true);

    const e4 = it.next(buf[0..]).?;
    expect_eq(std.mem.eql(u8, e4.key, "ExecStart"), true);
    expect_eq(std.mem.eql(u8, e4.value, "/usr/bin/cap run --port 8080"), true);
    expect_eq(e4.line_no, 9);

    const e5 = it.next(buf[0..]).?;
    expect_eq(std.mem.eql(u8, e5.key, "Environment"), true);
    expect_eq(std.mem.eql(u8, e5.value, "NODE_ENV=production"), true);

    const e6 = it.next(buf[0..]).?;
    expect_eq(std.mem.eql(u8, e6.key, "Environment"), true);
    expect_eq(std.mem.eql(u8, e6.value, "LOG_LEVEL=info"), true);

    const e7 = it.next(buf[0..]).?;
    expect_eq(std.mem.eql(u8, e7.section, "Install"), true);
    expect_eq(std.mem.eql(u8, e7.key, "WantedBy"), true);
    expect_eq(std.mem.eql(u8, e7.value, "multi-user.target"), true);

    expect_eq(it.next(buf[0..]) == null, true);
}

test("systemd_unit: getUnitValue override semantics and empty Key= reset semantics") {
    const unit =
        "[Service]\n" ++
        "Restart=on-failure\n" ++
        "Restart=always\n" ++
        "ExecStart=/old/bin\n" ++
        "ExecStart=\n" ++
        "ExecStart=/new/bin --fast\n" ++
        "ClearedKey=initial\n" ++
        "ClearedKey=\n";

    let buf: [128]u8 = [0; 128];

    // 后者覆盖前者
    const restart = getUnitValue(unit, "Service", "Restart", buf[0..]).?;
    expect_eq(std.mem.eql(u8, restart, "always"), true);

    // 清空后再赋新值
    const exec = getUnitValue(unit, "Service", "ExecStart", buf[0..]).?;
    expect_eq(std.mem.eql(u8, exec, "/new/bin --fast"), true);

    // 最终以 Key= 清空返回 null
    expect_eq(getUnitValue(unit, "Service", "ClearedKey", buf[0..]) == null, true);

    // 大小写敏感与不存在段/键返回 null
    expect_eq(getUnitValue(unit, "service", "Restart", buf[0..]) == null, true);
    expect_eq(getUnitValue(unit, "Service", "Missing", buf[0..]) == null, true);
}

test("systemd_unit: countSectionKey counts repeated directives across multiple sections") {
    const unit =
        "[Service]\n" ++
        "ExecStartPre=/bin/prep1\n" ++
        "ExecStartPre=/bin/prep2\n" ++
        "Environment=A=1\n" ++
        "Environment=B=2\n" ++
        "Environment=C=3\n" ++
        "[Unit]\n" ++
        "Environment=IGNORED_OTHER_SECTION\n";

    expect_eq(countSectionKey(unit, "Service", "ExecStartPre"), 2);
    expect_eq(countSectionKey(unit, "Service", "Environment"), 3);
    expect_eq(countSectionKey(unit, "Unit", "Environment"), 1);
    expect_eq(countSectionKey(unit, "Install", "WantedBy"), 0);
}

test("systemd_unit: CRLF line endings and values containing '=' preserved") {
    const unit = "[Service]\r\nEnvironment=DSN=postgres://u:p@h/db?sslmode=require\r\n";
    let buf: [128]u8 = [0; 128];
    const v = getUnitValue(unit, "Service", "Environment", buf[0..]).?;
    expect_eq(std.mem.eql(u8, v, "DSN=postgres://u:p@h/db?sslmode=require"), true);
}

test("systemd_unit: malformed lines skipped and small output buffer overflow guard") {
    const unit =
        "[Service]\n" ++
        "NO_EQUALS_LINE\n" ++
        "=EMPTY_KEY\n" ++
        "User=www-data\n";

    let buf: [64]u8 = [0; 64];
    expect_eq(std.mem.eql(u8, getUnitValue(unit, "Service", "User", buf[0..]).?, "www-data"), true);
    expect_eq(countSectionKey(unit, "Service", "User"), 1);

    let tiny: [3]u8 = [0; 3];
    expect_eq(getUnitValue(unit, "Service", "User", tiny[0..]) == null, true);
}
