// cap/test/testCapJsonRepair.cp
// Comprehensive unit tests for @cap/json-repair

import { test, expect_eq } from "cap:test";
import { repairJson } from "../../packages/jsonrepair/src/index.cp";

fn main() {
    test("1. @cap/json-repair — Markdown fences, unquoted keys & single quotes", () => {
        let out: [512]u8 = undefined;
        const raw = "```json\n{user: 'alice', msg: 'it\\'s \"ok\"'}\n```";
        expect_eq(repairJson(raw, out[0..512]), "{\"user\":\"alice\",\"msg\":\"it's \\\"ok\\\"\"}");
    });

    test("2. @cap/json-repair — Trailing commas & comments stripping", () => {
        let out: [512]u8 = undefined;
        const raw = "{\n  // agent config\n  \"items\": [1, 2, 3,], /* trailing */\n  \"ok\": true,\n}";
        expect_eq(repairJson(raw, out[0..512]), "{\"items\":[1,2,3],\"ok\":true}");
    });

    test("3. @cap/json-repair — Python True/False/None & JS undefined", () => {
        let out: [512]u8 = undefined;
        const raw = "{a: True, b: False, c: None, d: undefined}";
        expect_eq(repairJson(raw, out[0..512]), "{\"a\":true,\"b\":false,\"c\":null,\"d\":null}");
    });

    test("4. @cap/json-repair — Truncated LLM JSON stream auto-closing", () => {
        let out: [512]u8 = undefined;
        const raw = "{\"steps\":[{\"id\":1,\"name\":\"search\"},{\"id\":2,\"name\":\"run\"";
        expect_eq(repairJson(raw, out[0..512]), "{\"steps\":[{\"id\":1,\"name\":\"search\"},{\"id\":2,\"name\":\"run\"}]}");
    });

    test("5. @cap/json-repair — Already valid JSON preserved cleanly", () => {
        let out: [512]u8 = undefined;
        const raw = "{\"price\":19.99,\"neg\":-42,\"sci\":1.5e-3}";
        expect_eq(repairJson(raw, out[0..512]), "{\"price\":19.99,\"neg\":-42,\"sci\":1.5e-3}");
    });
}
