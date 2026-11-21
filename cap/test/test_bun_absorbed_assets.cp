// cap/test/test_bun_absorbed_assets.cp
// Comprehensive end-to-end integration test suite for Bun absorbed assets across standard library:
// - cap:string (smol, hashed, path, joiner)
// - cap:sync (guarded, work_pool)
// - cap:event (any_task, managed_task)
// - cap:sql (sqlite_database, sqlite_rows)
// - cap:shell (lexer, parser, exec, builtins, pipelines)

import { test, expect_eq } from "cap:test";
import { SmolStr, HashedString, PathString, StringJoiner } from "cap:string";
import { Guarded, WorkPool, TaskDesc } from "cap:sync";
import { AnyTask, ManagedTask } from "cap:event";
import { SqliteDatabase } from "cap:sql";
import { Lexer, Parser, sh } from "cap:shell";

fn testTaskCallback(ctx: *!?) {
    zig {
        if (ctx) |c| {
            const ptr: *i32 = @ptrCast(@alignCast(c));
            ptr.* += 10;
        }
    }
}

fn main() {
    test("1. cap:string — SmolStr stack SSO & heap fallback", () => {
        // Small string (<= 15 bytes): stays inlined on stack
        let s1 = SmolStr.init("hello world");
        expect_eq(s1.len() as i64, 11);
        expect_eq(s1.isInline(), true);
        expect_eq(s1.eql("hello world"), true);
        expect_eq(s1.eql("other"), false);
        s1.~init();

        // Large string (> 15 bytes): heap allocated
        let s2 = SmolStr.init("this is a longer string that exceeds 15 bytes");
        expect_eq(s2.len() as i64, 45);
        expect_eq(s2.isInline(), false);
        expect_eq(s2.eql("this is a longer string that exceeds 15 bytes"), true);
        s2.~init();
    });

    test("2. cap:string — HashedString, PathString & StringJoiner", () => {
        // HashedString O(1) comparison
        let h1 = HashedString.init("authorization");
        let h2 = HashedString.init("authorization");
        let h3 = HashedString.init("content-type");
        expect_eq(h1.eql(h2), true);
        expect_eq(h1.eql(h3), false);
        expect_eq(h1.len() as i64, 13);

        // PathString
        let path_obj = PathString.init("/data/dev/cp/lib/shell/index.cp");
        expect_eq(path_obj.isAbsolute(), true);
        expect_eq(path_obj.hasExtension(".cp"), true);
        expect_eq(path_obj.hasExtension(".zig"), false);

        let path_rel = PathString.init("src/main.rs");
        expect_eq(path_rel.isAbsolute(), false);
        expect_eq(path_rel.hasExtension(".rs"), true);

        // StringJoiner
        let joiner = StringJoiner.init();
        joiner.push("alpha");
        joiner.pushDelimited("beta", ", ");
        joiner.pushDelimited("gamma", ", ");
        expect_eq(joiner.len() as i64, 18);
        joiner.~init();
    });

    test("3. cap:sync — Guarded<T> thread-safe wrapper & WorkPool queue", () => {
        let g = Guarded<i32>.init(42);
        expect_eq(g.get() as i64, 42);
        g.set(100);
        expect_eq(g.get() as i64, 100);

        let pool = WorkPool.init();
        expect_eq(pool.queueCount() as i64, 0);
        expect_eq(pool.isRunning(), true);

        let task1 = TaskDesc {
            func: undefined,
            ctx: undefined,
            next: null,
        };
        let task2 = TaskDesc {
            func: undefined,
            ctx: undefined,
            next: null,
        };
        pool.schedule(&task1);
        pool.schedule(&task2);
        expect_eq(pool.queueCount() as i64, 2);

        if (polled1 := pool.pollTask()) {
            expect_eq(pool.queueCount() as i64, 1);
        } else {
            expect_eq(1, 0);
        }

        pool.shutdown();
        expect_eq(pool.isRunning(), false);
        pool.~init();
    });

    test("4. cap:event — AnyTask & ManagedTask dynamic task execution", () => {
        let counter: i32 = 5;
        let any_task = AnyTask.init(&counter, testTaskCallback);
        any_task.run();
        expect_eq(counter as i64, 15);

        let managed = ManagedTask.create(&counter, testTaskCallback);
        managed.run();
        expect_eq(counter as i64, 25);
    });

    test("5. cap:sql — SqliteDatabase in-memory lifecycle & execution", () => {
        let db = SqliteDatabase.open(":memory:") catch {
            expect_eq(1, 0);
            return;
        };
        db.exec("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT);") catch {
            expect_eq(1, 0);
            return;
        };
        db.exec("INSERT INTO users (id, name) VALUES (1, 'Alice');") catch {
            expect_eq(1, 0);
            return;
        };
        db.exec("INSERT INTO users (id, name) VALUES (2, 'Bob');") catch {
            expect_eq(1, 0);
            return;
        };

        let rows = db.query("SELECT id, name FROM users ORDER BY id;") catch {
            expect_eq(1, 0);
            return;
        };
        expect_eq(rows.next(), true);
        expect_eq(rows.columnInt(0), 1);
        expect_eq(rows.next(), true);
        expect_eq(rows.columnInt(0), 2);
        expect_eq(rows.next(), false);

        rows.close();
        db.close();
    });

    test("6. cap:shell — Lexer, Parser & in-process builtin execution", () => {
        // Lexer verification
        let lexer = Lexer.init("echo 'hello world' | cat > out.txt && pwd");
        const t1 = lexer.nextToken();
        expect_eq(t1.kind == .Word, true);
        const t2 = lexer.nextToken();
        expect_eq(t2.kind == .Word, true);
        const t3 = lexer.nextToken();
        expect_eq(t3.kind == .Pipe, true);

        // Parser verification
        let parser = Parser.init("echo 1 2 3 ; pwd");
        const script = parser.parseScript();
        expect_eq(script.statements_len as i64, 2);

        // Synchronous Shell Execution
        const r1 = sh("echo -n 'bun-absorbed-cp'");
        expect_eq(r1.ok(), true);
        expect_eq(r1.exit_code as i64, 0);

        const r2 = sh("true && echo 1");
        expect_eq(r2.ok(), true);

        const r3 = sh("false || echo fallback");
        expect_eq(r3.ok(), true);
    });
}
