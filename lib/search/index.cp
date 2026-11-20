// Source: cp/lib/search/index.cp (Unified Exports for cap:search)
// Unified Public Exports for Full-Text Search, BM25F, Boolean/Phrase Query,
// Snippet Highlighting & Prefix Autocomplete Engine (cap:search & cap.search)

export {
    SearchError,
    PrefixSuggestion,
    PrefixTrie,
} from "./trie.cp";

export {
    SearchHit,
    SearchIndex,
} from "./engine.cp";
