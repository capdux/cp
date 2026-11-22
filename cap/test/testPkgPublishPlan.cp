import { test, assert, assertEq } from "cap:test";
import {
    PUBLISH_DEFAULT_TAG,
    PUBLISH_DEFAULT_REGISTRY,
    PublishPlanError,
    PublishAccessLevel,
    PublishAuthMethod,
    PublishAuthTypeHeader,
    PublishDepKind,
    PublishStage,
    PublishDepSpec,
    PublishWorkspaceTarget,
    PublishCatalogEntry,
    PublishConfigOverrides,
    PublishPackageInput,
    PublishOptions,
    PublishAuthCredentials,
    PublishHttpHeader,
    PublishTarballDist,
    PublishLifecycleSession,
    encodeBase64Standard,
    isValidPublishPkgName,
    isScopedPublishPkgName,
    encodePackageNameForRegistry,
    isValidPublishSemver,
    validatePublishDistTag,
    applyPublishConfigOverrides,
    validatePublishInput,
    rewritePublishDependencies,
    buildPublishAuthHeaders,
    buildPublishPutUrl,
    formatPublishTarballUrl,
    buildPublishRegistryPayload,
    formatPublishDryRunReport,
} from "cap:pkg";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (haystack.len < needle.len) return false;
    const limit = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= limit) {
        let j: usize = 0;
        let ok = true;
        while (j < needle.len) {
            if (haystack[i + j] != needle[j]) {
                ok = false;
                break;
            }
            j += 1;
        }
        if (ok) return true;
        i += 1;
    }
    return false;
}

test("Package name, SemVer, dist-tag, private block, and scoped access policy validation", () => {
    assert(isValidPublishPkgName("lodash"));
    assert(isValidPublishPkgName("@acme/ui-core"));
    assert(isScopedPublishPkgName("@acme/ui-core"));
    assert(!isScopedPublishPkgName("lodash"));
    assert(!isValidPublishPkgName("UpperCase"));
    assert(!isValidPublishPkgName(".hidden"));
    assert(!isValidPublishPkgName("@scope/a/b"));

    let enc: [64]u8 = [0; 64];
    const elen = encodePackageNameForRegistry("@acme/ui-core", enc[0..]) catch unreachable;
    assert(strEql(enc[0..elen], "@acme%2fui-core"));

    assert(isValidPublishSemver("1.2.3"));
    assert(isValidPublishSemver("0.1.0-beta.1+build.42"));
    assert(!isValidPublishSemver("01.2.3"));
    assert(!isValidPublishSemver("1.2"));
    assert(!isValidPublishSemver("1.2.3-"));

    // Dist-tag must not conflict with SemVer
    validatePublishDistTag("next") catch unreachable;
    let tag_conflict = false;
    validatePublishDistTag("v1.2.3") catch |e| {
        tag_conflict = (e == PublishPlanError.DistTagConflictsWithSemver);
    };
    assert(tag_conflict);

    // private: true blocked
    const priv_pkg = PublishPackageInput{ name: "@acme/priv", version: "1.0.0", is_private: true };
    let priv_blocked = false;
    * = validatePublishInput(priv_pkg, PublishOptions.defaults()) catch |e| {
        priv_blocked = (e == PublishPlanError.PrivatePackageBlocked);
    };
    assert(priv_blocked);

    // Unscoped package with restricted access rejected
    const unscoped_pkg = PublishPackageInput{ name: "my-lib", version: "1.0.0" };
    let restricted_err = false;
    * = validatePublishInput(unscoped_pkg, PublishOptions{ access: PublishAccessLevel.Restricted }) catch |e| {
        restricted_err = (e == PublishPlanError.RestrictedUnscopedPackage);
    };
    assert(restricted_err);
});

test("publishConfig overrides priority: CLI > publishConfig > manifest > defaults", () => {
    const pkg = PublishPackageInput{
        name: "@acme/sdk",
        version: "2.0.0",
        main: "src/index.cp",
        module: "src/esm.cp",
        publish_config: PublishConfigOverrides{
            main: "dist/index.js",
            registry: "https://npm.acme.io",
            access: PublishAccessLevel.Public,
            tag: "beta",
            provenance: true,
        },
    };

    const eff1 = applyPublishConfigOverrides(pkg, PublishOptions.defaults());
    assert(strEql(eff1.main, "dist/index.js"));
    assert(strEql(eff1.module, "src/esm.cp"));
    assert(strEql(eff1.registry, "https://npm.acme.io"));
    assert(strEql(eff1.tag, "beta"));
    assert(eff1.access == PublishAccessLevel.Public);
    assert(eff1.provenance);

    // CLI tag overrides publishConfig.tag
    const cli_opts = PublishOptions{ tag: "canary", registry_url: "https://override.registry.dev" };
    const eff2 = applyPublishConfigOverrides(pkg, cli_opts);
    assert(strEql(eff2.tag, "canary"));
    assert(strEql(eff2.registry, "https://override.registry.dev"));
});

test("Pre-publish workspace:* / ^ / ~ and catalog: protocol dependency rewriting", () => {
    const workspaces: [2]PublishWorkspaceTarget = [
        PublishWorkspaceTarget{ name: "@acme/utils", version: "1.5.2" },
        PublishWorkspaceTarget{ name: "@acme/types", version: "2.0.0" },
    ];
    const catalogs: [2]PublishCatalogEntry = [
        PublishCatalogEntry{ catalog_name: "", pkg_name: "zod", version: "^3.23.8" },
        PublishCatalogEntry{ catalog_name: "react19", pkg_name: "react", version: "19.0.0" },
    ];
    const deps: [5]PublishDepSpec = [
        PublishDepSpec{ name: "@acme/utils", spec: "workspace:^", kind: PublishDepKind.Dependencies },
        PublishDepSpec{ name: "@acme/types", spec: "workspace:*", kind: PublishDepKind.PeerDependencies },
        PublishDepSpec{ name: "zod", spec: "catalog:", kind: PublishDepKind.Dependencies },
        PublishDepSpec{ name: "react", spec: "catalog:react19", kind: PublishDepKind.PeerDependencies },
        PublishDepSpec{ name: "lodash", spec: "^4.17.21", kind: PublishDepKind.OptionalDependencies },
    ];

    let out_deps: [8]PublishDepSpec = [PublishDepSpec.empty(); 8];
    let arena: [512]u8 = [0; 512];
    let arena_pos: usize = 0;

    const rewritten = rewritePublishDependencies(
        deps[0..],
        workspaces[0..],
        catalogs[0..],
        out_deps[0..],
        arena[0..],
        &arena_pos,
    ) catch unreachable;

    assertEq(rewritten, @as(u32, 4));
    assert(strEql(out_deps[0].spec, "^1.5.2"));
    assert(strEql(out_deps[1].spec, "2.0.0"));
    assert(strEql(out_deps[2].spec, "^3.23.8"));
    assert(strEql(out_deps[3].spec, "19.0.0"));
    assert(strEql(out_deps[4].spec, "^4.17.21"));

    // Unresolved workspace dep fails cleanly
    const bad_deps: [1]PublishDepSpec = [
        PublishDepSpec{ name: "@acme/missing", spec: "workspace:~" },
    ];
    let ws_err = false;
    * = rewritePublishDependencies(bad_deps[0..], workspaces[0..], catalogs[0..], out_deps[0..], arena[0..], &arena_pos) catch |e| {
        ws_err = (e == PublishPlanError.UnresolvedWorkspaceDep);
    };
    assert(ws_err);
});

test("Auth, OTP, Basic Base64, and OIDC Provenance HTTP header builder", () => {
    let headers: [8]PublishHttpHeader = [PublishHttpHeader.empty(); 8];
    let arena: [512]u8 = [0; 512];
    let arena_pos: usize = 0;

    // 1. Bearer + OTP + Provenance
    const bearer_creds = PublishAuthCredentials{
        method: PublishAuthMethod.BearerToken,
        token: "npm_secret_token_xyz",
        otp: "123456",
        auth_type: PublishAuthTypeHeader.Web,
        provenance: true,
    };
    const hlen = buildPublishAuthHeaders(bearer_creds, headers[0..], arena[0..], &arena_pos) catch unreachable;
    assertEq(hlen, @as(usize, 5));
    assert(strEql(headers[0].name, "authorization"));
    assert(strEql(headers[0].value, "Bearer npm_secret_token_xyz"));
    assert(strEql(headers[1].name, "npm-auth-type"));
    assert(strEql(headers[1].value, "web"));
    assert(strEql(headers[2].name, "npm-otp"));
    assert(strEql(headers[2].value, "123456"));
    assert(strEql(headers[3].name, "npm-provenance"));
    assert(strEql(headers[3].value, "true"));
    assert(strEql(headers[4].name, "content-type"));

    // 2. Basic Auth encodes user:pass in standard Base64 ("alice:s3cr3t" -> "YWxpY2U6czNjcjN0")
    const basic_creds = PublishAuthCredentials{
        method: PublishAuthMethod.BasicUserPass,
        username: "alice",
        password: "s3cr3t",
        auth_type: PublishAuthTypeHeader.Legacy,
    };
    const blen = buildPublishAuthHeaders(basic_creds, headers[0..], arena[0..], &arena_pos) catch unreachable;
    assertEq(blen, @as(usize, 3));
    assert(strEql(headers[0].value, "Basic YWxpY2U6czNjcjN0"));
    assert(strEql(headers[1].value, "legacy"));

    // 3. Invalid OTP code rejected
    const bad_otp = PublishAuthCredentials{
        method: PublishAuthMethod.BearerToken,
        token: "tok",
        otp: "12ab",
    };
    let otp_err = false;
    * = buildPublishAuthHeaders(bad_otp, headers[0..], arena[0..], &arena_pos) catch |e| {
        otp_err = (e == PublishPlanError.InvalidOtpCode);
    };
    assert(otp_err);
});

test("Standard Base64 encoder and full NPM/Cap Registry PUT JSON payload generation", () => {
    let b64: [32]u8 = [0; 32];
    const blen = encodeBase64Standard("hello", b64[0..]) catch unreachable;
    assert(strEql(b64[0..blen], "aGVsbG8="));

    const pkg = PublishPackageInput{
        name: "@acme/sdk",
        version: "1.4.0",
        description: "Fast \"CP\" SDK",
        license: "MIT",
        main: "dist/index.cp",
    };
    const summary = validatePublishInput(pkg, PublishOptions{ access: PublishAccessLevel.Public }) catch unreachable;

    let tb_url_buf: [256]u8 = [0; 256];
    const tb_url_len = formatPublishTarballUrl(summary.effective_registry, pkg.name, pkg.version, tb_url_buf[0..]) catch unreachable;
    assert(strEql(tb_url_buf[0..tb_url_len], "https://registry.npmjs.org/@acme/sdk/-/sdk-1.4.0.tgz"));

    const deps: [1]PublishDepSpec = [
        PublishDepSpec{ name: "zod", spec: "^3.23.8", kind: PublishDepKind.Dependencies },
    ];
    const dist = PublishTarballDist{
        tarball_filename: "@acme/sdk-1.4.0.tgz",
        tarball_url: tb_url_buf[0..tb_url_len],
        integrity: "sha512-abc123==",
        shasum: "0123456789abcdef0123456789abcdef01234567",
        file_count: 12,
        unpacked_size: 4096,
        tarball_bytes: "hello",
    };

    let json_buf: [2048]u8 = [0; 2048];
    const jlen = buildPublishRegistryPayload(summary, pkg, deps[0..], dist, json_buf[0..]) catch unreachable;
    const json = json_buf[0..jlen];

    assert(containsSubstr(json, "\"_id\":\"@acme/sdk\""));
    assert(containsSubstr(json, "\"description\":\"Fast \\\"CP\\\" SDK\""));
    assert(containsSubstr(json, "\"dist-tags\":{\"latest\":\"1.4.0\"}"));
    assert(containsSubstr(json, "\"dependencies\":{\"zod\":\"^3.23.8\"}"));
    assert(containsSubstr(json, "\"integrity\":\"sha512-abc123==\""));
    assert(containsSubstr(json, "\"fileCount\":12,\"unpackedSize\":4096"));
    assert(containsSubstr(json, "\"access\":\"public\""));
    assert(containsSubstr(json, "\"data\":\"aGVsbG8=\",\"length\":5"));

    // Invalid shasum rejected
    let bad_dist = dist;
    bad_dist.shasum = "too-short";
    let sha_err = false;
    * = buildPublishRegistryPayload(summary, pkg, deps[0..], bad_dist, json_buf[0..]) catch |e| {
        sha_err = (e == PublishPlanError.InvalidShasumFormat);
    };
    assert(sha_err);
});

test("Registry PUT endpoint URL and redacted dry-run summary report", () => {
    let url_buf: [256]u8 = [0; 256];
    const ulen = buildPublishPutUrl("https://registry.npmjs.org/", "@acme/sdk", url_buf[0..]) catch unreachable;
    assert(strEql(url_buf[0..ulen], "https://registry.npmjs.org/@acme%2fsdk"));

    const pkg = PublishPackageInput{ name: "@acme/sdk", version: "1.4.0" };
    const summary = validatePublishInput(pkg, PublishOptions{ access: PublishAccessLevel.Public, dry_run: true }) catch unreachable;
    assert(strEql(summary.effective_tag, PUBLISH_DEFAULT_TAG));
    assert(strEql(summary.effective_registry, PUBLISH_DEFAULT_REGISTRY));

    let rep_buf: [256]u8 = [0; 256];
    const rlen = formatPublishDryRunReport(summary, PublishAuthMethod.BearerToken, rep_buf[0..]) catch unreachable;
    assert(strEql(
        rep_buf[0..rlen],
        "publish @acme/sdk@1.4.0 tag=latest access=public registry=https://registry.npmjs.org auth=bearer(redacted) [DRY-RUN]",
    ));
});

test("PublishLifecycleSession 5-stage state machine and transition guards", () => {
    const pkg = PublishPackageInput{ name: "cap-demo", version: "0.1.0" };
    const summary = validatePublishInput(pkg, PublishOptions.defaults()) catch unreachable;

    let sess = PublishLifecycleSession.init();
    assert(sess.stage == PublishStage.Init);

    // Out-of-order transition blocked
    let bad_trans = false;
    sess.markUploaded() catch |e| {
        bad_trans = (e == PublishPlanError.InvalidStageTransition);
    };
    assert(bad_trans);

    sess.markValidated(summary) catch unreachable;
    assert(sess.stage == PublishStage.Validated);
    sess.markPacked(2048) catch unreachable;
    assertEq(sess.tarball_size, @as(usize, 2048));
    sess.markSigned() catch unreachable;
    sess.markUploaded() catch unreachable;
    sess.markVerified() catch unreachable;
    assert(sess.isComplete());
});
