// Type definitions for CP

export class HostedGitInfo {
    host_type: u8,
    domain: string,
    user: string,
    project: string,
    committish: string,
    valid: bool,
    fn invalid(): HostedGitInfo;
    fn formatHttps(out: []u8): usize;
    fn formatSsh(out: []u8): usize;
    fn formatBrowse(out: []u8): usize;
    fn formatBugs(out: []u8): usize;
}

export fn hgiPush(out: []u8, pos: usize, s: string): usize;
export fn hgiStartsWith(s: string, prefix: string): bool;
export fn hgiEndsWith(s: string, suffix: string): bool;
export fn hgiFindChar(s: string, ch: u8): usize?;
export fn hgiBuildFromPath(host_type: u8, domain: string, path_and_hash: string): HostedGitInfo;
export fn fromUrl(raw: string): HostedGitInfo;
