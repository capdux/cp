// Source: Custom (cap:test harness, per CHANGELOG)
/**
 * CP Test SDK (cap:test)
 * Zero-overhead native test runner & Bun/Jest-compatible testing framework.
 */

// ----------------------------------------------------------------------------
// 1. Legacy Builtin Entrypoints (100% Backwards Compatible)
// ----------------------------------------------------------------------------

pub fn test(name: string, callback: auto) {
    @test(name, callback);
}

pub fn expect_eq(actual: auto, expected: auto) {
    @expect_eq(actual, expected);
}

// ----------------------------------------------------------------------------
// 2. Bun/Jest-Compatible Chainable Expect & Suite/Hook DSL
// ----------------------------------------------------------------------------

export {
    NormalizeExpectType,
    Expectation,
    NegatedExpectation,
    expect,
} from "./test_runner/expect";

export {
    describe,
    describeSkip,
    it,
    itSkip,
    itTodo,
    beforeAll,
    beforeEach,
    afterEach,
    afterAll,
    mock,
    expect_true,
    expectEq,
    expectEqual,
    assert,
    assertEq,
} from "./test_runner/sdk";

// ----------------------------------------------------------------------------
// 3. Re-exports from cap:test_runner (Mock/Spy, FakeTimers, Snapshot, Runner)
// ----------------------------------------------------------------------------

export {
    TestOrderMode,
    Prng64,
    TestOrder,
    TestStatus,
    TestMode,
    HookKind,
    TestFilter,
    TestContext,
    TestCallback,
    HookCallback,
    TestCase,
    GroupItem,
    TestGroup,
    TestRunSummary,
    TestRunner,
    ReportFormat,
    PrettyFormatOptions,
    PrettyKind,
    PrettyField,
    PrettyNode,
    PrettyFormat,
    TestReporter,
    SNAPSHOT_FILE_HEADER,
    SnapshotUpdateMode,
    SnapshotMatchStatus,
    SnapshotMatchResult,
    SnapshotEntry,
    SnapshotStore,
    FAKE_TIMERS_MAX_SLOTS,
    FAKE_TIMERS_MAX_ADVANCE_MS,
    FAKE_TIMERS_DEFAULT_RUN_LIMIT,
    FakeTimerError,
    FakeTimerKind,
    FakeTimerSlot,
    FakeTimerFired,
    FakeTimerAdvanceResult,
    normalizeFakeDelayMs,
    FakeTimerClock,
    DONE_CALLBACK_DEFAULT_TIMEOUT_MS,
    DONE_CALLBACK_MAX_ERR_MSG,
    DoneCallbackError,
    DoneCallbackState,
    DoneCallback,
    MOCK_SPY_MAX_CALLS,
    MockSpyError,
    MockCallOutcome,
    MockCallRecord,
    MockSpy,
    ExpectOutcome,
    ExpectMatcher,
    ExpectNumMatcher,
    ExpectSeqMatcher,
    ExpectKvEntry,
    ExpectKeysMatcher,
    isCloseTo,
    equalIgnoringWhitespace,
    startsWith,
    endsWith,
    includesSubstring,
} from "./test_runner/index";
