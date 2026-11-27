// Migrated from TypeScript (assets/reading-list/react_16.14.0.js) via cap from -ts

/** @license React v16.14.0
 * react.development.js
 *
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

"use strict";

// TODO(cap-migrate): refine TS statement
// [ts] ((global, factory) => {
// [ts] if (@typeName(exports) == "object" && @typeName(module) != "undefined") { factory(exports) else; }
// [ts] if (@typeName(define) == "function" && define.amd) { define(["exports"], factory) else; }
// [ts] (global = global || self, factory(global.React = {}));
// [ts] }(this, ((exports) => { "use strict";
// [ts] let ReactVersion = "16.14.0";
// The Symbol used to tag the ReactElement-like types. If there is no native Symbol
// nor polyfill, then a plain number is used for performance.
// [ts] let hasSymbol = @typeName(Symbol) == "function" && "__symbol_for";
// [ts] let REACT_ELEMENT_TYPE = if (hasSymbol) "__symbol_react.element" else 0xeac7;
// [ts] let REACT_PORTAL_TYPE = if (hasSymbol) "__symbol_react.portal" else 0xeaca;
// [ts] let REACT_FRAGMENT_TYPE = if (hasSymbol) "__symbol_react.fragment" else 0xeacb;
// [ts] let REACT_STRICT_MODE_TYPE = if (hasSymbol) "__symbol_react.strict_mode" else 0xeacc;
// [ts] let REACT_PROFILER_TYPE = if (hasSymbol) "__symbol_react.profiler" else 0xead2;
// [ts] let REACT_PROVIDER_TYPE = if (hasSymbol) "__symbol_react.provider" else 0xeacd;
// [ts] let REACT_CONTEXT_TYPE = if (hasSymbol) "__symbol_react.context" else 0xeace; // TODO: We don't use AsyncMode or ConcurrentMode anymore. They were temporary
// [ts] let REACT_CONCURRENT_MODE_TYPE = if (hasSymbol) "__symbol_react.concurrent_mode" else 0xeacf;
// [ts] let REACT_FORWARD_REF_TYPE = if (hasSymbol) "__symbol_react.forward_ref" else 0xead0;
// [ts] let REACT_SUSPENSE_TYPE = if (hasSymbol) "__symbol_react.suspense" else 0xead1;
// [ts] let REACT_SUSPENSE_LIST_TYPE = if (hasSymbol) "__symbol_react.suspense_list" else 0xead8;
// [ts] let REACT_MEMO_TYPE = if (hasSymbol) "__symbol_react.memo" else 0xead3;
// [ts] let REACT_LAZY_TYPE = if (hasSymbol) "__symbol_react.lazy" else 0xead4;
// [ts] let REACT_BLOCK_TYPE = if (hasSymbol) "__symbol_react.block" else 0xead9;
// [ts] let REACT_FUNDAMENTAL_TYPE = if (hasSymbol) "__symbol_react.fundamental" else 0xead5;
// [ts] let REACT_RESPONDER_TYPE = if (hasSymbol) "__symbol_react.responder" else 0xead6;
// [ts] let REACT_SCOPE_TYPE = if (hasSymbol) "__symbol_react.scope" else 0xead7;
// [ts] let MAYBE_ITERATOR_SYMBOL = @typeName(Symbol) == "function" && "__symbol_iterator";
// [ts] let FAUX_ITERATOR_SYMBOL = "@@iterator";
// [ts] fn getIteratorFn(maybeIterable: auto) {
// [ts] if (maybeIterable == null || @typeName(maybeIterable) != "object") {
// [ts] return null;
// [ts] }
// [ts] let maybeIterator = MAYBE_ITERATOR_SYMBOL && maybeIterable[MAYBE_ITERATOR_SYMBOL] || maybeIterable[FAUX_ITERATOR_SYMBOL];
// [ts] if (@typeName(maybeIterator) == "function") {
// [ts] return maybeIterator;
// [ts] }
// [ts] return null;
// [ts] }
// [ts] /*
// [ts] object-assign
// [ts] (c) Sindre Sorhus
// [ts] @license MIT
// [ts] */
// [ts] /* eslint-disable no-unused-vars */
// [ts] let getOwnPropertySymbols = Object.getOwnPropertySymbols;
// [ts] let hasOwnProperty = Object.prototype.hasOwnProperty;
// [ts] let propIsEnumerable = Object.prototype.propertyIsEnumerable;
// [ts] fn toObject(val: auto) {
// [ts] if (val == null || val == null) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new TypeError('Object.assign cannot be called with null or undefined');)
// [ts] }
// [ts] return Object(val);
// [ts] }
// [ts] fn shouldUseNative() {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] if (!Object.assign) {
// [ts] return false;
// [ts] }
// Detect buggy property enumeration order in older V8 versions.
// https://bugs.chromium.org/p/v8/issues/detail?id=4118
// [ts] let test1 = String.init("abc");  // eslint-disable-line no-new-wrappers
// [ts] test1[5] = "de";
// [ts] if (Object.getOwnPropertyNames(test1)[0] == "5") {
// [ts] return false;
// [ts] }
// https://bugs.chromium.org/p/v8/issues/detail?id=3056
// [ts] let test2 = {};
// [ts] let i = 0;
// [ts] while (i < 10) { // [cap-migrate] for-step: i++
// [ts] test2["_" + String.fromCharCode(i)] = i;
// [ts] }
// [ts] let order2 = Object.getOwnPropertyNames(test2).map((n) => {
// [ts] return test2[n];
// [ts] });
// [ts] if (order2.join("") != "0123456789") {
// [ts] return false;
// [ts] }
// https://bugs.chromium.org/p/v8/issues/detail?id=3056
// [ts] let test3 = {};
// [ts] "abcdefghijklmnopqrst".split("").forEach((letter) => {
// [ts] test3[letter] = letter;
// [ts] });
// [ts] if (Object.keys(Object.assign({}, test3)).join("") !=
// [ts] "abcdefghijklmnopqrst") {
// [ts] return false;
// [ts] }
// [ts] return true;
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {)
// We don't expect any of the above to throw, but better to be safe.
// [ts] return false;
// [ts] }
// [ts] }
// [ts] let objectAssign = if (shouldUseNative()) Object.assign else (target, source) => {
// [ts] let from;
// [ts] let to = toObject(target);
// [ts] let symbols;
// [ts] let s = 1;
// [ts] while (s < arguments.length) { // [cap-migrate] for-step: s++
// [ts] from = Object(arguments[s]);
// [ts] for (let key in from) {
// [ts] if (hasOwnProperty.call(from, key)) {
// [ts] to[key] = from[key];
// [ts] }
// [ts] }
// [ts] if (getOwnPropertySymbols) {
// [ts] symbols = getOwnPropertySymbols(from);
// [ts] let i = 0;
// [ts] while (i < symbols.length) { // [cap-migrate] for-step: i++
// [ts] if (propIsEnumerable.call(from, symbols[i])) {
// [ts] to[symbols[i]] = from[symbols[i]];
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] return to;
// [ts] };
// [ts] /**
// [ts] * Keeps track of the current dispatcher.
// [ts] */
// [ts] let ReactCurrentDispatcher = {
// [ts] /**
// [ts] * @internal
// [ts] * @type {ReactComponent}
// [ts] */
// [ts] current: null
// [ts] };
// [ts] /**
// [ts] * Keeps track of the current batch's configuration such as how long an update
// [ts] * should suspend for if it needs to.
// [ts] */
// [ts] let ReactCurrentBatchConfig = {
// [ts] suspense: null
// [ts] };
// [ts] /**
// [ts] * Keeps track of the current owner.
// [ts] *
// [ts] * The current owner is the component who should own any components that are
// [ts] * currently being constructed.
// [ts] */
// [ts] let ReactCurrentOwner = {
// [ts] /**
// [ts] * @internal
// [ts] * @type {ReactComponent}
// [ts] */
// [ts] current: null
// [ts] };
// [ts] let BEFORE_SLASH_RE = "^(.*)[\\\\\\/]";
// [ts] fn describeComponentFrame(name: auto, source: auto, ownerName: auto) {
// [ts] let sourceInfo = "";
// [ts] if (source) {
// [ts] let path = source.fileName;
// [ts] let fileName = path.replace(BEFORE_SLASH_RE, "");
// [ts] {
// In DEV, include code for a common special case:
// prefer "folder/index.js" instead of just "index.js".
// [ts] if ("^index\\.".test(fileName)) {
// [ts] let match_val = path.match(BEFORE_SLASH_RE);
// [ts] if (match_val) {
// [ts] let pathBeforeSlash = match_val[1];
// [ts] if (pathBeforeSlash) {
// [ts] let folderName = pathBeforeSlash.replace(BEFORE_SLASH_RE, "");
// [ts] fileName = folderName + "/" + fileName;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] sourceInfo = " (at " + fileName + ":" + source.lineNumber + ")";
// [ts] } else if (ownerName) {
// [ts] sourceInfo = " (created by " + ownerName + ")";
// [ts] }
// [ts] return "\n    in " + (name || "Unknown") + sourceInfo;
// [ts] }
// [ts] let Resolved = 1;
// [ts] fn refineResolvedLazyComponent(lazyComponent: auto) {
// [ts] return if (lazyComponent._status == Resolved) lazyComponent._result else null;
// [ts] }
// [ts] fn getWrappedName(outerType: auto, innerType: auto, wrapperName: auto) {
// [ts] let functionName = innerType.displayName || innerType.name || "";
// [ts] return outerType.displayName || (if (functionName != "") wrapperName + "(" + functionName + ")" else wrapperName);
// [ts] }
// [ts] fn getComponentName(type: auto) {
// [ts] if (type == null) {
// Host root, text node or just invalid type.
// [ts] return null;
// [ts] }
// [ts] {
// [ts] if (@typeName(type.tag) == "number") {
// [ts] error("Received an unexpected object in getComponentName(). " + "This is likely a bug in React. Please file an issue.");
// [ts] }
// [ts] }
// [ts] if (@typeName(type) == "function") {
// [ts] return type.displayName || type.name || null;
// [ts] }
// [ts] if (@typeName(type) == "string") {
// [ts] return type;
// [ts] }
// [ts] switch (type) {
// [ts] REACT_FRAGMENT_TYPE => {
// [ts] return "Fragment";
// [ts] },
// [ts] REACT_PORTAL_TYPE => {
// [ts] return "Portal";
// [ts] },
// [ts] REACT_PROFILER_TYPE => {
// [ts] return "Profiler";
// [ts] },
// [ts] REACT_STRICT_MODE_TYPE => {
// [ts] return "StrictMode";
// [ts] },
// [ts] REACT_SUSPENSE_TYPE => {
// [ts] return "Suspense";
// [ts] },
// [ts] REACT_SUSPENSE_LIST_TYPE => {
// [ts] return "SuspenseList";
// [ts] }
// [ts] }
// [ts] if (@typeName(type) == "object") {
// [ts] switch (type.$$typeof) {
// [ts] REACT_CONTEXT_TYPE => {
// [ts] return "Context.Consumer";
// [ts] },
// [ts] REACT_PROVIDER_TYPE => {
// [ts] return "Context.Provider";
// [ts] },
// [ts] REACT_FORWARD_REF_TYPE => {
// [ts] return getWrappedName(type, type.render, "ForwardRef");
// [ts] },
// [ts] REACT_MEMO_TYPE => {
// [ts] return getComponentName(type.type);
// [ts] },
// [ts] REACT_BLOCK_TYPE => {
// [ts] return getComponentName(type.render);
// [ts] },
// [ts] REACT_LAZY_TYPE => {
// [ts] {
// [ts] let thenable = type;
// [ts] let resolvedThenable = refineResolvedLazyComponent(thenable);
// [ts] if (resolvedThenable) {
// [ts] return getComponentName(resolvedThenable);
// [ts] }
// [ts] break;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] return null;
// [ts] }
// [ts] let ReactDebugCurrentFrame = {};
// [ts] let currentlyValidatingElement = null;
// [ts] fn setCurrentlyValidatingElement(element: auto) {
// [ts] {
// [ts] currentlyValidatingElement = element;
// [ts] }
// [ts] }
// [ts] {
// Stack implementation injected by the current renderer.
// [ts] ReactDebugCurrentFrame.getCurrentStack = null;
// [ts] ReactDebugCurrentFrame.getStackAddendum = () => {
// [ts] let stack = ""; // Add an extra top frame while an element is being validated
// [ts] if (currentlyValidatingElement) {
// [ts] let name = getComponentName(currentlyValidatingElement.type);
// [ts] let owner = currentlyValidatingElement._owner;
// [ts] stack += describeComponentFrame(name, currentlyValidatingElement._source, owner && getComponentName(owner.type));
// [ts] } // Delegate to the injected renderer-specific implementation
// [ts] let impl = ReactDebugCurrentFrame.getCurrentStack;
// [ts] if (impl) {
// [ts] stack += impl() || "";
// [ts] }
// [ts] return stack;
// [ts] };
// [ts] }
// [ts] /**
// [ts] * Used by act() to track whether you're inside an act() scope.
// [ts] */
// [ts] let IsSomeRendererActing = {
// [ts] current: false
// [ts] };
// [ts] let ReactSharedInternals = {
// [ts] ReactCurrentDispatcher: ReactCurrentDispatcher,
// [ts] ReactCurrentBatchConfig: ReactCurrentBatchConfig,
// [ts] ReactCurrentOwner: ReactCurrentOwner,
// [ts] IsSomeRendererActing: IsSomeRendererActing,
// Used by renderers to avoid bundling object-assign twice in UMD bundles:
// [ts] assign: objectAssign
// [ts] };
// [ts] {
// [ts] objectAssign(ReactSharedInternals, {
// These should not be included in production.
// [ts] ReactDebugCurrentFrame: ReactDebugCurrentFrame,
// Shim for React DOM 16.0.0 which still destructured (but not used) this.
// TODO: remove in React 17.0.
// [ts] ReactComponentTreeHook: {}
// [ts] });
// [ts] }
// by calls to these methods by a Babel plugin.
//
// In PROD (or in packages without access to React internals),
// they are left as they are instead.
// [ts] fn warn(format: auto) {
// [ts] {
// [ts] let _len = arguments.length, args = Array.init(if (_len > 1) _len - 1 else 0), _key = 1;
// [ts] while (_key < _len) { // [cap-migrate] for-step: _key++
// [ts] args[_key - 1] = arguments[_key];
// [ts] }
// [ts] printWarning("warn", format, args);
// [ts] }
// [ts] }
// [ts] fn error(format: auto) {
// [ts] {
// [ts] let _len2 = arguments.length, args = Array.init(if (_len2 > 1) _len2 - 1 else 0), _key2 = 1;
// [ts] while (_key2 < _len2) { // [cap-migrate] for-step: _key2++
// [ts] args[_key2 - 1] = arguments[_key2];
// [ts] }
// [ts] printWarning("error", format, args);
// [ts] }
// [ts] }
// [ts] fn printWarning(level: auto, format: auto, args: auto) {
// When changing this logic, you might want to also
// update consoleWithStackDev.www.js as well.
// [ts] {
// [ts] let hasExistingStack = args.length > 0 && @typeName(args[args.length - 1]) == "string" && args[args.length - 1].indexOf("\n    in") == 0;
// [ts] if (!hasExistingStack) {
// [ts] let ReactDebugCurrentFrame = ReactSharedInternals.ReactDebugCurrentFrame;
// [ts] let stack = ReactDebugCurrentFrame.getStackAddendum();
// [ts] if (stack != "") {
// [ts] format += "%s";
// [ts] args = args.concat([stack]);
// [ts] }
// [ts] }
// [ts] let argsWithFormat = args.map((item) => {
// [ts] return "" + item;
// [ts] }); // Careful: RN currently depends on this prefix
// [ts] argsWithFormat.unshift("Warning: " + format); // We intentionally don't use spread (or .apply) directly because it
// breaks IE9: https://github.com/facebook/react/issues/13610
// eslint-disable-next-line react-internal/no-production-logging
// [ts] Function.prototype.apply.call(console[level], console, argsWithFormat);
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// --- Welcome to debugging React ---
// This error was thrown as a convenience so that you can use this stack
// to find the callsite that caused this warning to fire.
// [ts] let argIndex = 0;
// [ts] let message = "Warning: " + format.replace("%s", () => {
// [ts] return args[argIndex++];
// [ts] });
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(message);)
// [ts] } // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (x) {})
// [ts] }
// [ts] }
// [ts] let didWarnStateUpdateForUnmountedComponent = {};
// [ts] fn warnNoop(publicInstance: auto, callerName: auto) {
// [ts] {
// [ts] let _constructor = publicInstance.constructor;
// [ts] let componentName = _constructor && (_constructor.displayName || _constructor.name) || "ReactClass";
// [ts] let warningKey = componentName + "." + callerName;
// [ts] if (didWarnStateUpdateForUnmountedComponent[warningKey]) {
// [ts] return;
// [ts] }
// [ts] error("Can't call %s on a component that is not yet mounted. " + "This is a no-op, but it might indicate a bug in your application. " + "Instead, assign to `this.state` directly or define a `state = {};` " + "class property with the desired state in the %s component.", callerName, componentName);
// [ts] didWarnStateUpdateForUnmountedComponent[warningKey] = true;
// [ts] }
// [ts] }
// [ts] /**
// [ts] * This is the abstract API for an update queue.
// [ts] */
// [ts] let ReactNoopUpdateQueue = {
// [ts] /**
// [ts] * Checks whether or not this composite component is mounted.
// [ts] * @param {ReactClass} publicInstance The instance we want to test.
// [ts] * @return {boolean} True if mounted, false otherwise.
// [ts] * @protected
// [ts] * @final
// [ts] */
// [ts] isMounted: (publicInstance) => {
// [ts] return false;
// [ts] },
// [ts] /**
// [ts] * Forces an update. This should only be invoked when it is known with
// [ts] * certainty that we are **not** in a DOM transaction.
// [ts] *
// [ts] * You may want to call this when you know that some deeper aspect of the
// [ts] * component's state has changed but `setState` was not called.
// [ts] *
// [ts] * This will not invoke `shouldComponentUpdate`, but it will invoke
// [ts] * `componentWillUpdate` and `componentDidUpdate`.
// [ts] *
// [ts] * @param {ReactClass} publicInstance The instance that should rerender.
// [ts] * @param {?function} callback Called after component is updated.
// [ts] * @param {?string} callerName name of the calling function in the public API.
// [ts] * @internal
// [ts] */
// [ts] enqueueForceUpdate: (publicInstance, callback, callerName) => {
// [ts] warnNoop(publicInstance, "forceUpdate");
// [ts] },
// [ts] /**
// [ts] * Replaces all of the state. Always use this or `setState` to mutate state.
// [ts] * You should treat `this.state` as immutable.
// [ts] *
// [ts] * There is no guarantee that `this.state` will be immediately updated, so
// [ts] * accessing `this.state` after calling this method may return the old value.
// [ts] *
// [ts] * @param {ReactClass} publicInstance The instance that should rerender.
// [ts] * @param {object} completeState Next state.
// [ts] * @param {?function} callback Called after component is updated.
// [ts] * @param {?string} callerName name of the calling function in the public API.
// [ts] * @internal
// [ts] */
// [ts] enqueueReplaceState: (publicInstance, completeState, callback, callerName) => {
// [ts] warnNoop(publicInstance, "replaceState");
// [ts] },
// [ts] /**
// [ts] * Sets a subset of the state. This only exists because _pendingState is
// [ts] * internal. This provides a merging strategy that is not available to deep
// [ts] * properties which is confusing. TODO: Expose pendingState or don't use it
// [ts] * during the merge.
// [ts] *
// [ts] * @param {ReactClass} publicInstance The instance that should rerender.
// [ts] * @param {object} partialState Next partial state to be merged with state.
// [ts] * @param {?function} callback Called after component is updated.
// [ts] * @param {?string} Name of the calling function in the public API.
// [ts] * @internal
// [ts] */
// [ts] enqueueSetState: (publicInstance, partialState, callback, callerName) => {
// [ts] warnNoop(publicInstance, "setState");
// [ts] }
// [ts] };
// [ts] let emptyObject = {};
// [ts] {
// [ts] Object.freeze(emptyObject);
// [ts] }
// [ts] /**
// [ts] * Base class helpers for the updating state of a component.
// [ts] */
// [ts] fn Component(props: auto, context: auto, updater: auto) {
// [ts] _.props = props;
// [ts] _.context = context; // If a component has string refs, we will assign a different object later.
// [ts] _.refs = emptyObject; // We initialize the default updater but the real one gets injected by the
// renderer.
// [ts] _.updater = updater || ReactNoopUpdateQueue;
// [ts] }
// [ts] Component.prototype.isReactComponent = {};
// [ts] /**
// [ts] * Sets a subset of the state. Always use this to mutate
// [ts] * state. You should treat `this.state` as immutable.
// [ts] *
// [ts] * There is no guarantee that `this.state` will be immediately updated, so
// [ts] * accessing `this.state` after calling this method may return the old value.
// [ts] *
// [ts] * There is no guarantee that calls to `setState` will run synchronously,
// [ts] * as they may eventually be batched together.  You can provide an optional
// [ts] * callback that will be executed when the call to setState is actually
// [ts] * completed.
// [ts] *
// [ts] * When a function is provided to setState, it will be called at some point in
// [ts] * the future (not synchronously). It will be called with the up to date
// [ts] * component arguments (state, props, context). These values can be different
// [ts] * from this.* because your function may be called after receiveProps but before
// [ts] * shouldComponentUpdate, and this new state, props, and context will not yet be
// [ts] * assigned to this.
// [ts] *
// [ts] * @param {object|function} partialState Next partial state or function to
// [ts] *        produce next partial state to be merged with current state.
// [ts] * @param {?function} callback Called after state is updated.
// [ts] * @final
// [ts] * @protected
// [ts] */
// [ts] Component.prototype.setState = (partialState, callback) => {
// [ts] if (!(@typeName(partialState) == "object" || @typeName(partialState) == "function" || partialState == null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "setState(...): takes an object of state variables to update or a function which returns an object of state variables." );)
// [ts] }
// [ts] }
// [ts] _.updater.enqueueSetState(this, partialState, callback, "setState");
// [ts] };
// [ts] /**
// [ts] * Forces an update. This should only be invoked when it is known with
// [ts] * certainty that we are **not** in a DOM transaction.
// [ts] *
// [ts] * You may want to call this when you know that some deeper aspect of the
// [ts] * component's state has changed but `setState` was not called.
// [ts] *
// [ts] * This will not invoke `shouldComponentUpdate`, but it will invoke
// [ts] * `componentWillUpdate` and `componentDidUpdate`.
// [ts] *
// [ts] * @param {?function} callback Called after update is complete.
// [ts] * @final
// [ts] * @protected
// [ts] */
// [ts] Component.prototype.forceUpdate = (callback) => {
// [ts] _.updater.enqueueForceUpdate(this, callback, "forceUpdate");
// [ts] };
// [ts] /**
// [ts] * Deprecated APIs. These APIs used to exist on classic React classes but since
// [ts] * we would like to deprecate them, we're not going to move them over to this
// [ts] * modern base class. Instead, we define a getter that warns if it's accessed.
// [ts] */
// [ts] {
// [ts] let deprecatedAPIs = {
// [ts] isMounted: ["isMounted", "Instead, make sure to clean up subscriptions and pending requests in " + "componentWillUnmount to prevent memory leaks."],
// [ts] replaceState: ["replaceState", "Refactor your code to use setState instead (see " + "https://github.com/facebook/react/issues/3236)."]
// [ts] };
// [ts] let defineDeprecationWarning = (methodName, info) => {
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(Component.prototype, methodName, {
// [ts-skip] get: function () {
// [ts-skip] warn('%s(...) is deprecated in plain JavaScript React classes. %s', info[0], info[1]);
// [ts-skip]
// [ts-skip] return undefined;
// [ts-skip] }
// [ts-skip] });
// [ts] };
// [ts] for (let fnName in deprecatedAPIs) {
// [ts] if (deprecatedAPIs.hasOwnProperty(fnName)) {
// [ts] defineDeprecationWarning(fnName, deprecatedAPIs[fnName]);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn ComponentDummy() {}
// [ts] ComponentDummy.prototype = Component.prototype;
// [ts] /**
// [ts] * Convenience component with default shallow equality check for sCU.
// [ts] */
// [ts] fn PureComponent(props: auto, context: auto, updater: auto) {
// [ts] _.props = props;
// [ts] _.context = context; // If a component has string refs, we will assign a different object later.
// [ts] _.refs = emptyObject;
// [ts] _.updater = updater || ReactNoopUpdateQueue;
// [ts] }
// [ts] let pureComponentPrototype = PureComponent.prototype = ComponentDummy.init();
// [ts] pureComponentPrototype.constructor = PureComponent; // Avoid an extra prototype jump for these methods.
// [ts] objectAssign(pureComponentPrototype, Component.prototype);
// [ts] pureComponentPrototype.isPureReactComponent = true;
// an immutable object with a single mutable value
// [ts] fn createRef() {
// [ts] let refObject = {
// [ts] current: null
// [ts] };
// [ts] {
// [ts] Object.seal(refObject);
// [ts] }
// [ts] return refObject;
// [ts] }
// [ts] let hasOwnProperty$1 = Object.prototype.hasOwnProperty;
// [ts] let RESERVED_PROPS = {
// [ts] key: true,
// [ts] ref: true,
// [ts] __self: true,
// [ts] __source: true
// [ts] };
// [ts] let specialPropKeyWarningShown, specialPropRefWarningShown, didWarnAboutStringRefs;
// [ts] {
// [ts] didWarnAboutStringRefs = {};
// [ts] }
// [ts] fn hasValidRef(config: auto) {
// [ts] {
// [ts] if (hasOwnProperty$1.call(config, "ref")) {
// [ts] let getter = Object.getOwnPropertyDescriptor(config, "ref").get;
// [ts] if (getter && getter.isReactWarning) {
// [ts] return false;
// [ts] }
// [ts] }
// [ts] }
// [ts] return config.ref != null;
// [ts] }
// [ts] fn hasValidKey(config: auto) {
// [ts] {
// [ts] if (hasOwnProperty$1.call(config, "key")) {
// [ts] let getter = Object.getOwnPropertyDescriptor(config, "key").get;
// [ts] if (getter && getter.isReactWarning) {
// [ts] return false;
// [ts] }
// [ts] }
// [ts] }
// [ts] return config.key != null;
// [ts] }
// [ts] fn defineKeyPropWarningGetter(props: auto, displayName: auto) {
// [ts] let warnAboutAccessingKey = () => {
// [ts] {
// [ts] if (!specialPropKeyWarningShown) {
// [ts] specialPropKeyWarningShown = true;
// [ts] error("%s: `key` is not a prop. Trying to access it will result " + "in `undefined` being returned. If you need to access the same " + "value within the child component, you should pass it as a different " + "prop. (https://fb.me/react-special-props)", displayName);
// [ts] }
// [ts] }
// [ts] };
// [ts] warnAboutAccessingKey.isReactWarning = true;
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(props, 'key', {
// [ts-skip] get: warnAboutAccessingKey,
// [ts-skip] configurable: true
// [ts-skip] });
// [ts] }
// [ts] fn defineRefPropWarningGetter(props: auto, displayName: auto) {
// [ts] let warnAboutAccessingRef = () => {
// [ts] {
// [ts] if (!specialPropRefWarningShown) {
// [ts] specialPropRefWarningShown = true;
// [ts] error("%s: `ref` is not a prop. Trying to access it will result " + "in `undefined` being returned. If you need to access the same " + "value within the child component, you should pass it as a different " + "prop. (https://fb.me/react-special-props)", displayName);
// [ts] }
// [ts] }
// [ts] };
// [ts] warnAboutAccessingRef.isReactWarning = true;
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(props, 'ref', {
// [ts-skip] get: warnAboutAccessingRef,
// [ts-skip] configurable: true
// [ts-skip] });
// [ts] }
// [ts] fn warnIfStringRefCannotBeAutoConverted(config: auto) {
// [ts] {
// [ts] if (@typeName(config.ref) == "string" && ReactCurrentOwner.current && config.__self && ReactCurrentOwner.current.stateNode != config.__self) {
// [ts] let componentName = getComponentName(ReactCurrentOwner.current.type);
// [ts] if (!didWarnAboutStringRefs[componentName]) {
// [ts] error("Component \"%s\" contains the string ref \"%s\". " + "Support for string refs will be removed in a future major release. " + "This case cannot be automatically converted to an arrow function. " + "We ask you to manually fix this case by using useRef() or createRef() instead. " + "Learn more about using refs safely here: " + "https://fb.me/react-strict-mode-string-ref", getComponentName(ReactCurrentOwner.current.type), config.ref);
// [ts] didWarnAboutStringRefs[componentName] = true;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Factory method to create a new React element. This no longer adheres to
// [ts] * the class pattern, so do not use new to call it. Also, instanceof check
// [ts] * will not work. Instead test $$typeof field against Symbol.for('react.element') to check
// [ts] * if something is a React Element.
// [ts] *
// [ts] * @param {*} type
// [ts] * @param {*} props
// [ts] * @param {*} key
// [ts] * @param {string|object} ref
// [ts] * @param {*} owner
// [ts] * @param {*} self A *temporary* helper to detect places where `this` is
// [ts] * different from the `owner` when React.createElement is called, so that we
// [ts] * can warn. We want to get rid of owner and replace string `ref`s with arrow
// [ts] * functions, and as long as `this` and owner are the same, there will be no
// [ts] * change in behavior.
// [ts] * @param {*} source An annotation object (added by a transpiler or otherwise)
// [ts] * indicating filename, line number, and/or other information.
// [ts] * @internal
// [ts] */
// [ts] let ReactElement = (type, key, ref, self, source, owner, props) => {
// [ts] let element = {
// This tag allows us to uniquely identify this as a React Element
// [ts] $$typeof: REACT_ELEMENT_TYPE,
// Built-in properties that belong on the element
// [ts] type: type,
// [ts] key: key,
// [ts] ref: ref,
// [ts] props: props,
// Record the component responsible for creating this element.
// [ts] _owner: owner
// [ts] };
// [ts] {
// The validation flag is currently mutative. We put it on
// an external backing store so that we can freeze the whole object.
// This can be replaced with a WeakMap once they are implemented in
// commonly used development environments.
// [ts] element._store = {}; // To make comparing ReactElements easier for testing purposes, we make
// the validation flag non-enumerable (where possible, which should
// include every environment we run tests in), so the test framework
// ignores it.
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(element._store, 'validated', {
// [ts-skip] configurable: false,
// [ts-skip] enumerable: false,
// [ts-skip] writable: true,
// [ts-skip] value: false
// [ts-skip] }); // self and source are DEV only properties.
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(element, '_self', {
// [ts-skip] configurable: false,
// [ts-skip] enumerable: false,
// [ts-skip] writable: false,
// [ts-skip] value: self
// [ts-skip] }); // Two elements created in two different places should be considered
// equal for testing purposes and therefore we hide it from enumeration.
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(element, '_source', {
// [ts-skip] configurable: false,
// [ts-skip] enumerable: false,
// [ts-skip] writable: false,
// [ts-skip] value: source
// [ts-skip] });
// [ts] if (Object.freeze) {
// [ts] Object.freeze(element.props);
// [ts] Object.freeze(element);
// [ts] }
// [ts] }
// [ts] return element;
// [ts] };
// [ts] /**
// [ts] * Create and return a new ReactElement of the given type.
// [ts] * See https://reactjs.org/docs/react-api.html#createelement
// [ts] */
// [ts] fn createElement(type: auto, config: auto, children: auto) {
// [ts] let propName; // Reserved names are extracted
// [ts] let props = {};
// [ts] let key = null;
// [ts] let ref = null;
// [ts] let self = null;
// [ts] let source = null;
// [ts] if (config != null) {
// [ts] if (hasValidRef(config)) {
// [ts] ref = config.ref;
// [ts] {
// [ts] warnIfStringRefCannotBeAutoConverted(config);
// [ts] }
// [ts] }
// [ts] if (hasValidKey(config)) {
// [ts] key = "" + config.key;
// [ts] }
// [ts] self = if (config.__self == null) null else config.__self;
// [ts] source = if (config.__source == null) null else config.__source; // Remaining properties are added to a new props object
// [ts] for (propName in config) {
// [ts] if (hasOwnProperty$1.call(config, propName) && !RESERVED_PROPS.hasOwnProperty(propName)) {
// [ts] props[propName] = config[propName];
// [ts] }
// [ts] }
// [ts] } // Children can be more than one argument, and those are transferred onto
// the newly allocated props object.
// [ts] let childrenLength = arguments.length - 2;
// [ts] if (childrenLength == 1) {
// [ts] props.children = children;
// [ts] } else if (childrenLength > 1) {
// [ts] let childArray = Array(childrenLength);
// [ts] let i = 0;
// [ts] while (i < childrenLength) { // [cap-migrate] for-step: i++
// [ts] childArray[i] = arguments[i + 2];
// [ts] }
// [ts] {
// [ts] if (Object.freeze) {
// [ts] Object.freeze(childArray);
// [ts] }
// [ts] }
// [ts] props.children = childArray;
// [ts] } // Resolve default props
// [ts] if (type && type.defaultProps) {
// [ts] let defaultProps = type.defaultProps;
// [ts] for (propName in defaultProps) {
// [ts] if (props[propName] == null) {
// [ts] props[propName] = defaultProps[propName];
// [ts] }
// [ts] }
// [ts] }
// [ts] {
// [ts] if (key || ref) {
// [ts] let displayName = if (@typeName(type) == "function") type.displayName || type.name || "Unknown" else type;
// [ts] if (key) {
// [ts] defineKeyPropWarningGetter(props, displayName);
// [ts] }
// [ts] if (ref) {
// [ts] defineRefPropWarningGetter(props, displayName);
// [ts] }
// [ts] }
// [ts] }
// [ts] return ReactElement(type, key, ref, self, source, ReactCurrentOwner.current, props);
// [ts] }
// [ts] fn cloneAndReplaceKey(oldElement: auto, newKey: auto) {
// [ts] let newElement = ReactElement(oldElement.type, newKey, oldElement.ref, oldElement._self, oldElement._source, oldElement._owner, oldElement.props);
// [ts] return newElement;
// [ts] }
// [ts] /**
// [ts] * Clone and return a new ReactElement using element as the starting point.
// [ts] * See https://reactjs.org/docs/react-api.html#cloneelement
// [ts] */
// [ts] fn cloneElement(element: auto, config: auto, children: auto) {
// [ts] if (!!(element == null || element == null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "React.cloneElement(...): The argument must be a React element, but you passed " + element + "." );)
// [ts] }
// [ts] }
// [ts] let propName; // Original props are copied
// [ts] let props = objectAssign({}, element.props); // Reserved names are extracted
// [ts] let key = element.key;
// [ts] let ref = element.ref; // Self is preserved since the owner is preserved.
// [ts] let self = element._self; // Source is preserved since cloneElement is unlikely to be targeted by a
// transpiler, and the original source is probably a better indicator of the
// true owner.
// [ts] let source = element._source; // Owner will be preserved, unless ref is overridden
// [ts] let owner = element._owner;
// [ts] if (config != null) {
// [ts] if (hasValidRef(config)) {
// Silently steal the ref from the parent.
// [ts] ref = config.ref;
// [ts] owner = ReactCurrentOwner.current;
// [ts] }
// [ts] if (hasValidKey(config)) {
// [ts] key = "" + config.key;
// [ts] } // Remaining properties override existing props
// [ts] let defaultProps;
// [ts] if (element.type && element.type.defaultProps) {
// [ts] defaultProps = element.type.defaultProps;
// [ts] }
// [ts] for (propName in config) {
// [ts] if (hasOwnProperty$1.call(config, propName) && !RESERVED_PROPS.hasOwnProperty(propName)) {
// [ts] if (config[propName] == null && defaultProps != null) {
// Resolve default props
// [ts] props[propName] = defaultProps[propName];
// [ts] } else {
// [ts] props[propName] = config[propName];
// [ts] }
// [ts] }
// [ts] }
// [ts] } // Children can be more than one argument, and those are transferred onto
// the newly allocated props object.
// [ts] let childrenLength = arguments.length - 2;
// [ts] if (childrenLength == 1) {
// [ts] props.children = children;
// [ts] } else if (childrenLength > 1) {
// [ts] let childArray = Array(childrenLength);
// [ts] let i = 0;
// [ts] while (i < childrenLength) { // [cap-migrate] for-step: i++
// [ts] childArray[i] = arguments[i + 2];
// [ts] }
// [ts] props.children = childArray;
// [ts] }
// [ts] return ReactElement(element.type, key, ref, self, source, owner, props);
// [ts] }
// [ts] /**
// [ts] * Verifies the object is a ReactElement.
// [ts] * See https://reactjs.org/docs/react-api.html#isvalidelement
// [ts] * @param {?object} object
// [ts] * @return {boolean} True if `object` is a ReactElement.
// [ts] * @final
// [ts] */
// [ts] fn isValidElement(object: auto) {
// [ts] return @typeName(object) == "object" && object != null && object.$$typeof == REACT_ELEMENT_TYPE;
// [ts] }
// [ts] let SEPARATOR = ".";
// [ts] let SUBSEPARATOR = ":";
// [ts] /**
// [ts] * Escape and wrap key so it is safe to use as a reactid
// [ts] *
// [ts] * @param {string} key to be escaped.
// [ts] * @return {string} the escaped key.
// [ts] */
// [ts] fn escape(key: auto) {
// [ts] let escapeRegex = "[=:]";
// [ts] let escaperLookup = {
// [ts] "=": "=0",
// [ts] ":": "=2"
// [ts] };
// [ts] let escapedString = ("" + key).replace(escapeRegex, (match_val) => {
// [ts] return escaperLookup[match_val];
// [ts] });
// [ts] return "$" + escapedString;
// [ts] }
// [ts] /**
// [ts] * TODO: Test that a single child and an array with one item have the same key
// [ts] * pattern.
// [ts] */
// [ts] let didWarnAboutMaps = false;
// [ts] let userProvidedKeyEscapeRegex = "\\/+";
// [ts] fn escapeUserProvidedKey(text: auto) {
// [ts] return ("" + text).replace(userProvidedKeyEscapeRegex, "$&/");
// [ts] }
// [ts] let POOL_SIZE = 10;
// [ts] let traverseContextPool = [];
// [ts] fn getPooledTraverseContext(mapResult: auto, keyPrefix: auto, mapFunction: auto, mapContext: auto) {
// [ts] if (traverseContextPool.length) {
// [ts] let traverseContext = traverseContextPool.pop();
// [ts] traverseContext.result = mapResult;
// [ts] traverseContext.keyPrefix = keyPrefix;
// [ts] traverseContext.func = mapFunction;
// [ts] traverseContext.context = mapContext;
// [ts] traverseContext.count = 0;
// [ts] return traverseContext;
// [ts] } else {
// [ts] return {
// [ts] result: mapResult,
// [ts] keyPrefix: keyPrefix,
// [ts] func: mapFunction,
// [ts] context: mapContext,
// [ts] count: 0
// [ts] };
// [ts] }
// [ts] }
// [ts] fn releaseTraverseContext(traverseContext: auto) {
// [ts] traverseContext.result = null;
// [ts] traverseContext.keyPrefix = null;
// [ts] traverseContext.func = null;
// [ts] traverseContext.context = null;
// [ts] traverseContext.count = 0;
// [ts] if (traverseContextPool.length < POOL_SIZE) {
// [ts] traverseContextPool.push(traverseContext);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * @param {?*} children Children tree container.
// [ts] * @param {!string} nameSoFar Name of the key path so far.
// [ts] * @param {!function} callback Callback to invoke with each child found.
// [ts] * @param {?*} traverseContext Used to pass information throughout the traversal
// [ts] * process.
// [ts] * @return {!number} The number of children in this subtree.
// [ts] */
// [ts] fn traverseAllChildrenImpl(children: auto, nameSoFar: auto, callback: auto, traverseContext: auto) {
// [ts] let type = @typeName(children);
// [ts] if (type == "undefined" || type == "boolean") {
// All of the above are perceived as null.
// [ts] children = null;
// [ts] }
// [ts] let invokeCallback = false;
// [ts] if (children == null) {
// [ts] invokeCallback = true;
// [ts] } else {
// [ts] switch (type) {
// [ts] "string", "number" => {
// [ts] invokeCallback = true;
// break;
// [ts] },
// [ts] switch (children.$$typeof) {
// [ts] REACT_ELEMENT_TYPE, REACT_PORTAL_TYPE => {
// [ts] invokeCallback = true;
// [ts] }
// [ts] }
// [ts] "object" => {}
// [ts] }
// [ts] }
// [ts] if (invokeCallback) {
// [ts] callback(traverseContext, children, // If it's the only child, treat the name as if it was wrapped in an array
// so that it's consistent if the number of children grows.
// [ts] if (nameSoFar == "") SEPARATOR + getComponentKey(children, 0) else nameSoFar);
// [ts] return 1;
// [ts] }
// [ts] let child;
// [ts] let nextName;
// [ts] let subtreeCount = 0; // Count of children found in the current subtree.
// [ts] let nextNamePrefix = if (nameSoFar == "") SEPARATOR else nameSoFar + SUBSEPARATOR;
// [ts] if (Array.isArray(children)) {
// [ts] let i = 0;
// [ts] while (i < children.length) { // [cap-migrate] for-step: i++
// [ts] child = children[i];
// [ts] nextName = nextNamePrefix + getComponentKey(child, i);
// [ts] subtreeCount += traverseAllChildrenImpl(child, nextName, callback, traverseContext);
// [ts] }
// [ts] } else {
// [ts] let iteratorFn = getIteratorFn(children);
// [ts] if (@typeName(iteratorFn) == "function") {
// [ts] {
// Warn about using Maps as children
// [ts] if (iteratorFn == children.entries) {
// [ts] if (!didWarnAboutMaps) {
// [ts] warn("Using Maps as children is deprecated and will be removed in " + "a future major release. Consider converting children to " + "an array of keyed ReactElements instead.");
// [ts] }
// [ts] didWarnAboutMaps = true;
// [ts] }
// [ts] }
// [ts] let iterator = iteratorFn.call(children);
// [ts] let step;
// [ts] let ii = 0;
// [ts] while (!(step = iterator.next()).done) {
// [ts] child = step.value;
// [ts] nextName = nextNamePrefix + getComponentKey(child, ii++);
// [ts] subtreeCount += traverseAllChildrenImpl(child, nextName, callback, traverseContext);
// [ts] }
// [ts] } else if (type == "object") {
// [ts] let addendum = "";
// [ts] {
// [ts] addendum = " If you meant to render a collection of children, use an array " + "instead." + ReactDebugCurrentFrame.getStackAddendum();
// [ts] }
// [ts] let childrenString = "" + children;
// [ts] {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Objects are not valid as a React child (found: " + (childrenString === '[object Object]' ? 'object with keys {' + Object.keys(children).join(', ') + '}' : childrenString) + ")." + addendum );)
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] return subtreeCount;
// [ts] }
// [ts] /**
// [ts] * Traverses children that are typically specified as `props.children`, but
// [ts] * might also be specified through attributes:
// [ts] *
// [ts] * - `traverseAllChildren(this.props.children, ...)`
// [ts] * - `traverseAllChildren(this.props.leftPanelChildren, ...)`
// [ts] *
// [ts] * The `traverseContext` is an optional argument that is passed through the
// [ts] * entire traversal. It can be used to store accumulations or anything else that
// [ts] * the callback might find relevant.
// [ts] *
// [ts] * @param {?*} children Children tree object.
// [ts] * @param {!function} callback To invoke upon traversing each child.
// [ts] * @param {?*} traverseContext Context for traversal.
// [ts] * @return {!number} The number of children in this subtree.
// [ts] */
// [ts] fn traverseAllChildren(children: auto, callback: auto, traverseContext: auto) {
// [ts] if (children == null) {
// [ts] return 0;
// [ts] }
// [ts] return traverseAllChildrenImpl(children, "", callback, traverseContext);
// [ts] }
// [ts] /**
// [ts] * Generate a key string that identifies a component within a set.
// [ts] *
// [ts] * @param {*} component A component that could contain a manual key.
// [ts] * @param {number} index Index that is used if a manual key is not provided.
// [ts] * @return {string}
// [ts] */
// [ts] fn getComponentKey(component: auto, index: auto) {
// Do some typechecking here since we call this blindly. We want to ensure
// that we don't block potential future ES APIs.
// [ts] if (@typeName(component) == "object" && component != null && component.key != null) {
// Explicit key
// [ts] return escape(component.key);
// [ts] } // Implicit key determined by the index in the set
// [ts] return index.toString(36);
// [ts] }
// [ts] fn forEachSingleChild(bookKeeping: auto, child: auto, name: auto) {
// [ts] let func = bookKeeping.func,
// [ts] context = bookKeeping.context;
// [ts] func.call(context, child, bookKeeping.count++);
// [ts] }
// [ts] /**
// [ts] * Iterates through children that are typically specified as `props.children`.
// [ts] *
// [ts] * See https://reactjs.org/docs/react-api.html#reactchildrenforeach
// [ts] *
// [ts] * The provided forEachFunc(child, index) will be called for each
// [ts] * leaf child.
// [ts] *
// [ts] * @param {?*} children Children tree container.
// [ts] * @param {function(*, int)} forEachFunc
// [ts] * @param {*} forEachContext Context for forEachContext.
// [ts] */
// [ts] fn forEachChildren(children: auto, forEachFunc: auto, forEachContext: auto) {
// [ts] if (children == null) {
// [ts] return children;
// [ts] }
// [ts] let traverseContext = getPooledTraverseContext(null, null, forEachFunc, forEachContext);
// [ts] traverseAllChildren(children, forEachSingleChild, traverseContext);
// [ts] releaseTraverseContext(traverseContext);
// [ts] }
// [ts] fn mapSingleChildIntoContext(bookKeeping: auto, child: auto, childKey: auto) {
// [ts] let result = bookKeeping.result,
// [ts] keyPrefix = bookKeeping.keyPrefix,
// [ts] func = bookKeeping.func,
// [ts] context = bookKeeping.context;
// [ts] let mappedChild = func.call(context, child, bookKeeping.count++);
// [ts] if (Array.isArray(mappedChild)) {
// [ts] mapIntoWithKeyPrefixInternal(mappedChild, result, childKey, (c) => {
// [ts] return c;
// [ts] });
// [ts] } else if (mappedChild != null) {
// [ts] if (isValidElement(mappedChild)) {
// [ts] mappedChild = cloneAndReplaceKey(mappedChild, // Keep both the (mapped) and old keys if they differ, just as
// traverseAllChildren used to do for objects as children
// [ts] keyPrefix + (if (mappedChild.key && (!child || child.key != mappedChild.key)) escapeUserProvidedKey(mappedChild.key) + "/" else "") + childKey);
// [ts] }
// [ts] result.push(mappedChild);
// [ts] }
// [ts] }
// [ts] fn mapIntoWithKeyPrefixInternal(children: auto, array: auto, prefix: auto, func: auto, context: auto) {
// [ts] let escapedPrefix = "";
// [ts] if (prefix != null) {
// [ts] escapedPrefix = escapeUserProvidedKey(prefix) + "/";
// [ts] }
// [ts] let traverseContext = getPooledTraverseContext(array, escapedPrefix, func, context);
// [ts] traverseAllChildren(children, mapSingleChildIntoContext, traverseContext);
// [ts] releaseTraverseContext(traverseContext);
// [ts] }
// [ts] /**
// [ts] * Maps children that are typically specified as `props.children`.
// [ts] *
// [ts] * See https://reactjs.org/docs/react-api.html#reactchildrenmap
// [ts] *
// [ts] * The provided mapFunction(child, key, index) will be called for each
// [ts] * leaf child.
// [ts] *
// [ts] * @param {?*} children Children tree container.
// [ts] * @param {function(*, int)} func The map function.
// [ts] * @param {*} context Context for mapFunction.
// [ts] * @return {object} Object containing the ordered map of results.
// [ts] */
// [ts] fn mapChildren(children: auto, func: auto, context: auto) {
// [ts] if (children == null) {
// [ts] return children;
// [ts] }
// [ts] let result = [];
// [ts] mapIntoWithKeyPrefixInternal(children, result, null, func, context);
// [ts] return result;
// [ts] }
// [ts] /**
// [ts] * Count the number of children that are typically specified as
// [ts] * `props.children`.
// [ts] *
// [ts] * See https://reactjs.org/docs/react-api.html#reactchildrencount
// [ts] *
// [ts] * @param {?*} children Children tree container.
// [ts] * @return {number} The number of children.
// [ts] */
// [ts] fn countChildren(children: auto) {
// [ts] return traverseAllChildren(children, () => {
// [ts] return null;
// [ts] }, null);
// [ts] }
// [ts] /**
// [ts] * Flatten a children object (typically specified as `props.children`) and
// [ts] * return an array with appropriately re-keyed children.
// [ts] *
// [ts] * See https://reactjs.org/docs/react-api.html#reactchildrentoarray
// [ts] */
// [ts] fn toArray(children: auto) {
// [ts] let result = [];
// [ts] mapIntoWithKeyPrefixInternal(children, result, null, (child) => {
// [ts] return child;
// [ts] });
// [ts] return result;
// [ts] }
// [ts] /**
// [ts] * Returns the first child in a collection of children and verifies that there
// [ts] * is only one child in the collection.
// [ts] *
// [ts] * See https://reactjs.org/docs/react-api.html#reactchildrenonly
// [ts] *
// [ts] * The current implementation of this function assumes that a single child gets
// [ts] * passed without a wrapper, but the purpose of this helper function is to
// [ts] * abstract away the particular structure of children.
// [ts] *
// [ts] * @param {?object} children Child collection structure.
// [ts] * @return {ReactElement} The first and only `ReactElement` contained in the
// [ts] * structure.
// [ts] */
// [ts] fn onlyChild(children: auto) {
// [ts] if (!isValidElement(children)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "React.Children.only expected to receive a single React element child." );)
// [ts] }
// [ts] }
// [ts] return children;
// [ts] }
// [ts] fn createContext(defaultValue: auto, calculateChangedBits: auto) {
// [ts] if (calculateChangedBits == null) {
// [ts] calculateChangedBits = null;
// [ts] } else {
// [ts] {
// [ts] if (calculateChangedBits != null && @typeName(calculateChangedBits) != "function") {
// [ts] error("createContext: Expected the optional second argument to be a " + "function. Instead received: %s", calculateChangedBits);
// [ts] }
// [ts] }
// [ts] }
// [ts] let context = {
// [ts] $$typeof: REACT_CONTEXT_TYPE,
// [ts] _calculateChangedBits: calculateChangedBits,
// As a workaround to support multiple concurrent renderers, we categorize
// some renderers as primary and others as secondary. We only expect
// there to be two concurrent renderers at most: React Native (primary) and
// Fabric (secondary); React DOM (primary) and React ART (secondary).
// Secondary renderers store their context values on separate fields.
// [ts] _currentValue: defaultValue,
// [ts] _currentValue2: defaultValue,
// Used to track how many concurrent renderers this context currently
// supports within in a single renderer. Such as parallel server rendering.
// [ts] _threadCount: 0,
// These are circular
// [ts] Provider: null,
// [ts] Consumer: null
// [ts] };
// [ts] context.Provider = {
// [ts] $$typeof: REACT_PROVIDER_TYPE,
// [ts] _context: context
// [ts] };
// [ts] let hasWarnedAboutUsingNestedContextConsumers = false;
// [ts] let hasWarnedAboutUsingConsumerProvider = false;
// [ts] {
// A separate object, but proxies back to the original context object for
// backwards compatibility. It has a different $$typeof, so we can properly
// warn for the incorrect usage of Context as a Consumer.
// [ts] let Consumer = {
// [ts] $$typeof: REACT_CONTEXT_TYPE,
// [ts] _context: context,
// [ts] _calculateChangedBits: context._calculateChangedBits
// [ts] }; // $FlowFixMe: Flow complains about not setting a value, which is intentional here
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperties(Consumer, {
// [ts-skip] Provider: {
// [ts-skip] get: function () {
// [ts-skip] if (!hasWarnedAboutUsingConsumerProvider) {
// [ts-skip] hasWarnedAboutUsingConsumerProvider = true;
// [ts-skip]
// [ts-skip] error('Rendering <Context.Consumer.Provider> is not supported and will be removed in ' + 'a future major release. Did you mean to render <Context.Provider> instead?');
// [ts-skip] }
// [ts-skip]
// [ts-skip] return context.Provider;
// [ts-skip] },
// [ts-skip] set: function (_Provider) {
// [ts-skip] context.Provider = _Provider;
// [ts-skip] }
// [ts-skip] },
// [ts-skip] _currentValue: {
// [ts-skip] get: function () {
// [ts-skip] return context._currentValue;
// [ts-skip] },
// [ts-skip] set: function (_currentValue) {
// [ts-skip] context._currentValue = _currentValue;
// [ts-skip] }
// [ts-skip] },
// [ts-skip] _currentValue2: {
// [ts-skip] get: function () {
// [ts-skip] return context._currentValue2;
// [ts-skip] },
// [ts-skip] set: function (_currentValue2) {
// [ts-skip] context._currentValue2 = _currentValue2;
// [ts-skip] }
// [ts-skip] },
// [ts-skip] _threadCount: {
// [ts-skip] get: function () {
// [ts-skip] return context._threadCount;
// [ts-skip] },
// [ts-skip] set: function (_threadCount) {
// [ts-skip] context._threadCount = _threadCount;
// [ts-skip] }
// [ts-skip] },
// [ts-skip] Consumer: {
// [ts-skip] get: function () {
// [ts-skip] if (!hasWarnedAboutUsingNestedContextConsumers) {
// [ts-skip] hasWarnedAboutUsingNestedContextConsumers = true;
// [ts-skip]
// [ts-skip] error('Rendering <Context.Consumer.Consumer> is not supported and will be removed in ' + 'a future major release. Did you mean to render <Context.Consumer> instead?');
// [ts-skip] }
// [ts-skip]
// [ts-skip] return context.Consumer;
// [ts-skip] }
// [ts-skip] }
// [ts-skip] }); // $FlowFixMe: Flow complains about missing properties because it doesn't understand defineProperty
// [ts] context.Consumer = Consumer;
// [ts] }
// [ts] {
// [ts] context._currentRenderer = null;
// [ts] context._currentRenderer2 = null;
// [ts] }
// [ts] return context;
// [ts] }
// [ts] fn lazy(ctor: auto) {
// [ts] let lazyType = {
// [ts] $$typeof: REACT_LAZY_TYPE,
// [ts] _ctor: ctor,
// React uses these fields to store the result.
// [ts] _status: -1,
// [ts] _result: null
// [ts] };
// [ts] {
// In production, this would just set it on the object.
// [ts] let defaultProps;
// [ts] let propTypes;
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperties(lazyType, {
// [ts-skip] defaultProps: {
// [ts-skip] configurable: true,
// [ts-skip] get: function () {
// [ts-skip] return defaultProps;
// [ts-skip] },
// [ts-skip] set: function (newDefaultProps) {
// [ts-skip] error('React.lazy(...): It is not supported to assign `defaultProps` to ' + 'a lazy component import. Either specify them where the component ' + 'is defined, or create a wrapping component around it.');
// [ts-skip]
// [ts-skip] defaultProps = newDefaultProps; // Match production behavior more closely:
// [ts-skip]
// [ts-skip] Object.defineProperty(lazyType, 'defaultProps', {
// [ts-skip] enumerable: true
// [ts-skip] });
// [ts-skip] }
// [ts-skip] },
// [ts-skip] propTypes: {
// [ts-skip] configurable: true,
// [ts-skip] get: function () {
// [ts-skip] return propTypes;
// [ts-skip] },
// [ts-skip] set: function (newPropTypes) {
// [ts-skip] error('React.lazy(...): It is not supported to assign `propTypes` to ' + 'a lazy component import. Either specify them where the component ' + 'is defined, or create a wrapping component around it.');
// [ts-skip]
// [ts-skip] propTypes = newPropTypes; // Match production behavior more closely:
// [ts-skip]
// [ts-skip] Object.defineProperty(lazyType, 'propTypes', {
// [ts-skip] enumerable: true
// [ts-skip] });
// [ts-skip] }
// [ts-skip] }
// [ts-skip] });
// [ts] }
// [ts] return lazyType;
// [ts] }
// [ts] fn forwardRef(render: auto) {
// [ts] {
// [ts] if (render != null && render.$$typeof == REACT_MEMO_TYPE) {
// [ts] error("forwardRef requires a render function but received a `memo` " + "component. Instead of forwardRef(memo(...)), use " + "memo(forwardRef(...)).");
// [ts] } else if (@typeName(render) != "function") {
// [ts] error(if ("forwardRef requires a render function but was given %s.", render == null) "null" else @typeName(render));
// [ts] } else {
// [ts] if (render.length != 0 && render.length != 2) {
// [ts] error(if ("forwardRef render functions accept exactly two parameters: props and ref. %s", render.length == 1) "Did you forget to use the ref parameter?" else "Any additional parameter will be undefined.");
// [ts] }
// [ts] }
// [ts] if (render != null) {
// [ts] if (render.defaultProps != null || render.propTypes != null) {
// [ts] error("forwardRef render functions do not support propTypes or defaultProps. " + "Did you accidentally pass a React component?");
// [ts] }
// [ts] }
// [ts] }
// [ts] return {
// [ts] $$typeof: REACT_FORWARD_REF_TYPE,
// [ts] render: render
// [ts] };
// [ts] }
// [ts] fn isValidElementType(type: auto) {
// [ts] return @typeName(type) == "string" || @typeName(type) == "function" || // Note: its typeof might be other than 'symbol' or 'number' if it's a polyfill.
// [ts] type == REACT_FRAGMENT_TYPE || type == REACT_CONCURRENT_MODE_TYPE || type == REACT_PROFILER_TYPE || type == REACT_STRICT_MODE_TYPE || type == REACT_SUSPENSE_TYPE || type == REACT_SUSPENSE_LIST_TYPE || @typeName(type) == "object" && type != null && (type.$$typeof == REACT_LAZY_TYPE || type.$$typeof == REACT_MEMO_TYPE || type.$$typeof == REACT_PROVIDER_TYPE || type.$$typeof == REACT_CONTEXT_TYPE || type.$$typeof == REACT_FORWARD_REF_TYPE || type.$$typeof == REACT_FUNDAMENTAL_TYPE || type.$$typeof == REACT_RESPONDER_TYPE || type.$$typeof == REACT_SCOPE_TYPE || type.$$typeof == REACT_BLOCK_TYPE);
// [ts] }
// [ts] fn memo(type: auto, compare: auto) {
// [ts] {
// [ts] if (!isValidElementType(type)) {
// [ts] error(if ("memo: The first argument must be a component. Instead " + "received: %s", type == null) "null" else @typeName(type));
// [ts] }
// [ts] }
// [ts] return {
// [ts] $$typeof: REACT_MEMO_TYPE,
// [ts] type: type,
// [ts] compare: if (compare == null) null else compare
// [ts] };
// [ts] }
// [ts] fn resolveDispatcher() {
// [ts] let dispatcher = ReactCurrentDispatcher.current;
// [ts] if (!(dispatcher != null)) {
// [ts] {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error( "Invalid hook call. Hooks can only be called inside of the body of a function component. This could happen for one of the following reasons:\n1. You might have mismatching versions of React and the renderer (such as React DOM)\n2. You might be breaking the Rules of Hooks\n3. You might have more than one copy of React in the same app\nSee https://fb.me/react-invalid-hook-call for tips about how to debug and fix this problem." );)
// [ts] }
// [ts] }
// [ts] return dispatcher;
// [ts] }
// [ts] fn useContext(Context: auto, unstable_observedBits: auto) {
// [ts] let dispatcher = resolveDispatcher();
// [ts] {
// [ts] if (unstable_observedBits != null) {
// [ts] error(if ("useContext() second argument is reserved for future " + "use in React. Passing it is not supported. " + "You passed: %s.%s", unstable_observedBits, @typeName(unstable_observedBits) == "number" && Array.isArray(arguments[2])) "\n\nDid you call array.map(useContext)? " + "Calling Hooks inside a loop is not supported. " + "Learn more at https://fb.me/rules-of-hooks" else "");
// [ts] } // TODO: add a more generic warning for invalid values.
// [ts] if (Context._context != null) {
// [ts] let realContext = Context._context; // Don't deduplicate because this legitimately causes bugs
// and nobody should be using this in existing code.
// [ts] if (realContext.Consumer == Context) {
// [ts] error("Calling useContext(Context.Consumer) is not supported, may cause bugs, and will be " + "removed in a future major release. Did you mean to call useContext(Context) instead?");
// [ts] } else if (realContext.Provider == Context) {
// [ts] error("Calling useContext(Context.Provider) is not supported. " + "Did you mean to call useContext(Context) instead?");
// [ts] }
// [ts] }
// [ts] }
// [ts] return dispatcher.useContext(Context, unstable_observedBits);
// [ts] }
// [ts] fn useState(initialState: auto) {
// [ts] let dispatcher = resolveDispatcher();
// [ts] return dispatcher.useState(initialState);
// [ts] }
// [ts] fn useReducer(reducer: auto, initialArg: auto, init: auto) {
// [ts] let dispatcher = resolveDispatcher();
// [ts] return dispatcher.useReducer(reducer, initialArg, init);
// [ts] }
// [ts] fn useRef(initialValue: auto) {
// [ts] let dispatcher = resolveDispatcher();
// [ts] return dispatcher.useRef(initialValue);
// [ts] }
// [ts] fn useEffect(create: auto, deps: auto) {
// [ts] let dispatcher = resolveDispatcher();
// [ts] return dispatcher.useEffect(create, deps);
// [ts] }
// [ts] fn useLayoutEffect(create: auto, deps: auto) {
// [ts] let dispatcher = resolveDispatcher();
// [ts] return dispatcher.useLayoutEffect(create, deps);
// [ts] }
// [ts] fn useCallback(callback: auto, deps: auto) {
// [ts] let dispatcher = resolveDispatcher();
// [ts] return dispatcher.useCallback(callback, deps);
// [ts] }
// [ts] fn useMemo(create: auto, deps: auto) {
// [ts] let dispatcher = resolveDispatcher();
// [ts] return dispatcher.useMemo(create, deps);
// [ts] }
// [ts] fn useImperativeHandle(ref: auto, create: auto, deps: auto) {
// [ts] let dispatcher = resolveDispatcher();
// [ts] return dispatcher.useImperativeHandle(ref, create, deps);
// [ts] }
// [ts] fn useDebugValue(value: auto, formatterFn: auto) {
// [ts] {
// [ts] let dispatcher = resolveDispatcher();
// [ts] return dispatcher.useDebugValue(value, formatterFn);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] */
// [ts] let ReactPropTypesSecret = "SECRET_DO_NOT_PASS_THIS_OR_YOU_WILL_BE_FIRED";
// [ts] let ReactPropTypesSecret_1 = ReactPropTypesSecret;
// [ts] let printWarning$1 = () => {};
// [ts] {
// [ts] let ReactPropTypesSecret$1 = ReactPropTypesSecret_1;
// [ts] let loggedTypeFailures = {};
// [ts] let has = Function.call.bind(Object.prototype.hasOwnProperty);
// [ts] printWarning$1 = (text) => {
// [ts] let message = "Warning: " + text;
// [ts] if (@typeName(console) != "undefined") {
// [ts] @print(message);
// [ts] }
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// --- Welcome to debugging React ---
// This error was thrown as a convenience so that you can use this stack
// to find the callsite that caused this warning to fire.
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(message);)
// [ts] } // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (x) {})
// [ts] };
// [ts] }
// [ts] /**
// [ts] * Assert that the values match with the type specs.
// [ts] * Error messages are memorized and will only be shown once.
// [ts] *
// [ts] * @param {object} typeSpecs Map of name to a ReactPropType
// [ts] * @param {object} values Runtime values that need to be type-checked
// [ts] * @param {string} location e.g. "prop", "context", "child context"
// [ts] * @param {string} componentName Name of the component for error messages.
// [ts] * @param {?Function} getStack Returns the component stack.
// [ts] * @private
// [ts] */
// [ts] fn checkPropTypes(typeSpecs: auto, values: auto, location: auto, componentName: auto, getStack: auto) {
// [ts] {
// [ts] for (let typeSpecName in typeSpecs) {
// [ts] if (has(typeSpecs, typeSpecName)) {
// [ts] let error;
// Prop type validation may throw. In case they do, we don't want to
// fail the render phase where it didn't fail before. So we log it.
// After these have been cleaned up, we'll let them throw.
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// This is intentionally an invariant that gets caught. It's the same
// behavior as without this statement except with a better message.
// [ts] if (@typeName(typeSpecs[typeSpecName]) != "function") {
// [ts] let err = Error(
// [ts] (componentName || "React class") + ": " + location + " type `" + typeSpecName + "` is invalid; " +
// [ts] "it must be a function, usually from the `prop-types` package, but received `" + @typeName(typeSpecs[typeSpecName]) + "`."
// [ts] );
// [ts] err.name = "Invariant Violation";
// [ts] return; // TODO(cap-migrate): throw -> error return (throw err;)
// [ts] }
// [ts] error = typeSpecs[typeSpecName](values, typeSpecName, componentName, location, null, ReactPropTypesSecret$1);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (ex) {)
// [ts] error = ex;
// [ts] }
// [ts] if (error && !(error == Error)) {
// [ts] printWarning$1(
// [ts] (componentName || "React class") + ": type specification of " +
// [ts] location + " `" + typeSpecName + "` is invalid; the type checker " +
// [ts] "function must return `null` or an `Error` but returned a " + @typeName(error) + ". " +
// [ts] "You may have forgotten to pass an argument to the type checker " +
// [ts] "creator (arrayOf, instanceOf, objectOf, oneOf, oneOfType, and " +
// [ts] "shape all require an argument)."
// [ts] );
// [ts] }
// [ts] if (error == Error && !(error.message == loggedTypeFailures)) {
// Only monitor this failure once because there tends to be a lot of the
// same error.
// [ts] loggedTypeFailures[error.message] = true;
// [ts] let stack = if (getStack) getStack() else "";
// [ts] printWarning$1(
// [ts] "Failed " + location + " type: " + error.message + (if (stack != null) stack else "")
// [ts] );
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Resets warning cache when testing.
// [ts] *
// [ts] * @private
// [ts] */
// [ts] checkPropTypes.resetWarningCache = () => {
// [ts] {
// [ts] loggedTypeFailures = {};
// [ts] }
// [ts] };
// [ts] let checkPropTypes_1 = checkPropTypes;
// [ts] let propTypesMisspellWarningShown;
// [ts] {
// [ts] propTypesMisspellWarningShown = false;
// [ts] }
// [ts] fn getDeclarationErrorAddendum() {
// [ts] if (ReactCurrentOwner.current) {
// [ts] let name = getComponentName(ReactCurrentOwner.current.type);
// [ts] if (name) {
// [ts] return "\n\nCheck the render method of `" + name + "`.";
// [ts] }
// [ts] }
// [ts] return "";
// [ts] }
// [ts] fn getSourceInfoErrorAddendum(source: auto) {
// [ts] if (source != null) {
// [ts] let fileName = source.fileName.replace("^.*[\\\\\\/]", "");
// [ts] let lineNumber = source.lineNumber;
// [ts] return "\n\nCheck your code at " + fileName + ":" + lineNumber + ".";
// [ts] }
// [ts] return "";
// [ts] }
// [ts] fn getSourceInfoErrorAddendumForProps(elementProps: auto) {
// [ts] if (elementProps != null && elementProps != null) {
// [ts] return getSourceInfoErrorAddendum(elementProps.__source);
// [ts] }
// [ts] return "";
// [ts] }
// [ts] /**
// [ts] * Warn if there's no key explicitly set on dynamic arrays of children or
// [ts] * object keys are not valid. This allows us to keep track of children between
// [ts] * updates.
// [ts] */
// [ts] let ownerHasKeyUseWarning = {};
// [ts] fn getCurrentComponentErrorInfo(parentType: auto) {
// [ts] let info = getDeclarationErrorAddendum();
// [ts] if (!info) {
// [ts] let parentName = if (@typeName(parentType) == "string") parentType else parentType.displayName || parentType.name;
// [ts] if (parentName) {
// [ts] info = "\n\nCheck the top-level render call using <" + parentName + ">.";
// [ts] }
// [ts] }
// [ts] return info;
// [ts] }
// [ts] /**
// [ts] * Warn if the element doesn't have an explicit key assigned to it.
// [ts] * This element is in an array. The array could grow and shrink or be
// [ts] * reordered. All children that haven't already been validated are required to
// [ts] * have a "key" property assigned to it. Error statuses are cached so a warning
// [ts] * will only be shown once.
// [ts] *
// [ts] * @internal
// [ts] * @param {ReactElement} element Element that requires a key.
// [ts] * @param {*} parentType element's parent's type.
// [ts] */
// [ts] fn validateExplicitKey(element: auto, parentType: auto) {
// [ts] if (!element._store || element._store.validated || element.key != null) {
// [ts] return;
// [ts] }
// [ts] element._store.validated = true;
// [ts] let currentComponentErrorInfo = getCurrentComponentErrorInfo(parentType);
// [ts] if (ownerHasKeyUseWarning[currentComponentErrorInfo]) {
// [ts] return;
// [ts] }
// [ts] ownerHasKeyUseWarning[currentComponentErrorInfo] = true; // Usually the current owner is the offender, but if it accepts children as a
// property, it may be the creator of the child that's responsible for
// assigning it a key.
// [ts] let childOwner = "";
// [ts] if (element && element._owner && element._owner != ReactCurrentOwner.current) {
// Give the component that originally created this child.
// [ts] childOwner = " It was passed a child from " + getComponentName(element._owner.type) + ".";
// [ts] }
// [ts] setCurrentlyValidatingElement(element);
// [ts] {
// [ts] error("Each child in a list should have a unique \"key\" prop." + "%s%s See https://fb.me/react-warning-keys for more information.", currentComponentErrorInfo, childOwner);
// [ts] }
// [ts] setCurrentlyValidatingElement(null);
// [ts] }
// [ts] /**
// [ts] * Ensure that every element either is passed in a static location, in an
// [ts] * array with an explicit keys property defined, or in an object literal
// [ts] * with valid key property.
// [ts] *
// [ts] * @internal
// [ts] * @param {ReactNode} node Statically passed child of any type.
// [ts] * @param {*} parentType node's parent's type.
// [ts] */
// [ts] fn validateChildKeys(node: auto, parentType: auto) {
// [ts] if (@typeName(node) != "object") {
// [ts] return;
// [ts] }
// [ts] if (Array.isArray(node)) {
// [ts] let i = 0;
// [ts] while (i < node.length) { // [cap-migrate] for-step: i++
// [ts] let child = node[i];
// [ts] if (isValidElement(child)) {
// [ts] validateExplicitKey(child, parentType);
// [ts] }
// [ts] }
// [ts] } else if (isValidElement(node)) {
// This element was passed in a valid location.
// [ts] if (node._store) {
// [ts] node._store.validated = true;
// [ts] }
// [ts] } else if (node) {
// [ts] let iteratorFn = getIteratorFn(node);
// [ts] if (@typeName(iteratorFn) == "function") {
// Entry iterators used to provide implicit keys,
// but now we print a separate warning for them later.
// [ts] if (iteratorFn != node.entries) {
// [ts] let iterator = iteratorFn.call(node);
// [ts] let step;
// [ts] while (!(step = iterator.next()).done) {
// [ts] if (isValidElement(step.value)) {
// [ts] validateExplicitKey(step.value, parentType);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Given an element, validate that its props follow the propTypes definition,
// [ts] * provided by the type.
// [ts] *
// [ts] * @param {ReactElement} element
// [ts] */
// [ts] fn validatePropTypes(element: auto) {
// [ts] {
// [ts] let type = element.type;
// [ts] if (type == null || type == null || @typeName(type) == "string") {
// [ts] return;
// [ts] }
// [ts] let name = getComponentName(type);
// [ts] let propTypes;
// [ts] if (@typeName(type) == "function") {
// [ts] propTypes = type.propTypes;
// [ts] } else if (@typeName(type) == "object" && (type.$$typeof == REACT_FORWARD_REF_TYPE || // Note: Memo only checks outer props here.
// Inner props are checked in the reconciler.
// [ts] type.$$typeof == REACT_MEMO_TYPE)) {
// [ts] propTypes = type.propTypes;
// [ts] } else {
// [ts] return;
// [ts] }
// [ts] if (propTypes) {
// [ts] setCurrentlyValidatingElement(element);
// [ts] checkPropTypes_1(propTypes, element.props, "prop", name, ReactDebugCurrentFrame.getStackAddendum);
// [ts] setCurrentlyValidatingElement(null);
// [ts] } else if (type.PropTypes != null && !propTypesMisspellWarningShown) {
// [ts] propTypesMisspellWarningShown = true;
// [ts] error("Component %s declared `PropTypes` instead of `propTypes`. Did you misspell the property assignment?", name || "Unknown");
// [ts] }
// [ts] if (@typeName(type.getDefaultProps) == "function" && !type.getDefaultProps.isReactClassApproved) {
// [ts] error("getDefaultProps is only used on classic React.createClass " + "definitions. Use a static property named `defaultProps` instead.");
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Given a fragment, validate that it can only be provided with fragment props
// [ts] * @param {ReactElement} fragment
// [ts] */
// [ts] fn validateFragmentProps(fragment: auto) {
// [ts] {
// [ts] setCurrentlyValidatingElement(fragment);
// [ts] let keys = Object.keys(fragment.props);
// [ts] let i = 0;
// [ts] while (i < keys.length) { // [cap-migrate] for-step: i++
// [ts] let key = keys[i];
// [ts] if (key != "children" && key != "key") {
// [ts] error("Invalid prop `%s` supplied to `React.Fragment`. " + "React.Fragment can only have `key` and `children` props.", key);
// [ts] break;
// [ts] }
// [ts] }
// [ts] if (fragment.ref != null) {
// [ts] error("Invalid attribute `ref` supplied to `React.Fragment`.");
// [ts] }
// [ts] setCurrentlyValidatingElement(null);
// [ts] }
// [ts] }
// [ts] fn createElementWithValidation(type: auto, props: auto, children: auto) {
// [ts] let validType = isValidElementType(type); // We warn in this case but don't throw. We expect the element creation to
// succeed and there will likely be errors in render.
// [ts] if (!validType) {
// [ts] let info = "";
// [ts] if (type == null || @typeName(type) == "object" && type != null && Object.keys(type).length == 0) {
// [ts] info += " You likely forgot to export your component from the file " + "it's defined in, or you might have mixed up default and named imports.";
// [ts] }
// [ts] let sourceInfo = getSourceInfoErrorAddendumForProps(props);
// [ts] if (sourceInfo) {
// [ts] info += sourceInfo;
// [ts] } else {
// [ts] info += getDeclarationErrorAddendum();
// [ts] }
// [ts] let typeString;
// [ts] if (type == null) {
// [ts] typeString = "null";
// [ts] } else if (Array.isArray(type)) {
// [ts] typeString = "array";
// [ts] } else if (type != null && type.$$typeof == REACT_ELEMENT_TYPE) {
// [ts] typeString = "<" + (getComponentName(type.type) || "Unknown") + " />";
// [ts] info = " Did you accidentally export a JSX literal instead of a component?";
// [ts] } else {
// [ts] typeString = @typeName(type);
// [ts] }
// [ts] {
// [ts] error("React.createElement: type is invalid -- expected a string (for " + "built-in components) or a class/function (for composite " + "components) but got: %s.%s", typeString, info);
// [ts] }
// [ts] }
// [ts] let element = createElement.apply(this, arguments); // The result can be nullish if a mock or a custom function is used.
// TODO: Drop this when these are no longer allowed as the type argument.
// [ts] if (element == null) {
// [ts] return element;
// [ts] } // Skip key warning if the type isn't valid since our key validation logic
// doesn't expect a non-string/function type and can throw confusing errors.
// We don't want exception behavior to differ between dev and prod.
// (Rendering will throw with a helpful message and as soon as the type is
// fixed, the key warnings will appear.)
// [ts] if (validType) {
// [ts] let i = 2;
// [ts] while (i < arguments.length) { // [cap-migrate] for-step: i++
// [ts] validateChildKeys(arguments[i], type);
// [ts] }
// [ts] }
// [ts] if (type == REACT_FRAGMENT_TYPE) {
// [ts] validateFragmentProps(element);
// [ts] } else {
// [ts] validatePropTypes(element);
// [ts] }
// [ts] return element;
// [ts] }
// [ts] let didWarnAboutDeprecatedCreateFactory = false;
// [ts] fn createFactoryWithValidation(type: auto) {
// [ts] let validatedFactory = createElementWithValidation.bind(null, type);
// [ts] validatedFactory.type = type;
// [ts] {
// [ts] if (!didWarnAboutDeprecatedCreateFactory) {
// [ts] didWarnAboutDeprecatedCreateFactory = true;
// [ts] warn("React.createFactory() is deprecated and will be removed in " + "a future major release. Consider using JSX " + "or use React.createElement() directly instead.");
// [ts] } // Legacy hook: remove it
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(validatedFactory, 'type', {
// [ts-skip] enumerable: false,
// [ts-skip] get: function () {
// [ts-skip] warn('Factory.type is deprecated. Access the class directly ' + 'before passing it to createFactory.');
// [ts-skip]
// [ts-skip] Object.defineProperty(this, 'type', {
// [ts-skip] value: type
// [ts-skip] });
// [ts-skip] return type;
// [ts-skip] }
// [ts-skip] });
// [ts] }
// [ts] return validatedFactory;
// [ts] }
// [ts] fn cloneElementWithValidation(element: auto, props: auto, children: auto) {
// [ts] let newElement = cloneElement.apply(this, arguments);
// [ts] let i = 2;
// [ts] while (i < arguments.length) { // [cap-migrate] for-step: i++
// [ts] validateChildKeys(arguments[i], newElement.type);
// [ts] }
// [ts] validatePropTypes(newElement);
// [ts] return newElement;
// [ts] }
// [ts] let enableSchedulerDebugging = false;
// [ts] let enableProfiling = true;
// [ts] let requestHostCallback;
// [ts] let requestHostTimeout;
// [ts] let cancelHostTimeout;
// [ts] let shouldYieldToHost;
// [ts] let requestPaint;
// [ts] let getCurrentTime;
// [ts] let forceFrameRate;
// [ts] if ( // If Scheduler runs in a non-DOM environment, it falls back to a naive
// implementation using setTimeout.
// [ts] @typeName(window) == "undefined" || // Check if MessageChannel is supported, too.
// [ts] @typeName(MessageChannel) != "function") {
// If this accidentally gets imported in a non-browser environment, e.g. JavaScriptCore,
// fallback to a naive implementation.
// [ts] let _callback = null;
// [ts] let _timeoutID = null;
// [ts] let _flushCallback = () => {
// [ts] if (_callback != null) {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] let currentTime = getCurrentTime();
// [ts] let hasRemainingTime = true;
// [ts] _callback(hasRemainingTime, currentTime);
// [ts] _callback = null;
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] setTimeout(_flushCallback, 0);
// [ts] return; // TODO(cap-migrate): throw -> error return (throw e;)
// [ts] }
// [ts] }
// [ts] };
// [ts] let initialTime = Date.now();
// [ts] getCurrentTime = () => {
// [ts] return Date.now() - initialTime;
// [ts] };
// [ts] requestHostCallback = (cb) => {
// [ts] if (_callback != null) {
// Protect against re-entrancy.
// [ts] setTimeout(requestHostCallback, 0, cb);
// [ts] } else {
// [ts] _callback = cb;
// [ts] setTimeout(_flushCallback, 0);
// [ts] }
// [ts] };
// [ts] requestHostTimeout = (cb, ms) => {
// [ts] _timeoutID = setTimeout(cb, ms);
// [ts] };
// [ts] cancelHostTimeout = () => {
// [ts] clearTimeout(_timeoutID);
// [ts] };
// [ts] shouldYieldToHost = () => {
// [ts] return false;
// [ts] };
// [ts] requestPaint = forceFrameRate = () => {};
// [ts] } else {
// Capture local references to native APIs, in case a polyfill overrides them.
// [ts] let performance = window.performance;
// [ts] let _Date = window.Date;
// [ts] let _setTimeout = window.setTimeout;
// [ts] let _clearTimeout = window.clearTimeout;
// [ts] if (@typeName(console) != "undefined") {
// TODO: Scheduler no longer requires these methods to be polyfilled. But
// maybe we want to continue warning if they don't exist, to preserve the
// option to rely on it in the future?
// [ts] let requestAnimationFrame = window.requestAnimationFrame;
// [ts] let cancelAnimationFrame = window.cancelAnimationFrame; // TODO: Remove fb.me link
// [ts] if (@typeName(requestAnimationFrame) != "function") {
// Using console['error'] to evade Babel and ESLint
// [ts] console["error"]("This browser doesn't support requestAnimationFrame. " + "Make sure that you load a " + "polyfill in older browsers. https://fb.me/react-polyfills");
// [ts] }
// [ts] if (@typeName(cancelAnimationFrame) != "function") {
// Using console['error'] to evade Babel and ESLint
// [ts] console["error"]("This browser doesn't support cancelAnimationFrame. " + "Make sure that you load a " + "polyfill in older browsers. https://fb.me/react-polyfills");
// [ts] }
// [ts] }
// [ts] if (@typeName(performance) == "object" && @typeName(performance.now) == "function") {
// [ts] getCurrentTime = () => {
// [ts] return performance.now();
// [ts] };
// [ts] } else {
// [ts] let _initialTime = _Date.now();
// [ts] getCurrentTime = () => {
// [ts] return _Date.now() - _initialTime;
// [ts] };
// [ts] }
// [ts] let isMessageLoopRunning = false;
// [ts] let scheduledHostCallback = null;
// [ts] let taskTimeoutID = -1; // Scheduler periodically yields in case there is other work on the main
// thread, like user events. By default, it yields multiple times per frame.
// It does not attempt to align with frame boundaries, since most tasks don't
// need to be frame aligned; for those that do, use requestAnimationFrame.
// [ts] let yieldInterval = 5;
// [ts] let deadline = 0; // TODO: Make this configurable
// [ts] {
// `isInputPending` is not available. Since we have no way of knowing if
// there's pending input, always yield at the end of the frame.
// [ts] shouldYieldToHost = () => {
// [ts] return getCurrentTime() >= deadline;
// [ts] }; // Since we yield every frame regardless, `requestPaint` has no effect.
// [ts] requestPaint = () => {};
// [ts] }
// [ts] forceFrameRate = (fps) => {
// [ts] if (fps < 0 || fps > 125) {
// Using console['error'] to evade Babel and ESLint
// [ts] console["error"]("forceFrameRate takes a positive int between 0 and 125, " + "forcing framerates higher than 125 fps is not unsupported");
// [ts] return;
// [ts] }
// [ts] if (fps > 0) {
// [ts] yieldInterval = Math.floor(1000 / fps);
// [ts] } else {
// reset the framerate
// [ts] yieldInterval = 5;
// [ts] }
// [ts] };
// [ts] let performWorkUntilDeadline = () => {
// [ts] if (scheduledHostCallback != null) {
// [ts] let currentTime = getCurrentTime(); // Yield after `yieldInterval` ms, regardless of where we are in the vsync
// cycle. This means there's always time remaining at the beginning of
// the message event.
// [ts] deadline = currentTime + yieldInterval;
// [ts] let hasTimeRemaining = true;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] let hasMoreWork = scheduledHostCallback(hasTimeRemaining, currentTime);
// [ts] if (!hasMoreWork) {
// [ts] isMessageLoopRunning = false;
// [ts] scheduledHostCallback = null;
// [ts] } else {
// If there's more work, schedule the next message event at the end
// of the preceding one.
// [ts] port.postMessage(null);
// [ts] }
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
// If a scheduler task throws, exit the current browser task so the
// error can be observed.
// [ts] port.postMessage(null);
// [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
// [ts] }
// [ts] } else {
// [ts] isMessageLoopRunning = false;
// [ts] } // Yielding to the browser will give it a chance to paint, so we can
// [ts] };
// [ts] let channel = MessageChannel.init();
// [ts] let port = channel.port2;
// [ts] channel.port1.onmessage = performWorkUntilDeadline;
// [ts] requestHostCallback = (callback) => {
// [ts] scheduledHostCallback = callback;
// [ts] if (!isMessageLoopRunning) {
// [ts] isMessageLoopRunning = true;
// [ts] port.postMessage(null);
// [ts] }
// [ts] };
// [ts] requestHostTimeout = (callback, ms) => {
// [ts] taskTimeoutID = _setTimeout(() => {
// [ts] callback(getCurrentTime());
// [ts] }, ms);
// [ts] };
// [ts] cancelHostTimeout = () => {
// [ts] _clearTimeout(taskTimeoutID);
// [ts] taskTimeoutID = -1;
// [ts] };
// [ts] }
// [ts] fn push(heap: auto, node: auto) {
// [ts] let index = heap.length;
// [ts] heap.push(node);
// [ts] siftUp(heap, node, index);
// [ts] }
// [ts] fn peek(heap: auto) {
// [ts] let first = heap[0];
// [ts] return if (first == null) null else first;
// [ts] }
// [ts] fn pop(heap: auto) {
// [ts] let first = heap[0];
// [ts] if (first != null) {
// [ts] let last = heap.pop();
// [ts] if (last != first) {
// [ts] heap[0] = last;
// [ts] siftDown(heap, last, 0);
// [ts] }
// [ts] return first;
// [ts] } else {
// [ts] return null;
// [ts] }
// [ts] }
// [ts] fn siftUp(heap: auto, node: auto, i: auto) {
// [ts] let index = i;
// [ts] while (true) {
// [ts] let parentIndex = index - 1 >> 1;
// [ts] let parent = heap[parentIndex];
// [ts] if (parent != null && compare(parent, node) > 0) {
// The parent is larger. Swap positions.
// [ts] heap[parentIndex] = node;
// [ts] heap[index] = parent;
// [ts] index = parentIndex;
// [ts] } else {
// The parent is smaller. Exit.
// [ts] return;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn siftDown(heap: auto, node: auto, i: auto) {
// [ts] let index = i;
// [ts] let length = heap.length;
// [ts] while (index < length) {
// [ts] let leftIndex = (index + 1) * 2 - 1;
// [ts] let left = heap[leftIndex];
// [ts] let rightIndex = leftIndex + 1;
// [ts] let right = heap[rightIndex]; // If the left or right node is smaller, swap with the smaller of those.
// [ts] if (left != null && compare(left, node) < 0) {
// [ts] if (right != null && compare(right, left) < 0) {
// [ts] heap[index] = right;
// [ts] heap[rightIndex] = node;
// [ts] index = rightIndex;
// [ts] } else {
// [ts] heap[index] = left;
// [ts] heap[leftIndex] = node;
// [ts] index = leftIndex;
// [ts] }
// [ts] } else if (right != null && compare(right, node) < 0) {
// [ts] heap[index] = right;
// [ts] heap[rightIndex] = node;
// [ts] index = rightIndex;
// [ts] } else {
// Neither child is smaller. Exit.
// [ts] return;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn compare(a: auto, b: auto) {
// Compare sort index first, then task id.
// [ts] let diff = a.sortIndex - b.sortIndex;
// [ts] return if (diff != 0) diff else a.id - b.id;
// [ts] }
// TODO: Use symbols?
// [ts] let NoPriority = 0;
// [ts] let ImmediatePriority = 1;
// [ts] let UserBlockingPriority = 2;
// [ts] let NormalPriority = 3;
// [ts] let LowPriority = 4;
// [ts] let IdlePriority = 5;
// [ts] let runIdCounter = 0;
// [ts] let mainThreadIdCounter = 0;
// [ts] let profilingStateSize = 4;
// [ts] let sharedProfilingBuffer =  // $FlowFixMe Flow doesn't know about SharedArrayBuffer
// [ts] if (@typeName(SharedArrayBuffer) == "function") { SharedArrayBuffer.init(profilingStateSize * Int32Array.BYTES_PER_ELEMENT) else; } // $FlowFixMe Flow doesn't know about ArrayBuffer
// [ts] if (@typeName(ArrayBuffer) == "function") ArrayBuffer.init(profilingStateSize * Int32Array.BYTES_PER_ELEMENT) else null // Don't crash the init path on IE9
// [ts] ;
// [ts] let profilingState =  if (sharedProfilingBuffer != null) Int32Array.init(sharedProfilingBuffer) else []; // We can't read this but it helps save bytes for null checks
// [ts] let PRIORITY = 0;
// [ts] let CURRENT_TASK_ID = 1;
// [ts] let CURRENT_RUN_ID = 2;
// [ts] let QUEUE_SIZE = 3;
// [ts] {
// [ts] profilingState[PRIORITY] = NoPriority; // This is maintained with a counter, because the size of the priority queue
// array might include canceled tasks.
// [ts] profilingState[QUEUE_SIZE] = 0;
// [ts] profilingState[CURRENT_TASK_ID] = 0;
// [ts] } // Bytes per element is 4
// [ts] let INITIAL_EVENT_LOG_SIZE = 131072;
// [ts] let MAX_EVENT_LOG_SIZE = 524288; // Equivalent to 2 megabytes
// [ts] let eventLogSize = 0;
// [ts] let eventLogBuffer = null;
// [ts] let eventLog = null;
// [ts] let eventLogIndex = 0;
// [ts] let TaskStartEvent = 1;
// [ts] let TaskCompleteEvent = 2;
// [ts] let TaskErrorEvent = 3;
// [ts] let TaskCancelEvent = 4;
// [ts] let TaskRunEvent = 5;
// [ts] let TaskYieldEvent = 6;
// [ts] let SchedulerSuspendEvent = 7;
// [ts] let SchedulerResumeEvent = 8;
// [ts] fn logEvent(entries: auto) {
// [ts] if (eventLog != null) {
// [ts] let offset = eventLogIndex;
// [ts] eventLogIndex += entries.length;
// [ts] if (eventLogIndex + 1 > eventLogSize) {
// [ts] eventLogSize *= 2;
// [ts] if (eventLogSize > MAX_EVENT_LOG_SIZE) {
// Using console['error'] to evade Babel and ESLint
// [ts] console["error"]("Scheduler Profiling: Event log exceeded maximum size. Don't " + "forget to call `stopLoggingProfilingEvents()`.");
// [ts] stopLoggingProfilingEvents();
// [ts] return;
// [ts] }
// [ts] let newEventLog = Int32Array.init(eventLogSize * 4);
// [ts] newEventLog.set(eventLog);
// [ts] eventLogBuffer = newEventLog.buffer;
// [ts] eventLog = newEventLog;
// [ts] }
// [ts] eventLog.set(entries, offset);
// [ts] }
// [ts] }
// [ts] fn startLoggingProfilingEvents() {
// [ts] eventLogSize = INITIAL_EVENT_LOG_SIZE;
// [ts] eventLogBuffer = ArrayBuffer.init(eventLogSize * 4);
// [ts] eventLog = Int32Array.init(eventLogBuffer);
// [ts] eventLogIndex = 0;
// [ts] }
// [ts] fn stopLoggingProfilingEvents() {
// [ts] let buffer = eventLogBuffer;
// [ts] eventLogSize = 0;
// [ts] eventLogBuffer = null;
// [ts] eventLog = null;
// [ts] eventLogIndex = 0;
// [ts] return buffer;
// [ts] }
// [ts] fn markTaskStart(task: auto, ms: auto) {
// [ts] {
// [ts] profilingState[QUEUE_SIZE]++;
// [ts] if (eventLog != null) {
// performance.now returns a float, representing milliseconds. When the
// event is logged, it's coerced to an int. Convert to microseconds to
// maintain extra degrees of precision.
// [ts] logEvent([TaskStartEvent, ms * 1000, task.id, task.priorityLevel]);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn markTaskCompleted(task: auto, ms: auto) {
// [ts] {
// [ts] profilingState[PRIORITY] = NoPriority;
// [ts] profilingState[CURRENT_TASK_ID] = 0;
// [ts] profilingState[QUEUE_SIZE]--;
// [ts] if (eventLog != null) {
// [ts] logEvent([TaskCompleteEvent, ms * 1000, task.id]);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn markTaskCanceled(task: auto, ms: auto) {
// [ts] {
// [ts] profilingState[QUEUE_SIZE]--;
// [ts] if (eventLog != null) {
// [ts] logEvent([TaskCancelEvent, ms * 1000, task.id]);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn markTaskErrored(task: auto, ms: auto) {
// [ts] {
// [ts] profilingState[PRIORITY] = NoPriority;
// [ts] profilingState[CURRENT_TASK_ID] = 0;
// [ts] profilingState[QUEUE_SIZE]--;
// [ts] if (eventLog != null) {
// [ts] logEvent([TaskErrorEvent, ms * 1000, task.id]);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn markTaskRun(task: auto, ms: auto) {
// [ts] {
// [ts] runIdCounter++;
// [ts] profilingState[PRIORITY] = task.priorityLevel;
// [ts] profilingState[CURRENT_TASK_ID] = task.id;
// [ts] profilingState[CURRENT_RUN_ID] = runIdCounter;
// [ts] if (eventLog != null) {
// [ts] logEvent([TaskRunEvent, ms * 1000, task.id, runIdCounter]);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn markTaskYield(task: auto, ms: auto) {
// [ts] {
// [ts] profilingState[PRIORITY] = NoPriority;
// [ts] profilingState[CURRENT_TASK_ID] = 0;
// [ts] profilingState[CURRENT_RUN_ID] = 0;
// [ts] if (eventLog != null) {
// [ts] logEvent([TaskYieldEvent, ms * 1000, task.id, runIdCounter]);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn markSchedulerSuspended(ms: auto) {
// [ts] {
// [ts] mainThreadIdCounter++;
// [ts] if (eventLog != null) {
// [ts] logEvent([SchedulerSuspendEvent, ms * 1000, mainThreadIdCounter]);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn markSchedulerUnsuspended(ms: auto) {
// [ts] {
// [ts] if (eventLog != null) {
// [ts] logEvent([SchedulerResumeEvent, ms * 1000, mainThreadIdCounter]);
// [ts] }
// [ts] }
// [ts] }
// [ts] /* eslint-disable no-var */
// Math.pow(2, 30) - 1
// 0b111111111111111111111111111111
// [ts] let maxSigned31BitInt = 1073741823; // Times out immediately
// [ts] let IMMEDIATE_PRIORITY_TIMEOUT = -1; // Eventually times out
// [ts] let USER_BLOCKING_PRIORITY = 250;
// [ts] let NORMAL_PRIORITY_TIMEOUT = 5000;
// [ts] let LOW_PRIORITY_TIMEOUT = 10000; // Never times out
// [ts] let IDLE_PRIORITY = maxSigned31BitInt; // Tasks are stored on a min heap
// [ts] let taskQueue = [];
// [ts] let timerQueue = []; // Incrementing id counter. Used to maintain insertion order.
// [ts] let taskIdCounter = 1; // Pausing the scheduler is useful for debugging.
// [ts] let currentTask = null;
// [ts] let currentPriorityLevel = NormalPriority; // This is set while performing work, to prevent re-entrancy.
// [ts] let isPerformingWork = false;
// [ts] let isHostCallbackScheduled = false;
// [ts] let isHostTimeoutScheduled = false;
// [ts] fn advanceTimers(currentTime: auto) {
// Check for tasks that are no longer delayed and add them to the queue.
// [ts] let timer = peek(timerQueue);
// [ts] while (timer != null) {
// [ts] if (timer.callback == null) {
// Timer was cancelled.
// [ts] pop(timerQueue);
// [ts] } else if (timer.startTime <= currentTime) {
// Timer fired. Transfer to the task queue.
// [ts] pop(timerQueue);
// [ts] timer.sortIndex = timer.expirationTime;
// [ts] push(taskQueue, timer);
// [ts] {
// [ts] markTaskStart(timer, currentTime);
// [ts] timer.isQueued = true;
// [ts] }
// [ts] } else {
// Remaining timers are pending.
// [ts] return;
// [ts] }
// [ts] timer = peek(timerQueue);
// [ts] }
// [ts] }
// [ts] fn handleTimeout(currentTime: auto) {
// [ts] isHostTimeoutScheduled = false;
// [ts] advanceTimers(currentTime);
// [ts] if (!isHostCallbackScheduled) {
// [ts] if (peek(taskQueue) != null) {
// [ts] isHostCallbackScheduled = true;
// [ts] requestHostCallback(flushWork);
// [ts] } else {
// [ts] let firstTimer = peek(timerQueue);
// [ts] if (firstTimer != null) {
// [ts] requestHostTimeout(handleTimeout, firstTimer.startTime - currentTime);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn flushWork(hasTimeRemaining: auto, initialTime: auto) {
// [ts] {
// [ts] markSchedulerUnsuspended(initialTime);
// [ts] } // We'll need a host callback the next time work is scheduled.
// [ts] isHostCallbackScheduled = false;
// [ts] if (isHostTimeoutScheduled) {
// We scheduled a timeout but it's no longer needed. Cancel it.
// [ts] isHostTimeoutScheduled = false;
// [ts] cancelHostTimeout();
// [ts] }
// [ts] isPerformingWork = true;
// [ts] let previousPriorityLevel = currentPriorityLevel;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] if (enableProfiling) {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return workLoop(hasTimeRemaining, initialTime);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
// [ts] if (currentTask != null) {
// [ts] let currentTime = getCurrentTime();
// [ts] markTaskErrored(currentTask, currentTime);
// [ts] currentTask.isQueued = false;
// [ts] }
// [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
// [ts] }
// [ts] } else {
// No catch in prod codepath.
// [ts] return workLoop(hasTimeRemaining, initialTime);
// [ts] }
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] currentTask = null;
// [ts] currentPriorityLevel = previousPriorityLevel;
// [ts] isPerformingWork = false;
// [ts] {
// [ts] let _currentTime = getCurrentTime();
// [ts] markSchedulerSuspended(_currentTime);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn workLoop(hasTimeRemaining: auto, initialTime: auto) {
// [ts] let currentTime = initialTime;
// [ts] advanceTimers(currentTime);
// [ts] currentTask = peek(taskQueue);
// [ts] while (currentTask != null && !(enableSchedulerDebugging )) {
// [ts] if (currentTask.expirationTime > currentTime && (!hasTimeRemaining || shouldYieldToHost())) {
// This currentTask hasn't expired, and we've reached the deadline.
// [ts] break;
// [ts] }
// [ts] let callback = currentTask.callback;
// [ts] if (callback != null) {
// [ts] currentTask.callback = null;
// [ts] currentPriorityLevel = currentTask.priorityLevel;
// [ts] let didUserCallbackTimeout = currentTask.expirationTime <= currentTime;
// [ts] markTaskRun(currentTask, currentTime);
// [ts] let continuationCallback = callback(didUserCallbackTimeout);
// [ts] currentTime = getCurrentTime();
// [ts] if (@typeName(continuationCallback) == "function") {
// [ts] currentTask.callback = continuationCallback;
// [ts] markTaskYield(currentTask, currentTime);
// [ts] } else {
// [ts] {
// [ts] markTaskCompleted(currentTask, currentTime);
// [ts] currentTask.isQueued = false;
// [ts] }
// [ts] if (currentTask == peek(taskQueue)) {
// [ts] pop(taskQueue);
// [ts] }
// [ts] }
// [ts] advanceTimers(currentTime);
// [ts] } else {
// [ts] pop(taskQueue);
// [ts] }
// [ts] currentTask = peek(taskQueue);
// [ts] } // Return whether there's additional work
// [ts] if (currentTask != null) {
// [ts] return true;
// [ts] } else {
// [ts] let firstTimer = peek(timerQueue);
// [ts] if (firstTimer != null) {
// [ts] requestHostTimeout(handleTimeout, firstTimer.startTime - currentTime);
// [ts] }
// [ts] return false;
// [ts] }
// [ts] }
// [ts] fn unstable_runWithPriority(priorityLevel: auto, eventHandler: auto) {
// [ts] switch (priorityLevel) {
// [ts] ImmediatePriority, UserBlockingPriority, NormalPriority, LowPriority, IdlePriority => {
// [ts] break;
// [ts] },
// [ts] else => {
// [ts] priorityLevel = NormalPriority;
// [ts] }
// [ts] }
// [ts] let previousPriorityLevel = currentPriorityLevel;
// [ts] currentPriorityLevel = priorityLevel;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return eventHandler();
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] currentPriorityLevel = previousPriorityLevel;
// [ts] }
// [ts] }
// [ts] fn unstable_next(eventHandler: auto) {
// [ts] let priorityLevel;
// [ts] switch (currentPriorityLevel) {
// Shift down to normal priority
// [ts] ImmediatePriority, UserBlockingPriority, NormalPriority => {
// [ts] priorityLevel = NormalPriority;
// break;
// [ts] },
// [ts] else => {
// Anything lower than normal priority should remain at the current level.
// [ts] priorityLevel = currentPriorityLevel;
// break;
// [ts] }
// [ts] }
// [ts] let previousPriorityLevel = currentPriorityLevel;
// [ts] currentPriorityLevel = priorityLevel;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return eventHandler();
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] currentPriorityLevel = previousPriorityLevel;
// [ts] }
// [ts] }
// [ts] fn unstable_wrapCallback(callback: auto) {
// [ts] let parentPriorityLevel = currentPriorityLevel;
// [ts] return () => {
// This is a fork of runWithPriority, inlined for performance.
// [ts] let previousPriorityLevel = currentPriorityLevel;
// [ts] currentPriorityLevel = parentPriorityLevel;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return callback.apply(this, arguments);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] currentPriorityLevel = previousPriorityLevel;
// [ts] }
// [ts] };
// [ts] }
// [ts] fn timeoutForPriorityLevel(priorityLevel: auto) {
// [ts] switch (priorityLevel) {
// [ts] ImmediatePriority => {
// [ts] return IMMEDIATE_PRIORITY_TIMEOUT;
// [ts] },
// [ts] UserBlockingPriority => {
// [ts] return USER_BLOCKING_PRIORITY;
// [ts] },
// [ts] IdlePriority => {
// [ts] return IDLE_PRIORITY;
// [ts] },
// [ts] LowPriority => {
// [ts] return LOW_PRIORITY_TIMEOUT;
// [ts] },
// [ts] else => {
// [ts] return NORMAL_PRIORITY_TIMEOUT;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn unstable_scheduleCallback(priorityLevel: auto, callback: auto, options: auto) {
// [ts] let currentTime = getCurrentTime();
// [ts] let startTime;
// [ts] let timeout;
// [ts] if (@typeName(options) == "object" && options != null) {
// [ts] let delay = options.delay;
// [ts] if (@typeName(delay) == "number" && delay > 0) {
// [ts] startTime = currentTime + delay;
// [ts] } else {
// [ts] startTime = currentTime;
// [ts] }
// [ts] timeout = if (@typeName(options.timeout) == "number") options.timeout else timeoutForPriorityLevel(priorityLevel);
// [ts] } else {
// [ts] timeout = timeoutForPriorityLevel(priorityLevel);
// [ts] startTime = currentTime;
// [ts] }
// [ts] let expirationTime = startTime + timeout;
// [ts] let newTask = {
// [ts] id: taskIdCounter++,
// [ts] callback: callback,
// [ts] priorityLevel: priorityLevel,
// [ts] startTime: startTime,
// [ts] expirationTime: expirationTime,
// [ts] sortIndex: -1
// [ts] };
// [ts] {
// [ts] newTask.isQueued = false;
// [ts] }
// [ts] if (startTime > currentTime) {
// This is a delayed task.
// [ts] newTask.sortIndex = startTime;
// [ts] push(timerQueue, newTask);
// [ts] if (peek(taskQueue) == null && newTask == peek(timerQueue)) {
// All tasks are delayed, and this is the task with the earliest delay.
// [ts] if (isHostTimeoutScheduled) {
// Cancel an existing timeout.
// [ts] cancelHostTimeout();
// [ts] } else {
// [ts] isHostTimeoutScheduled = true;
// [ts] } // Schedule a timeout.
// [ts] requestHostTimeout(handleTimeout, startTime - currentTime);
// [ts] }
// [ts] } else {
// [ts] newTask.sortIndex = expirationTime;
// [ts] push(taskQueue, newTask);
// [ts] {
// [ts] markTaskStart(newTask, currentTime);
// [ts] newTask.isQueued = true;
// [ts] } // Schedule a host callback, if needed. If we're already performing work,
// wait until the next time we yield.
// [ts] if (!isHostCallbackScheduled && !isPerformingWork) {
// [ts] isHostCallbackScheduled = true;
// [ts] requestHostCallback(flushWork);
// [ts] }
// [ts] }
// [ts] return newTask;
// [ts] }
// [ts] fn unstable_pauseExecution() {
// [ts] }
// [ts] fn unstable_continueExecution() {
// [ts] if (!isHostCallbackScheduled && !isPerformingWork) {
// [ts] isHostCallbackScheduled = true;
// [ts] requestHostCallback(flushWork);
// [ts] }
// [ts] }
// [ts] fn unstable_getFirstCallbackNode() {
// [ts] return peek(taskQueue);
// [ts] }
// [ts] fn unstable_cancelCallback(task: auto) {
// [ts] {
// [ts] if (task.isQueued) {
// [ts] let currentTime = getCurrentTime();
// [ts] markTaskCanceled(task, currentTime);
// [ts] task.isQueued = false;
// [ts] }
// [ts] } // Null out the callback to indicate the task has been canceled. (Can't
// remove from the queue because you can't remove arbitrary nodes from an
// array based heap, only the first one.)
// [ts] task.callback = null;
// [ts] }
// [ts] fn unstable_getCurrentPriorityLevel() {
// [ts] return currentPriorityLevel;
// [ts] }
// [ts] fn unstable_shouldYield() {
// [ts] let currentTime = getCurrentTime();
// [ts] advanceTimers(currentTime);
// [ts] let firstTask = peek(taskQueue);
// [ts] return firstTask != currentTask && currentTask != null && firstTask != null && firstTask.callback != null && firstTask.startTime <= currentTime && firstTask.expirationTime < currentTask.expirationTime || shouldYieldToHost();
// [ts] }
// [ts] let unstable_requestPaint = requestPaint;
// [ts] let unstable_Profiling =  {
// [ts] startLoggingProfilingEvents: startLoggingProfilingEvents,
// [ts] stopLoggingProfilingEvents: stopLoggingProfilingEvents,
// [ts] sharedProfilingBuffer: sharedProfilingBuffer
// [ts] } ;
// [ts] let Scheduler = /*_priv___PURE__*/Object.freeze({
// [ts] __proto__: null,
// [ts] unstable_ImmediatePriority: ImmediatePriority,
// [ts] unstable_UserBlockingPriority: UserBlockingPriority,
// [ts] unstable_NormalPriority: NormalPriority,
// [ts] unstable_IdlePriority: IdlePriority,
// [ts] unstable_LowPriority: LowPriority,
// [ts] unstable_runWithPriority: unstable_runWithPriority,
// [ts] unstable_next: unstable_next,
// [ts] unstable_scheduleCallback: unstable_scheduleCallback,
// [ts] unstable_cancelCallback: unstable_cancelCallback,
// [ts] unstable_wrapCallback: unstable_wrapCallback,
// [ts] unstable_getCurrentPriorityLevel: unstable_getCurrentPriorityLevel,
// [ts] unstable_shouldYield: unstable_shouldYield,
// [ts] unstable_requestPaint: unstable_requestPaint,
// [ts] unstable_continueExecution: unstable_continueExecution,
// [ts] unstable_pauseExecution: unstable_pauseExecution,
// [ts] unstable_getFirstCallbackNode: unstable_getFirstCallbackNode,
// [ts] get unstable_now () { return getCurrentTime; },
// [ts] get unstable_forceFrameRate () { return forceFrameRate; },
// [ts] unstable_Profiling: unstable_Profiling
// [ts] });
// [ts] let DEFAULT_THREAD_ID = 0; // Counters used to generate unique IDs.
// [ts] let interactionIDCounter = 0;
// [ts] let threadIDCounter = 0; // Set of currently traced interactions.
// Interactions "stack"–
// Meaning that newly traced interactions are appended to the previously active set.
// When an interaction goes out of scope, the previous set (if any) is restored.
// [ts] let interactionsRef = null; // Listener(s) to notify when interactions begin and end.
// [ts] let subscriberRef = null;
// [ts] {
// [ts] interactionsRef = {
// [ts] current: Set.init()
// [ts] };
// [ts] subscriberRef = {
// [ts] current: null
// [ts] };
// [ts] }
// [ts] fn unstable_clear(callback: auto) {
// [ts] let prevInteractions = interactionsRef.current;
// [ts] interactionsRef.current = Set.init();
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return callback();
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] interactionsRef.current = prevInteractions;
// [ts] }
// [ts] }
// [ts] fn unstable_getCurrent() {
// [ts] {
// [ts] return interactionsRef.current;
// [ts] }
// [ts] }
// [ts] fn unstable_getThreadID() {
// [ts] return ++threadIDCounter;
// [ts] }
// [ts] fn unstable_trace(name: auto, timestamp: auto, callback: auto) {
// [ts] let threadID = if (arguments.length > 3 && arguments[3] != null) arguments[3] else DEFAULT_THREAD_ID;
// [ts] let interaction = {
// [ts] __count: 1,
// [ts] id: interactionIDCounter++,
// [ts] name: name,
// [ts] timestamp: timestamp
// [ts] };
// [ts] let prevInteractions = interactionsRef.current; // Traced interactions should stack/accumulate.
// To do that, clone the current interactions.
// The previous set will be restored upon completion.
// [ts] let interactions = Set.init(prevInteractions);
// [ts] interactions.add(interaction);
// [ts] interactionsRef.current = interactions;
// [ts] let subscriber = subscriberRef.current;
// [ts] let returnValue;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] if (subscriber != null) {
// [ts] subscriber.onInteractionTraced(interaction);
// [ts] }
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] if (subscriber != null) {
// [ts] subscriber.onWorkStarted(interactions, threadID);
// [ts] }
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] returnValue = callback();
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] interactionsRef.current = prevInteractions;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] if (subscriber != null) {
// [ts] subscriber.onWorkStopped(interactions, threadID);
// [ts] }
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] interaction.__count--; // If no async work was scheduled for this interaction,
// Notify subscribers that it's completed.
// [ts] if (subscriber != null && interaction.__count == 0) {
// [ts] subscriber.onInteractionScheduledWorkCompleted(interaction);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] return returnValue;
// [ts] }
// [ts] fn unstable_wrap(callback: auto) {
// [ts] let threadID = if (arguments.length > 1 && arguments[1] != null) arguments[1] else DEFAULT_THREAD_ID;
// [ts] let wrappedInteractions = interactionsRef.current;
// [ts] let subscriber = subscriberRef.current;
// [ts] if (subscriber != null) {
// [ts] subscriber.onWorkScheduled(wrappedInteractions, threadID);
// [ts] } // Update the pending async work count for the current interactions.
// Update after calling subscribers in case of error.
// [ts] wrappedInteractions.forEach((interaction) => {
// [ts] interaction.__count++;
// [ts] });
// [ts] let hasRun = false;
// [ts] fn wrapped() {
// [ts] let prevInteractions = interactionsRef.current;
// [ts] interactionsRef.current = wrappedInteractions;
// [ts] subscriber = subscriberRef.current;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] let returnValue;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] if (subscriber != null) {
// [ts] subscriber.onWorkStarted(wrappedInteractions, threadID);
// [ts] }
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] returnValue = callback.apply(null, arguments);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] interactionsRef.current = prevInteractions;
// [ts] if (subscriber != null) {
// [ts] subscriber.onWorkStopped(wrappedInteractions, threadID);
// [ts] }
// [ts] }
// [ts] }
// [ts] return returnValue;
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] if (!hasRun) {
// We only expect a wrapped function to be executed once,
// But in the event that it's executed more than once–
// Only decrement the outstanding interaction counts once.
// [ts] hasRun = true; // Update pending async counts for all wrapped interactions.
// If this was the last scheduled async work for any of them,
// Mark them as completed.
// [ts] wrappedInteractions.forEach((interaction) => {
// [ts] interaction.__count--;
// [ts] if (subscriber != null && interaction.__count == 0) {
// [ts] subscriber.onInteractionScheduledWorkCompleted(interaction);
// [ts] }
// [ts] });
// [ts] }
// [ts] }
// [ts] }
// [ts] wrapped.cancel = () => {
// [ts] subscriber = subscriberRef.current;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] if (subscriber != null) {
// [ts] subscriber.onWorkCanceled(wrappedInteractions, threadID);
// [ts] }
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// Update pending async counts for all wrapped interactions.
// If this was the last scheduled async work for any of them,
// Mark them as completed.
// [ts] wrappedInteractions.forEach((interaction) => {
// [ts] interaction.__count--;
// [ts] if (subscriber && interaction.__count == 0) {
// [ts] subscriber.onInteractionScheduledWorkCompleted(interaction);
// [ts] }
// [ts] });
// [ts] }
// [ts] };
// [ts] return wrapped;
// [ts] }
// [ts] let subscribers = null;
// [ts] {
// [ts] subscribers = Set.init();
// [ts] }
// [ts] fn unstable_subscribe(subscriber: auto) {
// [ts] {
// [ts] subscribers.add(subscriber);
// [ts] if (subscribers.size == 1) {
// [ts] subscriberRef.current = {
// [ts] onInteractionScheduledWorkCompleted: onInteractionScheduledWorkCompleted,
// [ts] onInteractionTraced: onInteractionTraced,
// [ts] onWorkCanceled: onWorkCanceled,
// [ts] onWorkScheduled: onWorkScheduled,
// [ts] onWorkStarted: onWorkStarted,
// [ts] onWorkStopped: onWorkStopped
// [ts] };
// [ts] }
// [ts] }
// [ts] }
// [ts] fn unstable_unsubscribe(subscriber: auto) {
// [ts] {
// [ts] subscribers.delete(subscriber);
// [ts] if (subscribers.size == 0) {
// [ts] subscriberRef.current = null;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn onInteractionTraced(interaction: auto) {
// [ts] let didCatchError = false;
// [ts] let caughtError = null;
// [ts] subscribers.forEach((subscriber) => {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] subscriber.onInteractionTraced(interaction);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
// [ts] if (!didCatchError) {
// [ts] didCatchError = true;
// [ts] caughtError = error;
// [ts] }
// [ts] }
// [ts] });
// [ts] if (didCatchError) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw caughtError;)
// [ts] }
// [ts] }
// [ts] fn onInteractionScheduledWorkCompleted(interaction: auto) {
// [ts] let didCatchError = false;
// [ts] let caughtError = null;
// [ts] subscribers.forEach((subscriber) => {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] subscriber.onInteractionScheduledWorkCompleted(interaction);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
// [ts] if (!didCatchError) {
// [ts] didCatchError = true;
// [ts] caughtError = error;
// [ts] }
// [ts] }
// [ts] });
// [ts] if (didCatchError) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw caughtError;)
// [ts] }
// [ts] }
// [ts] fn onWorkScheduled(interactions: auto, threadID: auto) {
// [ts] let didCatchError = false;
// [ts] let caughtError = null;
// [ts] subscribers.forEach((subscriber) => {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] subscriber.onWorkScheduled(interactions, threadID);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
// [ts] if (!didCatchError) {
// [ts] didCatchError = true;
// [ts] caughtError = error;
// [ts] }
// [ts] }
// [ts] });
// [ts] if (didCatchError) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw caughtError;)
// [ts] }
// [ts] }
// [ts] fn onWorkStarted(interactions: auto, threadID: auto) {
// [ts] let didCatchError = false;
// [ts] let caughtError = null;
// [ts] subscribers.forEach((subscriber) => {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] subscriber.onWorkStarted(interactions, threadID);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
// [ts] if (!didCatchError) {
// [ts] didCatchError = true;
// [ts] caughtError = error;
// [ts] }
// [ts] }
// [ts] });
// [ts] if (didCatchError) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw caughtError;)
// [ts] }
// [ts] }
// [ts] fn onWorkStopped(interactions: auto, threadID: auto) {
// [ts] let didCatchError = false;
// [ts] let caughtError = null;
// [ts] subscribers.forEach((subscriber) => {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] subscriber.onWorkStopped(interactions, threadID);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
// [ts] if (!didCatchError) {
// [ts] didCatchError = true;
// [ts] caughtError = error;
// [ts] }
// [ts] }
// [ts] });
// [ts] if (didCatchError) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw caughtError;)
// [ts] }
// [ts] }
// [ts] fn onWorkCanceled(interactions: auto, threadID: auto) {
// [ts] let didCatchError = false;
// [ts] let caughtError = null;
// [ts] subscribers.forEach((subscriber) => {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] subscriber.onWorkCanceled(interactions, threadID);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
// [ts] if (!didCatchError) {
// [ts] didCatchError = true;
// [ts] caughtError = error;
// [ts] }
// [ts] }
// [ts] });
// [ts] if (didCatchError) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw caughtError;)
// [ts] }
// [ts] }
// [ts] let SchedulerTracing = /*_priv___PURE__*/Object.freeze({
// [ts] __proto__: null,
// [ts] get __interactionsRef () { return interactionsRef; },
// [ts] get __subscriberRef () { return subscriberRef; },
// [ts] unstable_clear: unstable_clear,
// [ts] unstable_getCurrent: unstable_getCurrent,
// [ts] unstable_getThreadID: unstable_getThreadID,
// [ts] unstable_trace: unstable_trace,
// [ts] unstable_wrap: unstable_wrap,
// [ts] unstable_subscribe: unstable_subscribe,
// [ts] unstable_unsubscribe: unstable_unsubscribe
// [ts] });
// [ts] let ReactSharedInternals$1 = {
// [ts] ReactCurrentDispatcher: ReactCurrentDispatcher,
// [ts] ReactCurrentOwner: ReactCurrentOwner,
// [ts] IsSomeRendererActing: IsSomeRendererActing,
// Used by renderers to avoid bundling object-assign twice in UMD bundles:
// [ts] assign: objectAssign
// [ts] };
// [ts] {
// [ts] objectAssign(ReactSharedInternals$1, {
// These should not be included in production.
// [ts] ReactDebugCurrentFrame: ReactDebugCurrentFrame,
// Shim for React DOM 16.0.0 which still destructured (but not used) this.
// TODO: remove in React 17.0.
// [ts] ReactComponentTreeHook: {}
// [ts] });
// [ts] } // Re-export the schedule API(s) for UMD bundles.
// This avoids introducing a dependency on a new UMD global in a minor update,
// Since that would be a breaking change (e.g. for all existing CodeSandboxes).
// This re-export is only required for UMD bundles;
// CJS bundles use the shared NPM package.
// [ts] objectAssign(ReactSharedInternals$1, {
// [ts] Scheduler: Scheduler,
// [ts] SchedulerTracing: SchedulerTracing
// [ts] });
// [ts] {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] let frozenObject = Object.freeze({});
// [ts] let testMap = Map.init([[frozenObject, null]]);
// [ts] let testSet = Set.init([frozenObject]); // This is necessary for Rollup to not consider these unused.
// https://github.com/rollup/rollup/issues/1771
// TODO: we can remove these if Rollup fixes the bug.
// [ts] testMap.set(0, 0);
// [ts] testSet.add(0);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] }
// [ts] }
// [ts] let createElement$1 =  createElementWithValidation ;
// [ts] let cloneElement$1 =  cloneElementWithValidation ;
// [ts] let createFactory =  createFactoryWithValidation ;
// [ts] let Children = {
// [ts] map: mapChildren,
// [ts] forEach: forEachChildren,
// [ts] count: countChildren,
// [ts] toArray: toArray,
// [ts] only: onlyChild
// [ts] };
// [ts] exports.Children = Children;
// [ts] exports.Component = Component;
// [ts] exports.Fragment = REACT_FRAGMENT_TYPE;
// [ts] exports.Profiler = REACT_PROFILER_TYPE;
// [ts] exports.PureComponent = PureComponent;
// [ts] exports.StrictMode = REACT_STRICT_MODE_TYPE;
// [ts] exports.Suspense = REACT_SUSPENSE_TYPE;
// [ts] exports.__SECRET_INTERNALS_DO_NOT_USE_OR_YOU_WILL_BE_FIRED = ReactSharedInternals$1;
// [ts] exports.cloneElement = cloneElement$1;
// [ts] exports.createContext = createContext;
// [ts] exports.createElement = createElement$1;
// [ts] exports.createFactory = createFactory;
// [ts] exports.createRef = createRef;
// [ts] exports.forwardRef = forwardRef;
// [ts] exports.isValidElement = isValidElement;
// [ts] exports.lazy = lazy;
// [ts] exports.memo = memo;
// [ts] exports.useCallback = useCallback;
// [ts] exports.useContext = useContext;
// [ts] exports.useDebugValue = useDebugValue;
// [ts] exports.useEffect = useEffect;
// [ts] exports.useImperativeHandle = useImperativeHandle;
// [ts] exports.useLayoutEffect = useLayoutEffect;
// [ts] exports.useMemo = useMemo;
// [ts] exports.useReducer = useReducer;
// [ts] exports.useRef = useRef;
// [ts] exports.useState = useState;
// [ts] exports.version = ReactVersion;
// [ts] })));
