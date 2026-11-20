// Source: cp/cap/test/test_git_production_deep.cp
// Deep Production Verification Suite for cap:git & cap.git
// Covers:
//   1. GitObject.hashBlob & hashObject authoritative Git SHA-1 vectors + GitOid hex codec
//   2. GitTreeCodec & GitCommitCodec encode/decode round-trip & malformed input guards
//   3. GitIgnore comments, negation (!), trailing '/' dir-only, leading '/' anchor & '**/*.log'
//   4. GitRepo.readHead on real repository /data/dev/cp + invalid path IoError check
//   5. `import cap from "cap"; cap.git` automatic dynamic mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    GitError,
    GitObjectKind,
    GitOid,
    GitObject,
    GitTreeEntry,
    GitTreeCodec,
    GitCommit,
    GitCommitCodec,
    GitIgnore,
    GitHead,
    GitRepo,
} from "cap:git";

fn testHashBlobVectorsAndOid(): !void {
    const alloc = std.heap.page_allocator;
    let hex_buf: [40]u8 = undefined;

    // 1. Authoritative Git empty blob vector: git hash-object -t blob /dev/null
    const empty_oid = GitObject.hashBlob("");
    expect_eq(
        std.mem.eql(u8, empty_oid.toHex(&hex_buf), "e69de29bb2d1d6434b8b29ae775ad8c2e48c5391"),
        true,
    );

    // 2. Authoritative Git "hello\n" vector: printf 'hello\n' | git hash-object --stdin
    const hello_oid = GitObject.hashBlob("hello\n");
    expect_eq(
        std.mem.eql(u8, hello_oid.toHex(&hex_buf), "ce013625030ba8dba906f756967f9e9ca394464a"),
        true,
    );

    // 3. Authoritative Git empty tree vector: 4b825dc642cb6eb9a060e54bf8d69288fbee4904
    const empty_tree_oid = GitObject.hashObject(GitObjectKind.Tree, "");
    expect_eq(
        std.mem.eql(u8, empty_tree_oid.toHex(&hex_buf), "4b825dc642cb6eb9a060e54bf8d69288fbee4904"),
        true,
    );

    // 4. GitOid hex parsing (mixed case), alloc formatting, equality, and validation
    const parsed_upper = GitOid.parseHex("CE013625030BA8DBA906F756967F9E9CA394464A")?;
    expect_eq(parsed_upper.eql(hello_oid), true);
    expect_eq(parsed_upper.eql(empty_oid), false);
    expect_eq(GitOid.zero().isZero(), true);
    expect_eq(hello_oid.isZero(), false);

    const alloc_hex = hello_oid.toHexAlloc(alloc)?;
    defer alloc.free(alloc_hex);
    expect_eq(std.mem.eql(u8, alloc_hex, "ce013625030ba8dba906f756967f9e9ca394464a"), true);

    expect_eq(GitOid.isValidHex("e69de29bb2d1d6434b8b29ae775ad8c2e48c5391"), true);
    expect_eq(GitOid.isValidHex("e69de29bb2d1d6434b8b29ae775ad8c2e48c539"), false);
    expect_eq(GitOid.isValidHex("g69de29bb2d1d6434b8b29ae775ad8c2e48c5391"), false);

    let hit_invalid_oid = false;
    * = GitOid.parseHex("not-a-valid-oid") catch |e| {
        if (e == GitError.InvalidOid) hit_invalid_oid = true;
    };
    expect_eq(hit_invalid_oid, true);
}

fn testTreeAndCommitCodecRoundTrip(): !void {
    const alloc = std.heap.page_allocator;

    const oid_readme = GitObject.hashBlob("# CP Git\n");
    const oid_src_dir = GitOid.parseHex("1111111111111111111111111111111111111111")?;
    const oid_src_old = GitOid.parseHex("2222222222222222222222222222222222222222")?;
    const oid_z = GitOid.parseHex("3333333333333333333333333333333333333333")?;

    // Unsorted input including Git's classic dir-vs-dash ordering: "src-old" ('-') < "src/" ('/')
    const entries: [4]GitTreeEntry = [
        GitTreeEntry{ mode: 0o100644, name: "z.txt", oid: oid_z },
        GitTreeEntry{ mode: 0o040000, name: "src", oid: oid_src_dir },
        GitTreeEntry{ mode: 0o100755, name: "src-old", oid: oid_src_old },
        GitTreeEntry{ mode: 0o100644, name: "README.md", oid: oid_readme },
    ];

    const raw_tree = GitTreeCodec.encode(alloc, &entries)?;
    defer alloc.free(raw_tree);

    const decoded = GitTreeCodec.decode(alloc, raw_tree)?;
    defer alloc.free(decoded);

    expect_eq(decoded.len, 4 as usize);
    expect_eq(std.mem.eql(u8, decoded[0].name, "README.md"), true);
    expect_eq(decoded[0].mode, 0o100644 as u32);
    expect_eq(decoded[0].oid.eql(oid_readme), true);

    expect_eq(std.mem.eql(u8, decoded[1].name, "src-old"), true);
    expect_eq(decoded[1].mode, 0o100755 as u32);
    expect_eq(decoded[1].oid.eql(oid_src_old), true);

    expect_eq(std.mem.eql(u8, decoded[2].name, "src"), true);
    expect_eq(decoded[2].mode, 0o040000 as u32);
    expect_eq(decoded[2].isDir(), true);
    expect_eq(decoded[2].oid.eql(oid_src_dir), true);

    expect_eq(std.mem.eql(u8, decoded[3].name, "z.txt"), true);
    expect_eq(decoded[3].oid.eql(oid_z), true);

    // Reject truncated tree payload
    let hit_bad_tree = false;
    * = GitTreeCodec.decode(alloc, raw_tree[0 .. raw_tree.len - 5]) catch |e| {
        if (e == GitError.InvalidObject) hit_bad_tree = true;
    };
    expect_eq(hit_bad_tree, true);

    // GitCommitCodec encode & parse round-trip with 2 parents (merge commit)
    const tree_oid = GitObject.hashObject(GitObjectKind.Tree, raw_tree);
    const parent1 = GitOid.parseHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")?;
    const parent2 = GitOid.parseHex("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb")?;
    const parent_list: [2]GitOid = [parent1, parent2];

    const commit_in = GitCommit{
        tree: tree_oid,
        parents: &parent_list,
        author: "Alice <alice@cp-lang.org> 1700000000 +0000",
        committer: "Bob <bob@cp-lang.org> 1700000100 +0800",
        message: "feat(git): merge initial tree\n\nSigned-off-by: Alice\n",
    };

    const raw_commit = GitCommitCodec.encode(alloc, commit_in)?;
    defer alloc.free(raw_commit);

    let parsed = GitCommitCodec.parse(alloc, raw_commit)?;
    defer parsed.deinit();

    expect_eq(parsed.tree.eql(tree_oid), true);
    expect_eq(parsed.parents.len, 2 as usize);
    expect_eq(parsed.parents[0].eql(parent1), true);
    expect_eq(parsed.parents[1].eql(parent2), true);
    expect_eq(std.mem.eql(u8, parsed.author, commit_in.author), true);
    expect_eq(std.mem.eql(u8, parsed.committer, commit_in.committer), true);
    expect_eq(std.mem.eql(u8, parsed.message, commit_in.message), true);

    // Verify signed commit with multi-line gpgsig header parses cleanly
    const signed_raw =
        "tree 4b825dc642cb6eb9a060e54bf8d69288fbee4904\n" ++
        "author Dev <dev@cp-lang.org> 1700000000 +0000\n" ++
        "committer Dev <dev@cp-lang.org> 1700000000 +0000\n" ++
        "gpgsig -----BEGIN PGP SIGNATURE-----\n" ++
        " iQIzBAABCAAdFiEEexample\n" ++
        " -----END PGP SIGNATURE-----\n\n" ++
        "signed commit\n";
    let parsed_signed = GitCommitCodec.parse(alloc, signed_raw)?;
    defer parsed_signed.deinit();
    expect_eq(parsed_signed.parents.len, 0 as usize);
    expect_eq(std.mem.eql(u8, parsed_signed.message, "signed commit\n"), true);
}

fn testGitIgnoreEngine(): !void {
    const alloc = std.heap.page_allocator;
    let gi = GitIgnore.init(alloc);
    defer gi.deinit();

    const rules =
        "# Build & log exclusions\n" ++
        "\n" ++
        "*.log\r\n" ++
        "!important.log\n" ++
        "build/\n" ++
        "/root_only.txt\n" ++
        "docs/secret.md\n" ++
        "**/*.tmp\n" ++
        "src/**/test_*.o\n" ++
        "cache/**\n" ++
        "shard_[0-9]?.bin\n";
    gi.addPatterns(rules)?;

    // 1. Comments & blank lines skipped; *.log + !important.log last-match-wins
    expect_eq(gi.isIgnored("# Build & log exclusions", false), false);
    expect_eq(gi.isIgnored("app.log", false), true);
    expect_eq(gi.isIgnored("logs/debug.log", false), true);
    expect_eq(gi.isIgnored("important.log", false), false);
    expect_eq(gi.isIgnored("sub/important.log", false), false);

    // 2. Trailing '/' matches directories (at any depth) and contents inside ignored dirs, not files
    expect_eq(gi.isIgnored("build", true), true);
    expect_eq(gi.isIgnored("build", false), false);
    expect_eq(gi.isIgnored("packages/core/build", true), true);
    expect_eq(gi.isIgnored("packages/core/build", false), false);
    expect_eq(gi.isIgnored("build/output.o", false), true);

    // 3. Leading '/' and internal '/' anchor to repo root
    expect_eq(gi.isIgnored("root_only.txt", false), true);
    expect_eq(gi.isIgnored("nested/root_only.txt", false), false);
    expect_eq(gi.isIgnored("docs/secret.md", false), true);
    expect_eq(gi.isIgnored("sub/docs/secret.md", false), false);

    // 4. Double-star '**' multi-directory matching
    expect_eq(gi.isIgnored("a.tmp", false), true);
    expect_eq(gi.isIgnored("dir/a.tmp", false), true);
    expect_eq(gi.isIgnored("a/b/c/d.tmp", false), true);
    expect_eq(gi.isIgnored("src/test_one.o", false), true);
    expect_eq(gi.isIgnored("src/sub/deep/test_two.o", false), true);
    expect_eq(gi.isIgnored("lib/src/test_one.o", false), false);
    expect_eq(gi.isIgnored("cache", true), false);
    expect_eq(gi.isIgnored("cache/entry", false), true);
    expect_eq(gi.isIgnored("cache/a/b", false), true);

    // 5. '?' and '[0-9]' character class wildcards
    expect_eq(gi.isIgnored("shard_3a.bin", false), true);
    expect_eq(gi.isIgnored("shard_aa.bin", false), false);
    expect_eq(gi.isIgnored("shard_3.bin", false), false);
}

fn testGitRepoReadHeadOnRealRepo(): !void {
    const alloc = std.heap.page_allocator;

    // Inspect the real /data/dev/cp repository HEAD
    let head = GitRepo.readHead(alloc, "/data/dev/cp")?;
    defer head.deinit();

    expect_eq(head.oid_hex.len, 40 as usize);
    expect_eq(GitOid.isValidHex(head.oid_hex), true);
    const oid = GitOid.parseHex(head.oid_hex)?;
    expect_eq(oid.isZero(), false);
    if (!head.is_detached) {
        expect_eq(head.branch_name.len > 0, true);
    }

    // Missing repo must return GitError.IoError
    let hit_io_err = false;
    * = GitRepo.readHead(alloc, "/nonexistent/path/cp_missing_git_repo") catch |e| {
        if (e == GitError.IoError) hit_io_err = true;
    };
    expect_eq(hit_io_err, true);
}

fn testCapGitDynamicMount(): !void {
    const alloc = std.heap.page_allocator;
    let hex_buf: [40]u8 = undefined;

    // 1. cap.git.GitObject.hashBlob & GitOid
    const oid = cap.git.GitObject.hashBlob("hello\n");
    expect_eq(
        std.mem.eql(u8, oid.toHex(&hex_buf), "ce013625030ba8dba906f756967f9e9ca394464a"),
        true,
    );

    // 2. cap.git.GitTreeCodec & GitCommitCodec
    const entry_arr: [1]cap.git.GitTreeEntry = [
        cap.git.GitTreeEntry{ mode: 0o100644, name: "hello.txt", oid: oid },
    ];
    const tree_bytes = cap.git.GitTreeCodec.encode(alloc, &entry_arr)?;
    defer alloc.free(tree_bytes);
    const tree_oid = cap.git.GitObject.hashObject(cap.git.GitObjectKind.Tree, tree_bytes);
    const decoded_tree = cap.git.GitTreeCodec.decode(alloc, tree_bytes)?;
    defer alloc.free(decoded_tree);
    expect_eq(decoded_tree.len, 1 as usize);

    const commit_raw = std.fmt.allocPrint(
        alloc,
        "tree {s}\nauthor A <a@b.c> 1 +0000\ncommitter A <a@b.c> 1 +0000\n\ninit\n",
        .{tree_oid.toHex(&hex_buf)},
    ) catch unreachable;
    defer alloc.free(commit_raw);
    let parsed_commit = cap.git.GitCommitCodec.parse(alloc, commit_raw)?;
    defer parsed_commit.deinit();
    expect_eq(parsed_commit.tree.eql(tree_oid), true);

    // 3. cap.git.GitIgnore
    let gi = cap.git.GitIgnore.init(alloc);
    defer gi.deinit();
    gi.addPatterns("**/*.log\n!keep.log\n")?;
    expect_eq(gi.isIgnored("a/b/c.log", false), true);
    expect_eq(gi.isIgnored("a/b/keep.log", false), false);

    // 4. cap.git.GitRepo.readHead
    let head = cap.git.GitRepo.readHead(alloc, "/data/dev/cp")?;
    defer head.deinit();
    expect_eq(head.oid_hex.len, 40 as usize);
}

fn main() {
    test("1. GitObject.hashBlob & hashObject authoritative SHA-1 vectors + GitOid codec", () => {
        testHashBlobVectorsAndOid()?;
    });
    test("2. GitTreeCodec & GitCommitCodec encode/decode round-trip and validation", () => {
        testTreeAndCommitCodecRoundTrip()?;
    });
    test("3. GitIgnore comments, negation (!), dir-only (/), anchors & **/*.log globs", () => {
        testGitIgnoreEngine()?;
    });
    test("4. GitRepo.readHead real /data/dev/cp repo HEAD & OID inspection", () => {
        testGitRepoReadHeadOnRealRepo()?;
    });
    test("5. import cap from \"cap\"; cap.git Automatic Dynamic Mounting", () => {
        testCapGitDynamicMount()?;
    });
}
