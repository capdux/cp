// cap/test/test_dotenv_complete.cp
// Comprehensive verification of cap:dotenv standard library
import std from "std";
import { test, expect_eq } from "cap:test";
import {
    parse,
    parseWithOptions,
    expandString,
    EnvMap,
    DotenvOptions,
    DotenvLoader,
    config,
    load,
} from "cap:dotenv";

fn test_basic_parsing_and_comments(): !void {
    const input =
        "# Top level comment\n" +
        "SIMPLE=hello\n" +
        "   TRIMMED_KEY  =  trimmed_val   \n" +
        "export EXPORTED_VAR=exported_val\n" +
        "export   ANOTHER_EXP = another\n" +
        "COLON_STYLE: colon_val\n" +
        "# Another comment\n" +
        "EMPTY_VAL=\n" +
        "WITH_INLINE_COMMENT=valid_val # this is ignored\n";

    let env = parse(input);

    expect_eq(env.count, 7);
    expect_eq(std.mem.eql(u8, env.get("SIMPLE").?, "hello"), true);
    expect_eq(std.mem.eql(u8, env.get("TRIMMED_KEY").?, "trimmed_val"), true);
    expect_eq(std.mem.eql(u8, env.get("EXPORTED_VAR").?, "exported_val"), true);
    expect_eq(std.mem.eql(u8, env.get("ANOTHER_EXP").?, "another"), true);
    expect_eq(std.mem.eql(u8, env.get("COLON_STYLE").?, "colon_val"), true);
    expect_eq(std.mem.eql(u8, env.get("EMPTY_VAL").?, ""), true);
    expect_eq(std.mem.eql(u8, env.get("WITH_INLINE_COMMENT").?, "valid_val"), true);
    expect_eq(env.has("NON_EXISTENT"), false);
    expect_eq(std.mem.eql(u8, env.getOr("NON_EXISTENT", "fallback"), "fallback"), true);
}

fn test_quote_handling_and_escapes(): !void {
    const input =
        "DOUBLE_ESCAPED=\"first line\\nsecond line\\twith tab\"\n" +
        "DOUBLE_QUOTES=\"nested \\\"quote\\\" inside\"\n" +
        "SINGLE_RAW='literal \\n not escaped'\n" +
        "BACKTICK_RAW=`backtick string`\n" +
        "SPACED_DOUBLE=\"   spaced value   \"\n";

    let env = parse(input);

    expect_eq(std.mem.eql(u8, env.get("DOUBLE_ESCAPED").?, "first line\nsecond line\twith tab"), true);
    expect_eq(std.mem.eql(u8, env.get("DOUBLE_QUOTES").?, "nested \"quote\" inside"), true);
    expect_eq(std.mem.eql(u8, env.get("SINGLE_RAW").?, "literal \\n not escaped"), true);
    expect_eq(std.mem.eql(u8, env.get("BACKTICK_RAW").?, "backtick string"), true);
    expect_eq(std.mem.eql(u8, env.get("SPACED_DOUBLE").?, "   spaced value   "), true);
}

fn test_variable_interpolation_and_defaults(): !void {
    const input =
        "USER=antigravity\n" +
        "HOST=localhost\n" +
        "PORT=5432\n" +
        "DB=production_db\n" +
        "DATABASE_URL=postgres://${USER}:${PORT}@${HOST}/${DB}\n" +
        "APP_PORT=${CUSTOM_PORT:-8080}\n" +
        "FALLBACK_USER=${UNDEFINED_USER:-guest}\n" +
        "ESCAPED_DOLLAR=\\${NOT_EXPANDED}\n";

    let env = parse(input);

    expect_eq(std.mem.eql(u8, env.get("USER").?, "antigravity"), true);
    expect_eq(std.mem.eql(u8, env.get("DATABASE_URL").?, "postgres://antigravity:5432@localhost/production_db"), true);
    expect_eq(std.mem.eql(u8, env.get("APP_PORT").?, "8080"), true);
    expect_eq(std.mem.eql(u8, env.get("FALLBACK_USER").?, "guest"), true);
    expect_eq(std.mem.eql(u8, env.get("ESCAPED_DOLLAR").?, "${NOT_EXPANDED}"), true);
}

fn test_same_file_override_rule(): !void {
    // Keys defined later in the same file override keys defined earlier
    const input =
        "SETTING=initial\n" +
        "SETTING=updated\n" +
        "SETTING=final_val\n";

    let env = parse(input);
    expect_eq(env.count, 1);
    expect_eq(std.mem.eql(u8, env.get("SETTING").?, "final_val"), true);
}

fn test_dotenv_loader_and_cascade_priority(): !void {
    let loader = DotenvLoader.init();
    expect_eq(loader.count(), 0);

    // Simulate cascade loading using loadFromString with overwrite = false
    // Priority order:
    // 1. .env.development.local (loaded first, sets HIGHEST priority)
    // 2. .env.local
    // 3. .env.development
    // 4. .env (base defaults, loaded last with overwrite = false)

    // 1. Highest priority file
    loader.loadFromString(
        "API_KEY=dev_local_secret\n" +
        "PORT=3001\n",
        false,
        true,
    );

    // 2. .env.local
    loader.loadFromString(
        "PORT=3002\n" + // Should NOT override PORT=3001
        "CACHE_DIR=/tmp/local_cache\n",
        false,
        true,
    );

    // 3. .env.development
    loader.loadFromString(
        "PORT=3000\n" + // Should NOT override
        "LOG_LEVEL=debug\n",
        false,
        true,
    );

    // 4. .env base
    loader.loadFromString(
        "PORT=80\n" + // Should NOT override
        "LOG_LEVEL=info\n" + // Should NOT override LOG_LEVEL=debug
        "APP_NAME=AntigravityApp\n",
        false,
        true,
    );

    expect_eq(std.mem.eql(u8, loader.get("API_KEY").?, "dev_local_secret"), true);
    expect_eq(std.mem.eql(u8, loader.get("PORT").?, "3001"), true);
    expect_eq(std.mem.eql(u8, loader.get("CACHE_DIR").?, "/tmp/local_cache"), true);
    expect_eq(std.mem.eql(u8, loader.get("LOG_LEVEL").?, "debug"), true);
    expect_eq(std.mem.eql(u8, loader.get("APP_NAME").?, "AntigravityApp"), true);
}

fn test_config_api(): !void {
    let loader = config(DotenvOptions{
        path: "",
        mode: "development",
        cwd: ".",
        override: false,
        expand: true,
    });
    // Valid loader returned even if no local .env files exist in working dir
    expect_eq(loader.count() >= 0, true);
}

fn main() {
    test("1. cap:dotenv — Basic key-value parsing, comments & export prefix", () => {
        test_basic_parsing_and_comments()?;
    });

    test("2. cap:dotenv — Quote handling, single raw vs double escapes", () => {
        test_quote_handling_and_escapes()?;
    });

    test("3. cap:dotenv — Variable interpolation ${VAR} and default values :-", () => {
        test_variable_interpolation_and_defaults()?;
    });

    test("4. cap:dotenv — Same-file later-key override behavior", () => {
        test_same_file_override_rule()?;
    });

    test("5. cap:dotenv — DotenvLoader cascade loading priority simulation", () => {
        test_dotenv_loader_and_cascade_priority()?;
    });

    test("6. cap:dotenv — Config API options initialization", () => {
        test_config_api()?;
    });
}
