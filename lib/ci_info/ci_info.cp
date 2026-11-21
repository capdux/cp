// Source: Absorbed from /data/dev/bun/src/cli/ci_info.cp and codegen/ci_info.ts
// Pure-CP zero-allocation CI/CD environment detector (ci-info v4 semantics).

pub enum CiVendor: u8 {
    unknown = 0,
    github_actions = 1,
    gitlab = 2,
    circleci = 3,
    travis = 4,
    jenkins = 5,
    buildkite = 6,
    azure_pipelines = 7,
    aws_codebuild = 8,
    bitbucket = 9,
    appveyor = 10,
    vercel = 11,
    netlify = 12,
    cloudflare_pages = 13,
    cloudflare_workers = 14,
    drone = 15,
    teamcity = 16,
    bamboo = 17,
    bitrise = 18,
    buddy = 19,
    cirrus = 20,
    codefresh = 21,
    codemagic = 22,
    render = 23,
    railway = 24,
    woodpecker = 25,
    semaphore = 26,
    harness = 27,
    google_cloud_build = 28,
    agola = 29,
    appcircle = 30,
    github_codespaces = 31,

    pub fn displayName(): []const u8 {
        return switch (_) {
            .unknown => "Unknown",
            .github_actions => "GitHub Actions",
            .gitlab => "GitLab CI",
            .circleci => "CircleCI",
            .travis => "Travis CI",
            .jenkins => "Jenkins",
            .buildkite => "Buildkite",
            .azure_pipelines => "Azure Pipelines",
            .aws_codebuild => "AWS CodeBuild",
            .bitbucket => "Bitbucket Pipelines",
            .appveyor => "AppVeyor",
            .vercel => "Vercel",
            .netlify => "Netlify CI",
            .cloudflare_pages => "Cloudflare Pages",
            .cloudflare_workers => "Cloudflare Workers",
            .drone => "Drone",
            .teamcity => "TeamCity",
            .bamboo => "Bamboo",
            .bitrise => "Bitrise",
            .buddy => "Buddy",
            .cirrus => "Cirrus CI",
            .codefresh => "Codefresh",
            .codemagic => "Codemagic",
            .render => "Render",
            .railway => "Railway",
            .woodpecker => "Woodpecker",
            .semaphore => "Semaphore",
            .harness => "Harness CI",
            .google_cloud_build => "Google Cloud Build",
            .agola => "Agola CI",
            .appcircle => "Appcircle",
            .github_codespaces => "GitHub Codespaces",
        };
    }

    pub fn constantName(): []const u8 {
        return switch (_) {
            .unknown => "UNKNOWN",
            .github_actions => "GITHUB_ACTIONS",
            .gitlab => "GITLAB",
            .circleci => "CIRCLE",
            .travis => "TRAVIS",
            .jenkins => "JENKINS",
            .buildkite => "BUILDKITE",
            .azure_pipelines => "AZURE_PIPELINES",
            .aws_codebuild => "CODEBUILD",
            .bitbucket => "BITBUCKET",
            .appveyor => "APPVEYOR",
            .vercel => "VERCEL",
            .netlify => "NETLIFY",
            .cloudflare_pages => "CLOUDFLARE_PAGES",
            .cloudflare_workers => "CLOUDFLARE_WORKERS",
            .drone => "DRONE",
            .teamcity => "TEAMCITY",
            .bamboo => "BAMBOO",
            .bitrise => "BITRISE",
            .buddy => "BUDDY",
            .cirrus => "CIRRUS",
            .codefresh => "CODEFRESH",
            .codemagic => "CODEMAGIC",
            .render => "RENDER",
            .railway => "RAILWAY",
            .woodpecker => "WOODPECKER",
            .semaphore => "SEMAPHORE",
            .harness => "HARNESS",
            .google_cloud_build => "GCLOUD",
            .agola => "AGOLA",
            .appcircle => "APPCIRCLE",
            .github_codespaces => "CODESPACES",
        };
    }
}

pub struct EnvEntry {
    key: []const u8,
    value: []const u8,
}

pub struct CiInfo {
    is_ci: bool,
    is_pr: bool,
    vendor: CiVendor,

    pub fn name(): ([]const u8)? {
        if (_.vendor == .unknown) return null;
        return _.vendor.displayName();
    }
}

/// Detect CI environment from a slice of environment key-value entries.
pub fn detectFromEntries(entries: []const EnvEntry): CiInfo {
    // Explicit `CI=false` or `CI=0` disables detection (matching Bun's ci_info.cp)
    if (ci_val := lookupEnv(entries, "CI")) {
        if (eqIgnoreCase(ci_val, "false") || strEq(ci_val, "0")) {
            return CiInfo{ is_ci: false, is_pr: false, vendor: .unknown };
        }
    }

    const vendor = detectVendor(entries);
    if (vendor != .unknown) {
        return CiInfo{
            is_ci: true,
            is_pr: detectPr(entries, vendor),
            vendor: vendor,
        };
    }

    // Fallback generic CI environment variables
    const generic_ci = hasNonEmpty(entries, "CI") ||
        hasNonEmpty(entries, "CONTINUOUS_INTEGRATION") ||
        hasNonEmpty(entries, "BUILD_ID") ||
        hasNonEmpty(entries, "BUILD_NUMBER") ||
        hasNonEmpty(entries, "CI_APP_ID") ||
        hasNonEmpty(entries, "CI_BUILD_ID") ||
        hasNonEmpty(entries, "CI_BUILD_NUMBER") ||
        hasNonEmpty(entries, "CI_NAME") ||
        hasNonEmpty(entries, "RUN_ID");

    return CiInfo{
        is_ci: generic_ci,
        is_pr: false,
        vendor: .unknown,
    };
}

fn detectVendor(entries: []const EnvEntry): CiVendor {
    if (hasNonEmpty(entries, "GITHUB_ACTIONS")) return .github_actions;
    if (hasNonEmpty(entries, "GITLAB_CI")) return .gitlab;
    if (hasNonEmpty(entries, "CIRCLECI")) return .circleci;
    if (hasNonEmpty(entries, "TRAVIS")) return .travis;
    if (hasNonEmpty(entries, "JENKINS_URL") && hasNonEmpty(entries, "BUILD_ID")) return .jenkins;
    if (hasNonEmpty(entries, "BUILDKITE")) return .buildkite;
    if (hasNonEmpty(entries, "TF_BUILD")) return .azure_pipelines;
    if (hasNonEmpty(entries, "CODEBUILD_BUILD_ARN")) return .aws_codebuild;
    if (hasNonEmpty(entries, "BITBUCKET_COMMIT")) return .bitbucket;
    if (hasNonEmpty(entries, "APPVEYOR")) return .appveyor;
    if (hasNonEmpty(entries, "VERCEL") || hasNonEmpty(entries, "NOW_BUILDER")) return .vercel;
    if (hasNonEmpty(entries, "NETLIFY")) return .netlify;
    if (hasNonEmpty(entries, "CF_PAGES")) return .cloudflare_pages;
    if (hasNonEmpty(entries, "WORKERS_CI")) return .cloudflare_workers;
    if (hasNonEmpty(entries, "DRONE")) return .drone;
    if (hasNonEmpty(entries, "TEAMCITY_VERSION")) return .teamcity;
    if (hasNonEmpty(entries, "bamboo_planKey")) return .bamboo;
    if (hasNonEmpty(entries, "BITRISE_IO")) return .bitrise;
    if (hasNonEmpty(entries, "BUDDY_WORKSPACE_ID")) return .buddy;
    if (hasNonEmpty(entries, "CIRRUS_CI")) return .cirrus;
    if (hasNonEmpty(entries, "CF_BUILD_ID")) return .codefresh;
    if (hasNonEmpty(entries, "CM_BUILD_ID")) return .codemagic;
    if (hasNonEmpty(entries, "RENDER")) return .render;
    if (hasNonEmpty(entries, "RAILWAY_PROJECT_ID")) return .railway;
    if (hasNonEmpty(entries, "WOODPECKER_CI") || strEq(lookupEnv(entries, "CI") ?? "", "woodpecker")) return .woodpecker;
    if (hasNonEmpty(entries, "SEMAPHORE")) return .semaphore;
    if (hasNonEmpty(entries, "HARNESS_BUILD_ID")) return .harness;
    if (hasNonEmpty(entries, "BUILDER_OUTPUT")) return .google_cloud_build;
    if (hasNonEmpty(entries, "AGOLA_GIT_REF")) return .agola;
    if (hasNonEmpty(entries, "AC_APPCIRCLE")) return .appcircle;
    if (hasNonEmpty(entries, "CODESPACES")) return .github_codespaces;
    return .unknown;
}

fn detectPr(entries: []const EnvEntry, vendor: CiVendor): bool {
    switch (vendor) {
        .github_actions => return strEq(lookupEnv(entries, "GITHUB_EVENT_NAME") ?? "", "pull_request"),
        .gitlab => return hasNonEmpty(entries, "CI_MERGE_REQUEST_ID"),
        .circleci => return hasNonEmpty(entries, "CIRCLE_PULL_REQUEST"),
        .travis => return isTruthyNotFalse(lookupEnv(entries, "TRAVIS_PULL_REQUEST")),
        .buildkite => return isTruthyNotFalse(lookupEnv(entries, "BUILDKITE_PULL_REQUEST")),
        .azure_pipelines => return strEq(lookupEnv(entries, "BUILD_REASON") ?? "", "PullRequest"),
        .aws_codebuild => {
            const ev = lookupEnv(entries, "CODEBUILD_WEBHOOK_EVENT") ?? "";
            return strEq(ev, "PULL_REQUEST_CREATED") || strEq(ev, "PULL_REQUEST_UPDATED") || strEq(ev, "PULL_REQUEST_REOPENED");
        },
        .bitbucket => return hasNonEmpty(entries, "BITBUCKET_PR_ID"),
        .appveyor => return hasNonEmpty(entries, "APPVEYOR_PULL_REQUEST_NUMBER"),
        .netlify => return strEq(lookupEnv(entries, "PULL_REQUEST") ?? "", "true"),
        .drone => return strEq(lookupEnv(entries, "DRONE_BUILD_EVENT") ?? "", "pull_request"),
        .bitrise => return hasNonEmpty(entries, "BITRISE_PULL_REQUEST"),
        .buddy => return hasNonEmpty(entries, "BUDDY_EXECUTION_PULL_REQUEST_ID"),
        .cirrus => return hasNonEmpty(entries, "CIRRUS_PR"),
        .codefresh => return hasNonEmpty(entries, "CF_PULL_REQUEST_NUMBER") || hasNonEmpty(entries, "CF_PULL_REQUEST_ID"),
        .agola => return hasNonEmpty(entries, "AGOLA_PULL_REQUEST_ID"),
        .appcircle => return isTruthyNotFalse(lookupEnv(entries, "AC_GIT_PR")),
        .jenkins => return hasNonEmpty(entries, "CHANGE_ID") || hasNonEmpty(entries, "ghprbPullId"),
        else => return false,
    }
}

fn lookupEnv(entries: []const EnvEntry, key: []const u8): ([]const u8)? {
    let i: usize = 0;
    while (i < entries.len) : (i += 1) {
        if (strEq(entries[i].key, key)) return entries[i].value;
    }
    return null;
}

fn hasNonEmpty(entries: []const EnvEntry, key: []const u8): bool {
    if (v := lookupEnv(entries, key)) {
        return v.len > 0;
    }
    return false;
}

fn isTruthyNotFalse(v: ([]const u8)?): bool {
    if (s := v) {
        return s.len > 0 && !eqIgnoreCase(s, "false");
    }
    return false;
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn toLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLower(a[i]) != toLower(b[i])) return false;
    }
    return true;
}
