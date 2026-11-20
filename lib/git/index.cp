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
