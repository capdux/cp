import std from "std";
import { test, expect } from "cap:test";
import { Param, ParamMap, URLPath, parseURLPath } from "cap:route_params";

test("route_params: ParamMap CRUD, overwrite, clear, and zero-leak ~init()", () => {
    let map = ParamMap.init(std.testing.allocator);
    expect(map.isEmpty()).toBeTrue();

    map.append("id", "42") catch unreachable;
    map.append("slug", "hello-cp") catch unreachable;
    expect(map.count()).toBe(2);
    expect(map.has("id")).toBeTrue();
    expect(map.find("id").?).toBe("42");
    expect(map.find("slug").?).toBe("hello-cp");
    expect(map.find("missing") == null).toBeTrue();

    map.set("id", "99") catch unreachable;
    expect(map.count()).toBe(2);
    expect(map.find("id").?).toBe("99");

    const p1 = map.get(1).?;
    expect(p1.name).toBe("slug");
    expect(p1.value).toBe("hello-cp");
    expect(map.get(5) == null).toBeTrue();

    map.clear();
    expect(map.isEmpty()).toBeTrue();
});

test("route_params: parseURLPath standard paths, root, query, and source maps", () => {
    const r = parseURLPath("/");
    expect(r.path).toBe(".");
    expect(r.isRoot("")).toBeTrue();
    expect(r.extname).toBe("");
    expect(r.query_string).toBe("");

    const p1 = parseURLPath("/api/v1/users.json?limit=10&sort=asc");
    expect(p1.first_segment).toBe("api");
    expect(p1.path).toBe("api/v1/users.json");
    expect(p1.extname).toBe("json");
    expect(p1.query_string).toBe("?limit=10&sort=asc");
    expect(p1.is_source_map).toBeFalse();

    const p2 = parseURLPath("/static/assets/bundle.js.map?hash=abc");
    expect(p2.is_source_map).toBeTrue();
    expect(p2.extname).toBe("js");
    expect(p2.path).toBe("static/assets/bundle.js");
    expect(p2.pathWithoutAssetPrefix("/static/")).toBe("assets/bundle.js");
    expect(p2.query_string).toBe("?hash=abc");
});

test("route_params: parseURLPath edge cases (dot in dir, empty)", () => {
    const p = parseURLPath("/v1.2/items");
    expect(p.extname).toBe("");
    expect(p.first_segment).toBe("v1.2");
    expect(p.path).toBe("v1.2/items");

    const empty = parseURLPath("");
    expect(empty.path).toBe("");
    expect(empty.extname).toBe("");
});
