// cap/test/test_conf_complete.cp
// Comprehensive test suite verifying 100% complete and industrial-grade configuration interchange in cap:conf

import { test, expect_eq } from "cap:test";
import { Ini, ini } from "cap:ini";
import { TomlDoc, toml, TOML } from "cap:conf";

fn main() {
    test("1. INI complete suite: sections, quotes, escapes, types, introspection, serializer", () => {
        const sample_ini = "; INI Production Config\napp_name = CPApp\ndebug = true\nworkers = 8\ntimeout_sec = 2.5\n\n[server]\nhost = 0.0.0.0\nport = 8080\nenable_tls = yes\n\n[server.database]\nurl = \"postgres://user:pass@localhost:5432/db\"\nmax_pool = 50\nping_ms = 12.5\nactive = on\n\n[security]\nsecret_key = 'raw_unquoted_secret$key'\ngreeting = \"Line1\\nLine2\\tTabbed\"\n";

        let conf = Ini.parse(sample_ini);

        // Global keys
        expect_eq(conf.getGlobal("app_name") ?? "", "CPApp");
        expect_eq(conf.getBool("", "debug", false), true);
        expect_eq(conf.getInt("", "workers", 0) as i64, 8);
        expect_eq(conf.getFloat("", "timeout_sec", 0.0) > 2.0, true);

        // Nested section [server.database]
        expect_eq(conf.hasSection("server.database"), true);
        expect_eq(conf.get("server.database", "url") ?? "", "postgres://user:pass@localhost:5432/db");
        expect_eq(conf.getInt("server.database", "max_pool", 0) as i64, 50);
        expect_eq(conf.getBool("server.database", "active", false), true);

        // Quoted string escapes & raw single-quotes
        expect_eq(conf.get("security", "greeting") ?? "", "Line1\nLine2\tTabbed");
        expect_eq(conf.get("security", "secret_key") ?? "", "raw_unquoted_secret$key");

        // Introspection: getSections & getKeys
        let sections = conf.getSections();
        expect_eq(sections.len as i64, 3);

        let server_keys = conf.getKeys("server");
        expect_eq(server_keys.len as i64, 3);

        // Mutation: set, remove, removeSection
        conf.set("cache", "driver", "redis");
        expect_eq(conf.get("cache", "driver") ?? "", "redis");
        expect_eq(conf.hasSection("cache"), true);

        expect_eq(conf.remove("cache", "driver"), true);
        expect_eq(conf.get("cache", "driver") == null, true);

        expect_eq(conf.removeSection("cache"), false); // already empty

        // Serialization and round-trip
        let serialized = conf.toString();
        let re_parsed = Ini.parse(serialized);
        expect_eq(re_parsed.getGlobal("app_name") ?? "", "CPApp");
        expect_eq(re_parsed.get("server", "port") ?? "", "8080");
        expect_eq(re_parsed.getInt("server.database", "max_pool", 0) as i64, 50);
    });

    test("2. TOML complete suite: tables, dotted keys, scalars (hex/bin/oct/float), arrays, serializer", () => {
        const sample_toml = "# TOML Industrial Config\ntitle = \"TOML Spec Suite\"\nversion = 2\n\n[owner]\nname = \"Tom Preston-Werner\"\nactive = true\n\n[database]\nenabled = true\nports = [ 8001, 8002, 8003 ]\ntags = [ \"primary\", \"replica\" ]\nmax_connections = 1_000\nhex_val = 0xDEAD\noct_val = 0o755\nbin_val = 0b1010\npi = 3.14159\n\n[database.connection]\nhost = \"127.0.0.1\"\ntimeout = 30.5\n";

        let doc = TomlDoc.parse(sample_toml);

        // Root properties
        expect_eq(doc.getRoot("title") ?? "", "TOML Spec Suite");
        expect_eq(doc.getInt("", "version", 0) as i64, 2);

        // Standard table & booleans
        expect_eq(doc.hasTable("owner"), true);
        expect_eq(doc.get("owner", "name") ?? "", "Tom Preston-Werner");
        expect_eq(doc.getBool("owner", "active", false), true);

        // Numbers: decimal with underscore, hex, oct, bin, float
        expect_eq(doc.getInt("database", "max_connections", 0) as i64, 1000);
        expect_eq(doc.getInt("database", "hex_val", 0) as i64, 57005); // 0xDEAD = 57005
        expect_eq(doc.getInt("database", "oct_val", 0) as i64, 493);   // 0o755 = 493
        expect_eq(doc.getInt("database", "bin_val", 0) as i64, 10);    // 0b1010 = 10
        expect_eq(doc.getFloat("database", "pi", 0.0) > 3.14, true);

        // Array parsing
        let tags = doc.getArray("database", "tags");
        expect_eq(tags.len as i64, 2);
        expect_eq(tags[0], "primary");
        expect_eq(tags[1], "replica");

        let ports = doc.getArray("database", "ports");
        expect_eq(ports.len as i64, 3);
        expect_eq(ports[0], "8001");
        expect_eq(ports[1], "8002");
        expect_eq(ports[2], "8003");

        // Dotted nested table [database.connection]
        expect_eq(doc.hasTable("database.connection"), true);
        expect_eq(doc.get("database.connection", "host") ?? "", "127.0.0.1");
        expect_eq(doc.getFloat("database.connection", "timeout", 0.0) > 30.0, true);

        // Mutation & Introspection
        doc.set("analytics", "endpoint", "https://telemetry.org");
        expect_eq(doc.get("analytics", "endpoint") ?? "", "https://telemetry.org");
        expect_eq(doc.hasTable("analytics"), true);

        let tables = doc.getTables();
        expect_eq(tables.len as i64, 4);

        // Roundtrip serialization
        let toml_str = doc.toString();
        let re_parsed = TomlDoc.parse(toml_str);
        expect_eq(re_parsed.getRoot("title") ?? "", "TOML Spec Suite");
        expect_eq(re_parsed.getInt("database", "max_connections", 0) as i64, 1000);
        expect_eq(re_parsed.get("analytics", "endpoint") ?? "", "https://telemetry.org");
    });
}
