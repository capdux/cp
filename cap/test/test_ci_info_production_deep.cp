import { test, expect } from "cap:test";
import { CiVendor, EnvEntry, detectFromEntries } from "cap:ci_info";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("ci_info: GitHub Actions, GitLab, Buildkite, and Azure PR detection", () => {
    const gh: [2]EnvEntry = [
        EnvEntry{ key: "GITHUB_ACTIONS", value: "true" },
        EnvEntry{ key: "GITHUB_EVENT_NAME", value: "pull_request" },
    ];
    const r1 = detectFromEntries(gh[0..]);
    expect(r1.is_ci && r1.is_pr && r1.vendor == CiVendor.github_actions);
    expect(strEq(r1.name().?, "GitHub Actions"));

    const bk: [2]EnvEntry = [
        EnvEntry{ key: "BUILDKITE", value: "true" },
        EnvEntry{ key: "BUILDKITE_PULL_REQUEST", value: "false" },
    ];
    const r2 = detectFromEntries(bk[0..]);
    expect(r2.is_ci && !r2.is_pr && r2.vendor == CiVendor.buildkite);

    const az: [2]EnvEntry = [
        EnvEntry{ key: "TF_BUILD", value: "True" },
        EnvEntry{ key: "BUILD_REASON", value: "PullRequest" },
    ];
    const r3 = detectFromEntries(az[0..]);
    expect(r3.is_ci && r3.is_pr && r3.vendor == CiVendor.azure_pipelines);
});

test("ci_info: CI=false override and generic fallback", () => {
    const disabled: [2]EnvEntry = [
        EnvEntry{ key: "CI", value: "false" },
        EnvEntry{ key: "GITHUB_ACTIONS", value: "true" },
    ];
    const r1 = detectFromEntries(disabled[0..]);
    expect(!r1.is_ci && !r1.is_pr && r1.name() == null);

    const generic: [1]EnvEntry = [
        EnvEntry{ key: "CONTINUOUS_INTEGRATION", value: "1" },
    ];
    const r2 = detectFromEntries(generic[0..]);
    expect(r2.is_ci && r2.vendor == CiVendor.unknown && r2.name() == null);

    const empty: [0]EnvEntry = [];
    const r3 = detectFromEntries(empty[0..]);
    expect(!r3.is_ci && !r3.is_pr);
});
