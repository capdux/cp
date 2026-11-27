import { test, expect_eq } from "cap:test";
import {
    MAP_OBJ_MAX_ENTRIES, MapObjResult,
    swapKeysAndValues, pickKeys, omitKeys, filterEmptyValues, mergeObjects
} from "../../packages/map-obj/src/index.cp";

fn main() {
    test("1. @cap/map-obj — MapObjResult push & get basics", () => {
        let obj = MapObjResult.empty();
        expect_eq(obj.push("host", "localhost"), true);
        expect_eq(obj.push("port", "8080"), true);
        expect_eq(obj.count, 2);
        expect_eq(obj.get("host") ?? "", "localhost");
        expect_eq(obj.get("missing") == null, true);
    });

    test("2. @cap/map-obj — swapKeysAndValues inverts entry mapping", () => {
        let obj = MapObjResult.empty();
        * = obj.push("a", "1");
        * = obj.push("b", "2");
        let inv = swapKeysAndValues(obj);
        expect_eq(inv.get("1") ?? "", "a");
        expect_eq(inv.get("2") ?? "", "b");
    });

    test("3. @cap/map-obj — pickKeys retains only whitelisted keys", () => {
        let obj = MapObjResult.empty();
        * = obj.push("a", "1");
        * = obj.push("b", "2");
        * = obj.push("c", "3");
        let allow: [2]string = ["a", "c"];
        let picked = pickKeys(obj, allow[0..2]);
        expect_eq(picked.count, 2);
        expect_eq(picked.get("a") ?? "", "1");
        expect_eq(picked.get("b") == null, true);
        expect_eq(picked.get("c") ?? "", "3");
    });

    test("4. @cap/map-obj — omitKeys strips excluded keys", () => {
        let obj = MapObjResult.empty();
        * = obj.push("token", "secret");
        * = obj.push("user", "alice");
        let skip: [1]string = ["token"];
        let cleaned = omitKeys(obj, skip[0..1]);
        expect_eq(cleaned.count, 1);
        expect_eq(cleaned.get("token") == null, true);
        expect_eq(cleaned.get("user") ?? "", "alice");
    });

    test("5. @cap/map-obj — filterEmptyValues removes empty-string entries", () => {
        let obj = MapObjResult.empty();
        * = obj.push("k1", "ok");
        * = obj.push("k2", "");
        * = obj.push("k3", "yes");
        let filtered = filterEmptyValues(obj);
        expect_eq(filtered.count, 2);
        expect_eq(filtered.get("k2") == null, true);
    });

    test("6. @cap/map-obj — mergeObjects overrides existing keys & appends new ones", () => {
        let a = MapObjResult.empty();
        * = a.push("env", "dev");
        * = a.push("debug", "true");
        let b = MapObjResult.empty();
        * = b.push("env", "prod");
        * = b.push("region", "ap");
        let m = mergeObjects(a, b);
        expect_eq(m.count, 3);
        expect_eq(m.get("env") ?? "", "prod");
        expect_eq(m.get("debug") ?? "", "true");
        expect_eq(m.get("region") ?? "", "ap");
        expect_eq(MAP_OBJ_MAX_ENTRIES, 32);
    });
}
