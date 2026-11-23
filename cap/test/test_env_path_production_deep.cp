import std from "std";
import { test, expect } from "cap:test";
import { EnvPathList as EnvPath, EnvPathListIter as EnvPathIterator } from "cap:path";

test("env_path: append, trailing-slash normalization, prepend, and zero-leak ~init()", () => {
    let ep = EnvPath.initWithDelimiter(std.testing.allocator, ':');
    expect(ep.isEmpty()).toBeTrue();

    ep.append("/usr/local/bin/") catch unreachable;
    ep.append("::/usr/bin::") catch unreachable;
    ep.append("") catch unreachable;
    ep.append(":::") catch unreachable;
    expect(ep.slice()).toBe("/usr/local/bin:/usr/bin");

    ep.prepend("/opt/cap/bin/") catch unreachable;
    expect(ep.slice()).toBe("/opt/cap/bin:/usr/local/bin:/usr/bin");

    expect(ep.contains("/usr/bin")).toBeTrue();
    expect(ep.contains("/usr/bin/")).toBeTrue();
    expect(ep.contains("/nope")).toBeFalse();

    expect(ep.appendUnique("/usr/bin") catch unreachable).toBeFalse();
    expect(ep.appendUnique("/bin") catch unreachable).toBeTrue();
    expect(ep.slice()).toBe("/opt/cap/bin:/usr/local/bin:/usr/bin:/bin");
});

test("env_path: EnvPathIterator skips empty segments and handles Windows semicolon", () => {
    let it = EnvPathIterator.init(";C:\\bin;;D:\\tools;", ';');
    expect(it.next().?).toBe("C:\\bin");
    expect(it.next().?).toBe("D:\\tools");
    expect(it.next() == null).toBeTrue();
});
