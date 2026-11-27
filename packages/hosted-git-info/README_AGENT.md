# hosted-git-info@1.0.0 (Agent Quickref)

> Zero-heap parser & URL formatter for GitHub, GitLab, and Bitbucket shorthands, SSH, and HTTPS git URLs (npm hosted-git-info equivalent) | License: MIT
> Import: `import { ... } from "hosted-git-info";` | Full types: `src/index.d.cp`

## Functions
- `export fn hgiPush(out: []u8, pos: usize, s: string): usize;`
- `export fn hgiStartsWith(s: string, prefix: string): bool;`
- `export fn hgiEndsWith(s: string, suffix: string): bool;`
- `export fn hgiFindChar(s: string, ch: u8): usize?;`
- `export fn hgiBuildFromPath(host_type: u8, domain: string, path_and_hash: string): HostedGitInfo;`
- `export fn fromUrl(raw: string): HostedGitInfo;`
