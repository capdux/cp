// Source: cp/lib/spell/index.cp (Unified Exports for cap:spell)
// Pure CP zero-C-dependency SymSpell, BK-Tree, Damerau-Levenshtein & Soundex engine.

export {
    SpellError,
    SuggestVerbosity,
    SpellSuggestion,
    damerauLevenshtein,
    soundex,
    SymSpellIndex,
    BkTree,
} from "./spell.cp";
