// Source: cp/lib/git/index.cp (Unified Exports for cap:git)
// Unified Public Exports for Git Object Hashing/Codecs, .gitignore Engine & .git Inspector (cap:git & cap.git)

export {
    GitError,
    GitObjectKind,
    GitOid,
    GitObject,
    GitTreeEntry,
    GitTreeCodec,
    GitCommit,
    GitCommitCodec,
} from "./object.cp";

export {
    GitIgnore,
} from "./ignore.cp";

export {
    GitHead,
    GitRepo,
} from "./repo.cp";

export {
    FileChangeKind,
    FileDiffStat,
    DiffSummary,
    GitDiffIter,
    summarizeGitDiff,
} from "./diffstat.cp";

export {
    GitRefKind,
    ParsedGitRef,
    isValidGitRefName,
    isValidBranchName,
    parseFullGitRef,
    isValidGitOidHex,
    isValidShortOidHex,
} from "./refcheck.cp";

export {
    GitProtocol,
    GitRemoteUrl,
} from "./url.cp";

export {
    GitignoreRule,
    parseGitignoreLine,
    ruleMatchesPath,
    isIgnored,
} from "./ignorematch.cp";

export {
    GitConfigEntry,
    GitConfig,
} from "./config.cp";

export {
    ReleaseBump,
    ConventionalCommit,
} from "./commit.cp";

export {
    CodeownersRule,
    OwnerIter,
    CodeownersIter,
    resolveOwners,
    isOwnedBy,
} from "./codeowners.cp";
