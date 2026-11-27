// Migrated from TypeScript (assets/reading-list/react_18.1.0.js) via cap from -ts

// TODO(cap-migrate): refine TS statement
// [ts] /**
// [ts] * @license React
// [ts] * react.development.js
// [ts] *
// [ts] * Copyright (c) Facebook, Inc. and its affiliates.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] */
// [ts] ((global, factory) => {
// [ts] if (@typeName(exports) == "object" && @typeName(module) != "undefined") { factory(exports) else; }
// [ts] if (@typeName(define) == "function" && define.amd) { define(["exports"], factory) else; }
// [ts] (global = global || self, factory(global.React = {}));
// [ts] }(this, ((exports) => { "use strict";
// [ts] let ReactVersion = "18.1.0";
// -----------------------------------------------------------------------------
// [ts] let enableScopeAPI = false; // Experimental Create Event Handle API.
// [ts] let enableCacheElement = false;
// [ts] let enableTransitionTracing = false; // No known bugs, but needs performance testing
// [ts] let enableLegacyHidden = false; // Enables unstable_avoidThisFallback feature in Fiber
// stuff. Intended to enable React core members to more easily debug scheduling
// issues in DEV builds.
// [ts] let enableDebugTracing = false; // Track which Fiber(s) schedule render work.
// ATTENTION
// [ts] let REACT_ELEMENT_TYPE =  "__symbol_react.element";
// [ts] let REACT_PORTAL_TYPE =  "__symbol_react.portal";
// [ts] let REACT_FRAGMENT_TYPE =  "__symbol_react.fragment";
// [ts] let REACT_STRICT_MODE_TYPE =  "__symbol_react.strict_mode";
// [ts] let REACT_PROFILER_TYPE =  "__symbol_react.profiler";
// [ts] let REACT_PROVIDER_TYPE =  "__symbol_react.provider";
// [ts] let REACT_CONTEXT_TYPE =  "__symbol_react.context";
// [ts] let REACT_FORWARD_REF_TYPE =  "__symbol_react.forward_ref";
// [ts] let REACT_SUSPENSE_TYPE =  "__symbol_react.suspense";
// [ts] let REACT_SUSPENSE_LIST_TYPE =  "__symbol_react.suspense_list";
// [ts] let REACT_MEMO_TYPE =  "__symbol_react.memo";
// [ts] let REACT_LAZY_TYPE =  "__symbol_react.lazy";
// [ts] let REACT_OFFSCREEN_TYPE =  "__symbol_react.offscreen";
// [ts] let MAYBE_ITERATOR_SYMBOL =  "__symbol_iterator";
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
// [ts] transition: null
// [ts] };
// [ts] let ReactCurrentActQueue = {
// [ts] current: null,
// Used to reproduce behavior of `batchedUpdates` in legacy mode.
// [ts] isBatchingLegacy: false,
// [ts] didScheduleLegacyUpdate: false
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
// [ts] let ReactDebugCurrentFrame = {};
// [ts] let currentExtraStackFrame = null;
// [ts] fn setExtraStackFrame(stack: auto) {
// [ts] {
// [ts] currentExtraStackFrame = stack;
// [ts] }
// [ts] }
// [ts] {
// [ts] ReactDebugCurrentFrame.setExtraStackFrame = (stack) => {
// [ts] {
// [ts] currentExtraStackFrame = stack;
// [ts] }
// [ts] }; // Stack implementation injected by the current renderer.
// [ts] ReactDebugCurrentFrame.getCurrentStack = null;
// [ts] ReactDebugCurrentFrame.getStackAddendum = () => {
// [ts] let stack = ""; // Add an extra top frame while an element is being validated
// [ts] if (currentExtraStackFrame) {
// [ts] stack += currentExtraStackFrame;
// [ts] } // Delegate to the injected renderer-specific implementation
// [ts] let impl = ReactDebugCurrentFrame.getCurrentStack;
// [ts] if (impl) {
// [ts] stack += impl() || "";
// [ts] }
// [ts] return stack;
// [ts] };
// [ts] }
// [ts] let ReactSharedInternals = {
// [ts] ReactCurrentDispatcher: ReactCurrentDispatcher,
// [ts] ReactCurrentBatchConfig: ReactCurrentBatchConfig,
// [ts] ReactCurrentOwner: ReactCurrentOwner
// [ts] };
// [ts] {
// [ts] ReactSharedInternals.ReactDebugCurrentFrame = ReactDebugCurrentFrame;
// [ts] ReactSharedInternals.ReactCurrentActQueue = ReactCurrentActQueue;
// [ts] }
// by calls to these methods by a Babel plugin.
//
// In PROD (or in packages without access to React internals),
// they are left as they are instead.
// [ts] fn warn(format: auto) {
// [ts] {
// [ts] {
// [ts] let _len = arguments.length, args = Array.init(if (_len > 1) _len - 1 else 0), _key = 1;
// [ts] while (_key < _len) { // [cap-migrate] for-step: _key++
// [ts] args[_key - 1] = arguments[_key];
// [ts] }
// [ts] printWarning("warn", format, args);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn error(format: auto) {
// [ts] {
// [ts] {
// [ts] let _len2 = arguments.length, args = Array.init(if (_len2 > 1) _len2 - 1 else 0), _key2 = 1;
// [ts] while (_key2 < _len2) { // [cap-migrate] for-step: _key2++
// [ts] args[_key2 - 1] = arguments[_key2];
// [ts] }
// [ts] printWarning("error", format, args);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn printWarning(level: auto, format: auto, args: auto) {
// When changing this logic, you might want to also
// update consoleWithStackDev.www.js as well.
// [ts] {
// [ts] let ReactDebugCurrentFrame = ReactSharedInternals.ReactDebugCurrentFrame;
// [ts] let stack = ReactDebugCurrentFrame.getStackAddendum();
// [ts] if (stack != "") {
// [ts] format += "%s";
// [ts] args = args.concat([stack]);
// [ts] } // eslint-disable-next-line react-internal/safe-string-coercion
// [ts] let argsWithFormat = args.map((item) => {
// [ts] return String(item);
// [ts] }); // Careful: RN currently depends on this prefix
// [ts] argsWithFormat.unshift("Warning: " + format); // We intentionally don't use spread (or .apply) directly because it
// breaks IE9: https://github.com/facebook/react/issues/13610
// eslint-disable-next-line react-internal/no-production-logging
// [ts] Function.prototype.apply.call(console[level], console, argsWithFormat);
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
// [ts] let assign = Object.assign;
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
// [ts] if (@typeName(partialState) != "object" && @typeName(partialState) != "function" && partialState != null) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('setState(...): takes an object of state variables to update or a ' + 'function which returns an object of state variables.');)
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
// [ts] assign(pureComponentPrototype, Component.prototype);
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
// [ts] let isArrayImpl = Array.isArray; // eslint-disable-next-line no-redeclare
// [ts] fn isArray(a: auto) {
// [ts] return isArrayImpl(a);
// [ts] }
// [ts] /*
// [ts] * The `'' + value` pattern (used in in perf-sensitive code) throws for Symbol
// [ts] * and Temporal.* types. See https://github.com/facebook/react/pull/22064.
// [ts] *
// [ts] * The functions in this module will throw an easier-to-understand,
// [ts] * easier-to-debug exception with a clear errors message message explaining the
// [ts] * problem. (Instead of a confusing exception thrown inside the implementation
// [ts] * of the `value` object).
// [ts] */
// $FlowFixMe only called in DEV, so void return is not possible.
// [ts] fn typeName(value: auto) {
// [ts] {
// toStringTag is needed for namespaced types like Temporal.Instant
// [ts] let hasToStringTag = @typeName(Symbol) == "function" && "__symbol_toStringTag";
// [ts] let type = hasToStringTag && value["__symbol_toStringTag"] || value.constructor.name || "Object";
// [ts] return type;
// [ts] }
// [ts] } // $FlowFixMe only called in DEV, so void return is not possible.
// [ts] fn willCoercionThrow(value: auto) {
// [ts] {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] testStringCoercion(value);
// [ts] return false;
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] return true;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn testStringCoercion(value: auto) {
// If you ended up here by following an exception call stack, here's what's
// happened: you supplied an object or symbol value to React (as a prop, key,
// DOM attribute, CSS property, string ref, etc.) and when React tried to
// coerce it to a string using `'' + value`, an exception was thrown.
//
// The most common types that will cause this exception are `Symbol` instances
// and Temporal objects like `Temporal.Instant`. But any object that has a
// `valueOf` or `[Symbol.toPrimitive]` method that throws will also cause this
// exception. (Library authors do this to prevent users from using built-in
// numeric operators like `+` or comparison operators like `>=` because custom
// methods are needed to perform accurate arithmetic or comparison.)
//
// To fix the problem, coerce this object or symbol value to a string before
// passing it to React. The most reliable way is usually `String(value)`.
//
// To find which value is throwing, check the browser or debugger console.
// Before this exception was thrown, there should be `console.error` output
// that shows the type (Symbol, Temporal.PlainDate, etc.) that caused the
// problem and how that type was used: key, atrribute, input value prop, etc.
// In most cases, this console output also shows the component and its
// ancestor components where the exception happened.
//
// eslint-disable-next-line react-internal/safe-string-coercion
// [ts] return "" + value;
// [ts] }
// [ts] fn checkKeyStringCoercion(value: auto) {
// [ts] {
// [ts] if (willCoercionThrow(value)) {
// [ts] error("The provided key is an unsupported type %s." + " This value must be coerced to a string before before using it here.", typeName(value));
// [ts] return testStringCoercion(value); // throw (to help callers find troubleshooting comments)
// [ts] }
// [ts] }
// [ts] }
// [ts] fn getWrappedName(outerType: auto, innerType: auto, wrapperName: auto) {
// [ts] let displayName = outerType.displayName;
// [ts] if (displayName) {
// [ts] return displayName;
// [ts] }
// [ts] let functionName = innerType.displayName || innerType.name || "";
// [ts] return if (functionName != "") wrapperName + "(" + functionName + ")" else wrapperName;
// [ts] } // Keep in sync with react-reconciler/getComponentNameFromFiber
// [ts] fn getContextName(type: auto) {
// [ts] return type.displayName || "Context";
// [ts] } // Note that the reconciler package should generally prefer to use getComponentNameFromFiber() instead.
// [ts] fn getComponentNameFromType(type: auto) {
// [ts] if (type == null) {
// Host root, text node or just invalid type.
// [ts] return null;
// [ts] }
// [ts] {
// [ts] if (@typeName(type.tag) == "number") {
// [ts] error("Received an unexpected object in getComponentNameFromType(). " + "This is likely a bug in React. Please file an issue.");
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
// [ts] let context = type;
// [ts] return getContextName(context) + ".Consumer";
// [ts] },
// [ts] REACT_PROVIDER_TYPE => {
// [ts] let provider = type;
// [ts] return getContextName(provider._context) + ".Provider";
// [ts] },
// [ts] REACT_FORWARD_REF_TYPE => {
// [ts] return getWrappedName(type, type.render, "ForwardRef");
// [ts] },
// [ts] REACT_MEMO_TYPE => {
// [ts] let outerName = type.displayName || null;
// [ts] if (outerName != null) {
// [ts] return outerName;
// [ts] }
// [ts] return getComponentNameFromType(type.type) || "Memo";
// [ts] },
// [ts] REACT_LAZY_TYPE => {
// [ts] {
// [ts] let lazyComponent = type;
// [ts] let payload = lazyComponent._payload;
// [ts] let init = lazyComponent._init;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return getComponentNameFromType(init(payload));
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (x) {)
// [ts] return null;
// [ts] }
// [ts] }
// eslint-disable-next-line no-fallthrough
// [ts] }
// [ts] }
// [ts] }
// [ts] return null;
// [ts] }
// [ts] let hasOwnProperty = Object.prototype.hasOwnProperty;
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
// [ts] if (hasOwnProperty.call(config, "ref")) {
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
// [ts] if (hasOwnProperty.call(config, "key")) {
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
// [ts] error("%s: `key` is not a prop. Trying to access it will result " + "in `undefined` being returned. If you need to access the same " + "value within the child component, you should pass it as a different " + "prop. (https://reactjs.org/link/special-props)", displayName);
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
// [ts] error("%s: `ref` is not a prop. Trying to access it will result " + "in `undefined` being returned. If you need to access the same " + "value within the child component, you should pass it as a different " + "prop. (https://reactjs.org/link/special-props)", displayName);
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
// [ts] let componentName = getComponentNameFromType(ReactCurrentOwner.current.type);
// [ts] if (!didWarnAboutStringRefs[componentName]) {
// [ts] error("Component \"%s\" contains the string ref \"%s\". " + "Support for string refs will be removed in a future major release. " + "This case cannot be automatically converted to an arrow function. " + "We ask you to manually fix this case by using useRef() or createRef() instead. " + "Learn more about using refs safely here: " + "https://reactjs.org/link/strict-mode-string-ref", componentName, config.ref);
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
// [ts] {
// [ts] checkKeyStringCoercion(config.key);
// [ts] }
// [ts] key = "" + config.key;
// [ts] }
// [ts] self = if (config.__self == null) null else config.__self;
// [ts] source = if (config.__source == null) null else config.__source; // Remaining properties are added to a new props object
// [ts] for (propName in config) {
// [ts] if (hasOwnProperty.call(config, propName) && !RESERVED_PROPS.hasOwnProperty(propName)) {
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
// [ts] if (element == null || element == null) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error("React.cloneElement(...): The argument must be a React element, but you passed " + element + ".");)
// [ts] }
// [ts] let propName; // Original props are copied
// [ts] let props = assign({}, element.props); // Reserved names are extracted
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
// [ts] {
// [ts] checkKeyStringCoercion(config.key);
// [ts] }
// [ts] key = "" + config.key;
// [ts] } // Remaining properties override existing props
// [ts] let defaultProps;
// [ts] if (element.type && element.type.defaultProps) {
// [ts] defaultProps = element.type.defaultProps;
// [ts] }
// [ts] for (propName in config) {
// [ts] if (hasOwnProperty.call(config, propName) && !RESERVED_PROPS.hasOwnProperty(propName)) {
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
// [ts] let escapedString = key.replace(escapeRegex, (match_val) => {
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
// [ts] return text.replace(userProvidedKeyEscapeRegex, "$&/");
// [ts] }
// [ts] /**
// [ts] * Generate a key string that identifies a element within a set.
// [ts] *
// [ts] * @param {*} element A element that could contain a manual key.
// [ts] * @param {number} index Index that is used if a manual key is not provided.
// [ts] * @return {string}
// [ts] */
// [ts] fn getElementKey(element: auto, index: auto) {
// Do some typechecking here since we call this blindly. We want to ensure
// that we don't block potential future ES APIs.
// [ts] if (@typeName(element) == "object" && element != null && element.key != null) {
// Explicit key
// [ts] {
// [ts] checkKeyStringCoercion(element.key);
// [ts] }
// [ts] return escape("" + element.key);
// [ts] } // Implicit key determined by the index in the set
// [ts] return index.toString(36);
// [ts] }
// [ts] fn mapIntoArray(children: auto, array: auto, escapedPrefix: auto, nameSoFar: auto, callback: auto) {
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
// [ts] let _child = children;
// [ts] let mappedChild = callback(_child); // If it's the only child, treat the name as if it was wrapped in an array
// so that it's consistent if the number of children grows:
// [ts] let childKey = if (nameSoFar == "") SEPARATOR + getElementKey(_child, 0) else nameSoFar;
// [ts] if (isArray(mappedChild)) {
// [ts] let escapedChildKey = "";
// [ts] if (childKey != null) {
// [ts] escapedChildKey = escapeUserProvidedKey(childKey) + "/";
// [ts] }
// [ts] mapIntoArray(mappedChild, array, escapedChildKey, "", (c) => {
// [ts] return c;
// [ts] });
// [ts] } else if (mappedChild != null) {
// [ts] if (isValidElement(mappedChild)) {
// [ts] {
// The `if` statement here prevents auto-disabling of the safe
// coercion ESLint rule, so we must manually disable it below.
// $FlowFixMe Flow incorrectly thinks React.Portal doesn't have a key
// [ts] if (mappedChild.key && (!_child || _child.key != mappedChild.key)) {
// [ts] checkKeyStringCoercion(mappedChild.key);
// [ts] }
// [ts] }
// [ts] mappedChild = cloneAndReplaceKey(mappedChild, // Keep both the (mapped) and old keys if they differ, just as
// traverseAllChildren used to do for objects as children
// [ts] escapedPrefix + ( // $FlowFixMe Flow incorrectly thinks React.Portal doesn't have a key
// [ts] mappedChild.key && (!_child || _child.key != mappedChild.key) ? // $FlowFixMe Flow incorrectly thinks existing element's key can be a number
// eslint-disable-next-line react-internal/safe-string-coercion
// [ts] escapeUserProvidedKey("" + mappedChild.key) + "/" : "") + childKey);
// [ts] }
// [ts] array.push(mappedChild);
// [ts] }
// [ts] return 1;
// [ts] }
// [ts] let child;
// [ts] let nextName;
// [ts] let subtreeCount = 0; // Count of children found in the current subtree.
// [ts] let nextNamePrefix = if (nameSoFar == "") SEPARATOR else nameSoFar + SUBSEPARATOR;
// [ts] if (isArray(children)) {
// [ts] let i = 0;
// [ts] while (i < children.length) { // [cap-migrate] for-step: i++
// [ts] child = children[i];
// [ts] nextName = nextNamePrefix + getElementKey(child, i);
// [ts] subtreeCount += mapIntoArray(child, array, escapedPrefix, nextName, callback);
// [ts] }
// [ts] } else {
// [ts] let iteratorFn = getIteratorFn(children);
// [ts] if (@typeName(iteratorFn) == "function") {
// [ts] let iterableChildren = children;
// [ts] {
// Warn about using Maps as children
// [ts] if (iteratorFn == iterableChildren.entries) {
// [ts] if (!didWarnAboutMaps) {
// [ts] warn("Using Maps as children is not supported. " + "Use an array of keyed ReactElements instead.");
// [ts] }
// [ts] didWarnAboutMaps = true;
// [ts] }
// [ts] }
// [ts] let iterator = iteratorFn.call(iterableChildren);
// [ts] let step;
// [ts] let ii = 0;
// [ts] while (!(step = iterator.next()).done) {
// [ts] child = step.value;
// [ts] nextName = nextNamePrefix + getElementKey(child, ii++);
// [ts] subtreeCount += mapIntoArray(child, array, escapedPrefix, nextName, callback);
// [ts] }
// [ts] } else if (type == "object") {
// eslint-disable-next-line react-internal/safe-string-coercion
// [ts] let childrenString = String(children);
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error("Objects are not valid as a React child (found: " + (childrenString === '[object Object]' ? 'object with keys {' + Object.keys(children).join(', ') + '}' : childrenString) + "). " + 'If you meant to render a collection of children, use an array ' + 'instead.');)
// [ts] }
// [ts] }
// [ts] return subtreeCount;
// [ts] }
// [ts] /**
// [ts] * Maps children that are typically specified as `props.children`.
// [ts] *
// [ts] * See https://reactjs.org/docs/react-api.html#reactchildrenmap
// [ts] *
// [ts] * The provided mapFunction(child, index) will be called for each
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
// [ts] let count = 0;
// [ts] mapIntoArray(children, result, "", "", (child) => {
// [ts] return func.call(context, child, count++);
// [ts] });
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
// [ts] let n = 0;
// [ts] mapChildren(children, () => {
// [ts] n++; // Don't return anything
// [ts] });
// [ts] return n;
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
// [ts] mapChildren(children, () => {
// [ts] forEachFunc.apply(this, arguments); // Don't return anything.
// [ts] }, forEachContext);
// [ts] }
// [ts] /**
// [ts] * Flatten a children object (typically specified as `props.children`) and
// [ts] * return an array with appropriately re-keyed children.
// [ts] *
// [ts] * See https://reactjs.org/docs/react-api.html#reactchildrentoarray
// [ts] */
// [ts] fn toArray(children: auto) {
// [ts] return mapChildren(children, (child) => {
// [ts] return child;
// [ts] }) || [];
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
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('React.Children.only expected to receive a single React element child.');)
// [ts] }
// [ts] return children;
// [ts] }
// [ts] fn createContext(defaultValue: auto) {
// TODO: Second argument used to be an optional `calculateChangedBits`
// function. Warn to reserve for future use?
// [ts] let context = {
// [ts] $$typeof: REACT_CONTEXT_TYPE,
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
// [ts] Consumer: null,
// Add these to use same hidden class in VM as ServerContext
// [ts] _defaultValue: null,
// [ts] _globalName: null
// [ts] };
// [ts] context.Provider = {
// [ts] $$typeof: REACT_PROVIDER_TYPE,
// [ts] _context: context
// [ts] };
// [ts] let hasWarnedAboutUsingNestedContextConsumers = false;
// [ts] let hasWarnedAboutUsingConsumerProvider = false;
// [ts] let hasWarnedAboutDisplayNameOnConsumer = false;
// [ts] {
// A separate object, but proxies back to the original context object for
// backwards compatibility. It has a different $$typeof, so we can properly
// warn for the incorrect usage of Context as a Consumer.
// [ts] let Consumer = {
// [ts] $$typeof: REACT_CONTEXT_TYPE,
// [ts] _context: context
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
// [ts-skip] },
// [ts-skip] displayName: {
// [ts-skip] get: function () {
// [ts-skip] return context.displayName;
// [ts-skip] },
// [ts-skip] set: function (displayName) {
// [ts-skip] if (!hasWarnedAboutDisplayNameOnConsumer) {
// [ts-skip] warn('Setting `displayName` on Context.Consumer has no effect. ' + "You should set it directly on the context with Context.displayName = '%s'.", displayName);
// [ts-skip]
// [ts-skip] hasWarnedAboutDisplayNameOnConsumer = true;
// [ts-skip] }
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
// [ts] let Uninitialized = -1;
// [ts] let Pending = 0;
// [ts] let Resolved = 1;
// [ts] let Rejected = 2;
// [ts] fn lazyInitializer(payload: auto) {
// [ts] if (payload._status == Uninitialized) {
// [ts] let ctor = payload._result;
// [ts] let thenable = ctor(); // Transition to the next state.
// This might throw either because it's missing or throws. If so, we treat it
// as still uninitialized and try again next time. Which is the same as what
// happens if the ctor or any wrappers processing the ctor throws. This might
// end up fixing it if the resolution was a concurrency bug.
// [ts] thenable.then((moduleObject) => {
// [ts] if (payload._status == Pending || payload._status == Uninitialized) {
// Transition to the next state.
// [ts] let resolved = payload;
// [ts] resolved._status = Resolved;
// [ts] resolved._result = moduleObject;
// [ts] }
// [ts] }, (error) => {
// [ts] if (payload._status == Pending || payload._status == Uninitialized) {
// Transition to the next state.
// [ts] let rejected = payload;
// [ts] rejected._status = Rejected;
// [ts] rejected._result = error;
// [ts] }
// [ts] });
// [ts] if (payload._status == Uninitialized) {
// In case, we're still uninitialized, then we're waiting for the thenable
// to resolve. Set it as pending in the meantime.
// [ts] let pending = payload;
// [ts] pending._status = Pending;
// [ts] pending._result = thenable;
// [ts] }
// [ts] }
// [ts] if (payload._status == Resolved) {
// [ts] let moduleObject = payload._result;
// [ts] {
// [ts] if (moduleObject == null) {
// [ts] error("lazy: Expected the result of a dynamic imp" + "ort() call. " + "Instead received: %s\n\nYour code should look like: \n  " + // Break up imports to avoid accidentally parsing them as dependencies.
// [ts] "const MyComponent = lazy(() => imp" + "ort('./MyComponent'))\n\n" + "Did you accidentally put curly braces around the import?", moduleObject);
// [ts] }
// [ts] }
// [ts] {
// [ts] if (!("default" == moduleObject)) {
// [ts] error("lazy: Expected the result of a dynamic imp" + "ort() call. " + "Instead received: %s\n\nYour code should look like: \n  " + // Break up imports to avoid accidentally parsing them as dependencies.
// [ts] "const MyComponent = lazy(() => imp" + "ort('./MyComponent'))", moduleObject);
// [ts] }
// [ts] }
// [ts] return moduleObject.default;
// [ts] } else {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw payload._result;)
// [ts] }
// [ts] }
// [ts] fn lazy(ctor: auto) {
// [ts] let payload = {
// We use these fields to store the result.
// [ts] _status: Uninitialized,
// [ts] _result: ctor
// [ts] };
// [ts] let lazyType = {
// [ts] $$typeof: REACT_LAZY_TYPE,
// [ts] _payload: payload,
// [ts] _init: lazyInitializer
// [ts] };
// [ts] {
// In production, this would just set it on the object.
// [ts] let defaultProps;
// [ts] let propTypes; // $FlowFixMe
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
// [ts-skip] // $FlowFixMe
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
// [ts-skip] // $FlowFixMe
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
// [ts] let elementType = {
// [ts] $$typeof: REACT_FORWARD_REF_TYPE,
// [ts] render: render
// [ts] };
// [ts] {
// [ts] let ownName;
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(elementType, 'displayName', {
// [ts-skip] enumerable: false,
// [ts-skip] configurable: true,
// [ts-skip] get: function () {
// [ts-skip] return ownName;
// [ts-skip] },
// [ts-skip] set: function (name) {
// [ts-skip] ownName = name; // The inner component shouldn't inherit this display name in most cases,
// [ts-skip] // because the component may be used elsewhere.
// [ts-skip] // But it's nice for anonymous functions to inherit the name,
// [ts-skip] // so that our component-stack generation logic will display their frames.
// [ts-skip] // An anonymous function generally suggests a pattern like:
// [ts-skip] //   React.forwardRef((props, ref) => {...});
// [ts-skip] // This kind of inner function is not used elsewhere so the side effect is okay.
// [ts-skip]
// [ts-skip] if (!render.name && !render.displayName) {
// [ts-skip] render.displayName = name;
// [ts-skip] }
// [ts-skip] }
// [ts-skip] });
// [ts] }
// [ts] return elementType;
// [ts] }
// [ts] let REACT_MODULE_REFERENCE;
// [ts] {
// [ts] REACT_MODULE_REFERENCE = "__symbol_react.module.reference";
// [ts] }
// [ts] fn isValidElementType(type: auto) {
// [ts] if (@typeName(type) == "string" || @typeName(type) == "function") {
// [ts] return true;
// [ts] } // Note: typeof might be other than 'symbol' or 'number' (e.g. if it's a polyfill).
// [ts] if (type == REACT_FRAGMENT_TYPE || type == REACT_PROFILER_TYPE || enableDebugTracing  || type == REACT_STRICT_MODE_TYPE || type == REACT_SUSPENSE_TYPE || type == REACT_SUSPENSE_LIST_TYPE || enableLegacyHidden  || type == REACT_OFFSCREEN_TYPE || enableScopeAPI  || enableCacheElement  || enableTransitionTracing ) {
// [ts] return true;
// [ts] }
// [ts] if (@typeName(type) == "object" && type != null) {
// [ts] if (type.$$typeof == REACT_LAZY_TYPE || type.$$typeof == REACT_MEMO_TYPE || type.$$typeof == REACT_PROVIDER_TYPE || type.$$typeof == REACT_CONTEXT_TYPE || type.$$typeof == REACT_FORWARD_REF_TYPE || // This needs to include all possible module reference object
// types supported by any Flight configuration anywhere since
// we don't know which Flight build this will end up being used
// with.
// [ts] type.$$typeof == REACT_MODULE_REFERENCE || type.getModuleId != null) {
// [ts] return true;
// [ts] }
// [ts] }
// [ts] return false;
// [ts] }
// [ts] fn memo(type: auto, compare: auto) {
// [ts] {
// [ts] if (!isValidElementType(type)) {
// [ts] error(if ("memo: The first argument must be a component. Instead " + "received: %s", type == null) "null" else @typeName(type));
// [ts] }
// [ts] }
// [ts] let elementType = {
// [ts] $$typeof: REACT_MEMO_TYPE,
// [ts] type: type,
// [ts] compare: if (compare == null) null else compare
// [ts] };
// [ts] {
// [ts] let ownName;
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(elementType, 'displayName', {
// [ts-skip] enumerable: false,
// [ts-skip] configurable: true,
// [ts-skip] get: function () {
// [ts-skip] return ownName;
// [ts-skip] },
// [ts-skip] set: function (name) {
// [ts-skip] ownName = name; // The inner component shouldn't inherit this display name in most cases,
// [ts-skip] // because the component may be used elsewhere.
// [ts-skip] // But it's nice for anonymous functions to inherit the name,
// [ts-skip] // so that our component-stack generation logic will display their frames.
// [ts-skip] // An anonymous function generally suggests a pattern like:
// [ts-skip] //   React.memo((props) => {...});
// [ts-skip] // This kind of inner function is not used elsewhere so the side effect is okay.
// [ts-skip]
// [ts-skip] if (!type.name && !type.displayName) {
// [ts-skip] type.displayName = name;
// [ts-skip] }
// [ts-skip] }
// [ts-skip] });
// [ts] }
// [ts] return elementType;
// [ts] }
// [ts] fn resolveDispatcher() {
// [ts] let dispatcher = ReactCurrentDispatcher.current;
// [ts] {
// [ts] if (dispatcher == null) {
// [ts] error("Invalid hook call. Hooks can only be called inside of the body of a function component. This could happen for" + " one of the following reasons:\n" + "1. You might have mismatching versions of React and the renderer (such as React DOM)\n" + "2. You might be breaking the Rules of Hooks\n" + "3. You might have more than one copy of React in the same app\n" + "See https://reactjs.org/link/invalid-hook-call for tips about how to debug and fix this problem.");
// [ts] }
// [ts] } // Will result in a null access error if accessed outside render phase. We
// intentionally don't throw our own error because this is in a hot path.
// Also helps ensure this is inlined.
// [ts] return dispatcher;
// [ts] }
// [ts] fn useContext(Context: auto) {
// [ts] let dispatcher = resolveDispatcher();
// [ts] {
// TODO: add a more generic warning for invalid values.
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
// [ts] return dispatcher.useContext(Context);
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
// [ts] fn useInsertionEffect(create: auto, deps: auto) {
// [ts] let dispatcher = resolveDispatcher();
// [ts] return dispatcher.useInsertionEffect(create, deps);
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
// [ts] fn useTransition() {
// [ts] let dispatcher = resolveDispatcher();
// [ts] return dispatcher.useTransition();
// [ts] }
// [ts] fn useDeferredValue(value: auto) {
// [ts] let dispatcher = resolveDispatcher();
// [ts] return dispatcher.useDeferredValue(value);
// [ts] }
// [ts] fn useId() {
// [ts] let dispatcher = resolveDispatcher();
// [ts] return dispatcher.useId();
// [ts] }
// [ts] fn useSyncExternalStore(subscribe: auto, getSnapshot: auto, getServerSnapshot: auto) {
// [ts] let dispatcher = resolveDispatcher();
// [ts] return dispatcher.useSyncExternalStore(subscribe, getSnapshot, getServerSnapshot);
// [ts] }
// Helpers to patch console.logs to avoid logging during side-effect free
// replaying on render function. This currently only patches the object
// lazily which won't cover if the log function was extracted eagerly.
// We could also eagerly patch the method.
// [ts] let disabledDepth = 0;
// [ts] let prevLog;
// [ts] let prevInfo;
// [ts] let prevWarn;
// [ts] let prevError;
// [ts] let prevGroup;
// [ts] let prevGroupCollapsed;
// [ts] let prevGroupEnd;
// [ts] fn disabledLog() {}
// [ts] disabledLog.__reactDisabledLog = true;
// [ts] fn disableLogs() {
// [ts] {
// [ts] if (disabledDepth == 0) {
// [ts] /* eslint-disable react-internal/no-production-logging */
// [ts] prevLog = console.log;
// [ts] prevInfo = console.info;
// [ts] prevWarn = console.warn;
// [ts] prevError = console.error;
// [ts] prevGroup = console.group;
// [ts] prevGroupCollapsed = console.groupCollapsed;
// [ts] prevGroupEnd = console.groupEnd; // https://github.com/facebook/react/issues/19099
// [ts] let props = {
// [ts] configurable: true,
// [ts] enumerable: true,
// [ts] value: disabledLog,
// [ts] writable: true
// [ts] }; // $FlowFixMe Flow thinks console is immutable.
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperties(console, {
// [ts-skip] info: props,
// [ts-skip] log: props,
// [ts-skip] warn: props,
// [ts-skip] error: props,
// [ts-skip] group: props,
// [ts-skip] groupCollapsed: props,
// [ts-skip] groupEnd: props
// [ts-skip] });
// [ts] /* eslint-enable react-internal/no-production-logging */
// [ts] }
// [ts] disabledDepth++;
// [ts] }
// [ts] }
// [ts] fn reenableLogs() {
// [ts] {
// [ts] disabledDepth--;
// [ts] if (disabledDepth == 0) {
// [ts] /* eslint-disable react-internal/no-production-logging */
// [ts] let props = {
// [ts] configurable: true,
// [ts] enumerable: true,
// [ts] writable: true
// [ts] }; // $FlowFixMe Flow thinks console is immutable.
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperties(console, {
// [ts-skip] log: assign({}, props, {
// [ts-skip] value: prevLog
// [ts-skip] }),
// [ts-skip] info: assign({}, props, {
// [ts-skip] value: prevInfo
// [ts-skip] }),
// [ts-skip] warn: assign({}, props, {
// [ts-skip] value: prevWarn
// [ts-skip] }),
// [ts-skip] error: assign({}, props, {
// [ts-skip] value: prevError
// [ts-skip] }),
// [ts-skip] group: assign({}, props, {
// [ts-skip] value: prevGroup
// [ts-skip] }),
// [ts-skip] groupCollapsed: assign({}, props, {
// [ts-skip] value: prevGroupCollapsed
// [ts-skip] }),
// [ts-skip] groupEnd: assign({}, props, {
// [ts-skip] value: prevGroupEnd
// [ts-skip] })
// [ts-skip] });
// [ts] /* eslint-enable react-internal/no-production-logging */
// [ts] }
// [ts] if (disabledDepth < 0) {
// [ts] error("disabledDepth fell below zero. " + "This is a bug in React. Please file an issue.");
// [ts] }
// [ts] }
// [ts] }
// [ts] let ReactCurrentDispatcher$1 = ReactSharedInternals.ReactCurrentDispatcher;
// [ts] let prefix;
// [ts] fn describeBuiltInComponentFrame(name: auto, source: auto, ownerFn: auto) {
// [ts] {
// [ts] if (prefix == null) {
// Extract the VM specific prefix used by each line.
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error();)
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (x) {)
// [ts] let match_val = x.stack.trim().match("\\n( *(at )?)");
// [ts] prefix = match_val && match_val[1] || "";
// [ts] }
// [ts] } // We use the prefix to ensure our stacks line up with native stack frames.
// [ts] return "\n" + prefix + name;
// [ts] }
// [ts] }
// [ts] let reentry = false;
// [ts] let componentFrameCache;
// [ts] {
// [ts] let PossiblyWeakMap = if (@typeName(WeakMap) == "function") WeakMap else Map;
// [ts] componentFrameCache = PossiblyWeakMap.init();
// [ts] }
// [ts] fn describeNativeComponentFrame(fn: auto, construct: auto) {
// If something asked for a stack inside a fake render, it should get ignored.
// [ts] if ( !fn || reentry) {
// [ts] return "";
// [ts] }
// [ts] {
// [ts] let frame = componentFrameCache.get(fn);
// [ts] if (frame != null) {
// [ts] return frame;
// [ts] }
// [ts] }
// [ts] let control;
// [ts] reentry = true;
// [ts] let previousPrepareStackTrace = Error.prepareStackTrace; // $FlowFixMe It does accept undefined.
// [ts] Error.prepareStackTrace = null;
// [ts] let previousDispatcher;
// [ts] {
// [ts] previousDispatcher = ReactCurrentDispatcher$1.current; // Set the dispatcher in DEV because this might be call in the render function
// for warnings.
// [ts] ReactCurrentDispatcher$1.current = null;
// [ts] disableLogs();
// [ts] }
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// This should throw.
// [ts] if (construct) {
// Something should be setting the props in the constructor.
// [ts] let Fake = () => {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error();)
// [ts] }; // $FlowFixMe
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(Fake.prototype, 'props', {
// [ts-skip] set: function () {
// [ts-skip] // We use a throwing setter instead of frozen or non-writable props
// [ts-skip] // because that won't throw in a non-strict mode function.
// [ts-skip] throw Error();
// [ts-skip] }
// [ts-skip] });
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: if (typeof Reflect === 'object' && Reflect.construct) {
// [ts-skip] // We construct a different control for this case to include any extra
// [ts-skip] // frames added by the construct call.
// [ts-skip] try {
// [ts-skip] Reflect.construct(Fake, []);
// [ts-skip] } catch (x) {
// [ts-skip] control = x;
// [ts-skip] }
// [ts-skip]
// [ts-skip] Reflect.construct(fn, [], Fake);
// [ts-skip] } else {
// [ts-skip] try {
// [ts-skip] Fake.call();
// [ts-skip] } catch (x) {
// [ts-skip] control = x;
// [ts-skip] }
// [ts-skip]
// [ts-skip] fn.call(Fake.prototype);
// [ts-skip] }
// [ts] } else {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return; // TODO(cap-migrate): throw -> error return (throw Error();)
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (x) {)
// [ts] control = x;
// [ts] }
// [ts] fn();
// [ts] }
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (sample) {)
// This is inlined manually because closure doesn't do it for us.
// [ts] if (sample && control && @typeName(sample.stack) == "string") {
// This extracts the first frame from the sample that isn't also in the control.
// Skipping one frame that we assume is the frame that calls the two.
// [ts] let sampleLines = sample.stack.split("\n");
// [ts] let controlLines = control.stack.split("\n");
// [ts] let s = sampleLines.length - 1;
// [ts] let c = controlLines.length - 1;
// [ts] while (s >= 1 && c >= 0 && sampleLines[s] != controlLines[c]) {
// We expect at least one stack frame to be shared.
// Typically this will be the root most one. However, stack frames may be
// cut off due to maximum stack limits. In this case, one maybe cut off
// earlier than the other. We assume that the sample is longer or the same
// and there for cut off earlier. So we should find the root most frame in
// the sample somewhere in the control.
// [ts] c--;
// [ts] }
// [ts] while (s >= 1 && c >= 0) { // [cap-migrate] for-step: s--, c--
// Next we find the first one that isn't the same which should be the
// frame that called our sample function and the control.
// [ts] if (sampleLines[s] != controlLines[c]) {
// In V8, the first line is describing the message but other VMs don't.
// If we're about to return the first line, and the control is also on the same
// line, that's a pretty good indicator that our sample threw at same line as
// the control. I.e. before we entered the sample frame. So we ignore this result.
// This can happen if you passed a class to function component, or non-function.
// [ts] if (s != 1 || c != 1) {
// [ts] do {
// [ts] s--;
// [ts] c--; // We may still have similar intermediate frames from the construct call.
// The next one that isn't the same should be our match though.
// [ts] if (c < 0 || sampleLines[s] != controlLines[c]) {
// V8 adds a "new" prefix for native classes. Let's remove it to make it prettier.
// [ts] let _frame = "\n" + sampleLines[s].replace(" at new ", " at "); // If our component frame is labeled "<anonymous>"
// but we have a user-provided "displayName"
// splice it in to make the stack more readable.
// [ts] if (fn.displayName && _frame.includes("<anonymous>")) {
// [ts] _frame = _frame.replace("<anonymous>", fn.displayName);
// [ts] }
// [ts] {
// [ts] if (@typeName(fn) == "function") {
// [ts] componentFrameCache.set(fn, _frame);
// [ts] }
// [ts] } // Return the line we found.
// [ts] return _frame;
// [ts] }
// [ts] } while (s >= 1 && c >= 0);
// [ts] }
// [ts] break;
// [ts] }
// [ts] }
// [ts] }
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] reentry = false;
// [ts] {
// [ts] ReactCurrentDispatcher$1.current = previousDispatcher;
// [ts] reenableLogs();
// [ts] }
// [ts] Error.prepareStackTrace = previousPrepareStackTrace;
// [ts] } // Fallback to just using the name if we couldn't make it throw.
// [ts] let name = if (fn) fn.displayName || fn.name else "";
// [ts] let syntheticFrame = if (name) describeBuiltInComponentFrame(name) else "";
// [ts] {
// [ts] if (@typeName(fn) == "function") {
// [ts] componentFrameCache.set(fn, syntheticFrame);
// [ts] }
// [ts] }
// [ts] return syntheticFrame;
// [ts] }
// [ts] fn describeFunctionComponentFrame(fn: auto, source: auto, ownerFn: auto) {
// [ts] {
// [ts] return describeNativeComponentFrame(fn, false);
// [ts] }
// [ts] }
// [ts] fn shouldConstruct(Component: auto) {
// [ts] let prototype = Component.prototype;
// [ts] return !!(prototype && prototype.isReactComponent);
// [ts] }
// [ts] fn describeUnknownElementTypeFrameInDEV(type: auto, source: auto, ownerFn: auto) {
// [ts] if (type == null) {
// [ts] return "";
// [ts] }
// [ts] if (@typeName(type) == "function") {
// [ts] {
// [ts] return describeNativeComponentFrame(type, shouldConstruct(type));
// [ts] }
// [ts] }
// [ts] if (@typeName(type) == "string") {
// [ts] return describeBuiltInComponentFrame(type);
// [ts] }
// [ts] switch (type) {
// [ts] REACT_SUSPENSE_TYPE => {
// [ts] return describeBuiltInComponentFrame("Suspense");
// [ts] },
// [ts] REACT_SUSPENSE_LIST_TYPE => {
// [ts] return describeBuiltInComponentFrame("SuspenseList");
// [ts] }
// [ts] }
// [ts] if (@typeName(type) == "object") {
// [ts] switch (type.$$typeof) {
// [ts] REACT_FORWARD_REF_TYPE => {
// [ts] return describeFunctionComponentFrame(type.render);
// [ts] },
// Memo may contain any component type so we recursively resolve it.
// [ts] REACT_MEMO_TYPE => {
// [ts] return describeUnknownElementTypeFrameInDEV(type.type, source, ownerFn);
// [ts] },
// [ts] REACT_LAZY_TYPE => {
// [ts] {
// [ts] let lazyComponent = type;
// [ts] let payload = lazyComponent._payload;
// [ts] let init = lazyComponent._init;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// Lazy may contain any component type so we recursively resolve it.
// [ts] return describeUnknownElementTypeFrameInDEV(init(payload), source, ownerFn);
// [ts] } // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (x) {})
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] return "";
// [ts] }
// [ts] let loggedTypeFailures = {};
// [ts] let ReactDebugCurrentFrame$1 = ReactSharedInternals.ReactDebugCurrentFrame;
// [ts] fn setCurrentlyValidatingElement(element: auto) {
// [ts] {
// [ts] if (element) {
// [ts] let owner = element._owner;
// [ts] let stack = describeUnknownElementTypeFrameInDEV(if (element.type, element._source, owner) owner.type else null);
// [ts] ReactDebugCurrentFrame$1.setExtraStackFrame(stack);
// [ts] } else {
// [ts] ReactDebugCurrentFrame$1.setExtraStackFrame(null);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn checkPropTypes(typeSpecs: auto, values: auto, location: auto, componentName: auto, element: auto) {
// [ts] {
// $FlowFixMe This is okay but Flow doesn't know it.
// [ts] let has = Function.call.bind(hasOwnProperty);
// [ts] for (let typeSpecName in typeSpecs) {
// [ts] if (has(typeSpecs, typeSpecName)) {
// [ts] let error$1 = void 0; // Prop type validation may throw. In case they do, we don't want to
// fail the render phase where it didn't fail before. So we log it.
// After these have been cleaned up, we'll let them throw.
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// This is intentionally an invariant that gets caught. It's the same
// behavior as without this statement except with a better message.
// [ts] if (@typeName(typeSpecs[typeSpecName]) != "function") {
// eslint-disable-next-line react-internal/prod-error-codes
// [ts] let err = Error((componentName || "React class") + ": " + location + " type `" + typeSpecName + "` is invalid; " + "it must be a function, usually from the `prop-types` package, but received `" + @typeName(typeSpecs[typeSpecName]) + "`." + "This often happens because of typos such as `PropTypes.function` instead of `PropTypes.func`.");
// [ts] err.name = "Invariant Violation";
// [ts] return; // TODO(cap-migrate): throw -> error return (throw err;)
// [ts] }
// [ts] error$1 = typeSpecs[typeSpecName](values, typeSpecName, componentName, location, null, "SECRET_DO_NOT_PASS_THIS_OR_YOU_WILL_BE_FIRED");
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (ex) {)
// [ts] error$1 = ex;
// [ts] }
// [ts] if (error$1 && !(error$1 == Error)) {
// [ts] setCurrentlyValidatingElement(element);
// [ts] error("%s: type specification of %s" + " `%s` is invalid; the type checker " + "function must return `null` or an `Error` but returned a %s. " + "You may have forgotten to pass an argument to the type checker " + "creator (arrayOf, instanceOf, objectOf, oneOf, oneOfType, and " + "shape all require an argument).", componentName || "React class", location, typeSpecName, @typeName(error$1));
// [ts] setCurrentlyValidatingElement(null);
// [ts] }
// [ts] if (error$1 == Error && !(error$1.message == loggedTypeFailures)) {
// Only monitor this failure once because there tends to be a lot of the
// same error.
// [ts] loggedTypeFailures[error$1.message] = true;
// [ts] setCurrentlyValidatingElement(element);
// [ts] error("Failed %s type: %s", location, error$1.message);
// [ts] setCurrentlyValidatingElement(null);
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] (element) => {
// [ts] {
// [ts] if (element) {
// [ts] let owner = element._owner;
// [ts] let stack = describeUnknownElementTypeFrameInDEV(if (element.type, element._source, owner) owner.type else null);
// [ts] setExtraStackFrame(stack);
// [ts] } else {
// [ts] setExtraStackFrame(null);
// [ts] }
// [ts] }
// [ts] }
// [ts] let propTypesMisspellWarningShown;
// [ts] {
// [ts] propTypesMisspellWarningShown = false;
// [ts] }
// [ts] fn getDeclarationErrorAddendum() {
// [ts] if (ReactCurrentOwner.current) {
// [ts] let name = getComponentNameFromType(ReactCurrentOwner.current.type);
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
// [ts] childOwner = " It was passed a child from " + getComponentNameFromType(element._owner.type) + ".";
// [ts] }
// [ts] {
// [ts] setCurrentlyValidatingElement$1(element);
// [ts] error("Each child in a list should have a unique \"key\" prop." + "%s%s See https://reactjs.org/link/warning-keys for more information.", currentComponentErrorInfo, childOwner);
// [ts] setCurrentlyValidatingElement$1(null);
// [ts] }
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
// [ts] if (isArray(node)) {
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
// Intentionally inside to avoid triggering lazy initializers:
// [ts] let name = getComponentNameFromType(type);
// [ts] checkPropTypes(propTypes, element.props, "prop", name, element);
// [ts] } else if (type.PropTypes != null && !propTypesMisspellWarningShown) {
// [ts] propTypesMisspellWarningShown = true; // Intentionally inside to avoid triggering lazy initializers:
// [ts] let _name = getComponentNameFromType(type);
// [ts] error("Component %s declared `PropTypes` instead of `propTypes`. Did you misspell the property assignment?", _name || "Unknown");
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
// [ts] let keys = Object.keys(fragment.props);
// [ts] let i = 0;
// [ts] while (i < keys.length) { // [cap-migrate] for-step: i++
// [ts] let key = keys[i];
// [ts] if (key != "children" && key != "key") {
// [ts] setCurrentlyValidatingElement$1(fragment);
// [ts] error("Invalid prop `%s` supplied to `React.Fragment`. " + "React.Fragment can only have `key` and `children` props.", key);
// [ts] setCurrentlyValidatingElement$1(null);
// [ts] break;
// [ts] }
// [ts] }
// [ts] if (fragment.ref != null) {
// [ts] setCurrentlyValidatingElement$1(fragment);
// [ts] error("Invalid attribute `ref` supplied to `React.Fragment`.");
// [ts] setCurrentlyValidatingElement$1(null);
// [ts] }
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
// [ts] } else if (isArray(type)) {
// [ts] typeString = "array";
// [ts] } else if (type != null && type.$$typeof == REACT_ELEMENT_TYPE) {
// [ts] typeString = "<" + (getComponentNameFromType(type.type) || "Unknown") + " />";
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
// [ts] let enableProfiling = false;
// [ts] let frameYieldMs = 5;
// [ts] fn push(heap: auto, node: auto) {
// [ts] let index = heap.length;
// [ts] heap.push(node);
// [ts] siftUp(heap, node, index);
// [ts] }
// [ts] fn peek(heap: auto) {
// [ts] return if (heap.length == 0) null else heap[0];
// [ts] }
// [ts] fn pop(heap: auto) {
// [ts] if (heap.length == 0) {
// [ts] return null;
// [ts] }
// [ts] let first = heap[0];
// [ts] let last = heap.pop();
// [ts] if (last != first) {
// [ts] heap[0] = last;
// [ts] siftDown(heap, last, 0);
// [ts] }
// [ts] return first;
// [ts] }
// [ts] fn siftUp(heap: auto, node: auto, i: auto) {
// [ts] let index = i;
// [ts] while (index > 0) {
// [ts] let parentIndex = index - 1 >> 1;
// [ts] let parent = heap[parentIndex];
// [ts] if (compare(parent, node) > 0) {
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
// [ts] let halfLength = length >> 1;
// [ts] while (index < halfLength) {
// [ts] let leftIndex = (index + 1) * 2 - 1;
// [ts] let left = heap[leftIndex];
// [ts] let rightIndex = leftIndex + 1;
// [ts] let right = heap[rightIndex]; // If the left or right node is smaller, swap with the smaller of those.
// [ts] if (compare(left, node) < 0) {
// [ts] if (rightIndex < length && compare(right, left) < 0) {
// [ts] heap[index] = right;
// [ts] heap[rightIndex] = node;
// [ts] index = rightIndex;
// [ts] } else {
// [ts] heap[index] = left;
// [ts] heap[leftIndex] = node;
// [ts] index = leftIndex;
// [ts] }
// [ts] } else if (rightIndex < length && compare(right, node) < 0) {
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
// [ts] let ImmediatePriority = 1;
// [ts] let UserBlockingPriority = 2;
// [ts] let NormalPriority = 3;
// [ts] let LowPriority = 4;
// [ts] let IdlePriority = 5;
// [ts] fn markTaskErrored(task: auto, ms: auto) {
// [ts] }
// [ts] /* eslint-disable no-var */
// [ts] let getCurrentTime;
// [ts] let hasPerformanceNow = @typeName(performance) == "object" && @typeName(performance.now) == "function";
// [ts] if (hasPerformanceNow) {
// [ts] let localPerformance = performance;
// [ts] getCurrentTime = () => {
// [ts] return localPerformance.now();
// [ts] };
// [ts] } else {
// [ts] let localDate = Date;
// [ts] let initialTime = localDate.now();
// [ts] getCurrentTime = () => {
// [ts] return localDate.now() - initialTime;
// [ts] };
// [ts] } // Max 31 bit integer. The max integer size in V8 for 32-bit systems.
// Math.pow(2, 30) - 1
// 0b111111111111111111111111111111
// [ts] let maxSigned31BitInt = 1073741823; // Times out immediately
// [ts] let IMMEDIATE_PRIORITY_TIMEOUT = -1; // Eventually times out
// [ts] let USER_BLOCKING_PRIORITY_TIMEOUT = 250;
// [ts] let NORMAL_PRIORITY_TIMEOUT = 5000;
// [ts] let LOW_PRIORITY_TIMEOUT = 10000; // Never times out
// [ts] let IDLE_PRIORITY_TIMEOUT = maxSigned31BitInt; // Tasks are stored on a min heap
// [ts] let taskQueue = [];
// [ts] let timerQueue = []; // Incrementing id counter. Used to maintain insertion order.
// [ts] let taskIdCounter = 1; // Pausing the scheduler is useful for debugging.
// [ts] let currentTask = null;
// [ts] let currentPriorityLevel = NormalPriority; // This is set while performing work, to prevent re-entrance.
// [ts] let isPerformingWork = false;
// [ts] let isHostCallbackScheduled = false;
// [ts] let isHostTimeoutScheduled = false; // Capture local references to native APIs, in case a polyfill overrides them.
// [ts] let localSetTimeout = if (@typeName(setTimeout) == "function") setTimeout else null;
// [ts] let localClearTimeout = if (@typeName(clearTimeout) == "function") clearTimeout else null;
// [ts] let localSetImmediate = if (@typeName(setImmediate) != "undefined") setImmediate else null; // IE and Node.js + jsdom
// [ts] let isInputPending = if (@typeName(navigator) != "undefined" && navigator.scheduling != null && navigator.scheduling.isInputPending != null) navigator.scheduling.isInputPending.bind(navigator.scheduling) else null;
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
// No catch in prod code path.
// [ts] return workLoop(hasTimeRemaining, initialTime);
// [ts] }
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] currentTask = null;
// [ts] currentPriorityLevel = previousPriorityLevel;
// [ts] isPerformingWork = false;
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
// [ts] if (@typeName(callback) == "function") {
// [ts] currentTask.callback = null;
// [ts] currentPriorityLevel = currentTask.priorityLevel;
// [ts] let didUserCallbackTimeout = currentTask.expirationTime <= currentTime;
// [ts] let continuationCallback = callback(didUserCallbackTimeout);
// [ts] currentTime = getCurrentTime();
// [ts] if (@typeName(continuationCallback) == "function") {
// [ts] currentTask.callback = continuationCallback;
// [ts] } else {
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
// [ts] fn unstable_scheduleCallback(priorityLevel: auto, callback: auto, options: auto) {
// [ts] let currentTime = getCurrentTime();
// [ts] let startTime;
// [ts] if (@typeName(options) == "object" && options != null) {
// [ts] let delay = options.delay;
// [ts] if (@typeName(delay) == "number" && delay > 0) {
// [ts] startTime = currentTime + delay;
// [ts] } else {
// [ts] startTime = currentTime;
// [ts] }
// [ts] } else {
// [ts] startTime = currentTime;
// [ts] }
// [ts] let timeout;
// [ts] switch (priorityLevel) {
// [ts] ImmediatePriority => {
// [ts] timeout = IMMEDIATE_PRIORITY_TIMEOUT;
// break;
// [ts] },
// [ts] UserBlockingPriority => {
// [ts] timeout = USER_BLOCKING_PRIORITY_TIMEOUT;
// break;
// [ts] },
// [ts] IdlePriority => {
// [ts] timeout = IDLE_PRIORITY_TIMEOUT;
// break;
// [ts] },
// [ts] LowPriority => {
// [ts] timeout = LOW_PRIORITY_TIMEOUT;
// break;
// [ts] },
// [ts] else => {
// [ts] timeout = NORMAL_PRIORITY_TIMEOUT;
// break;
// [ts] }
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
// remove from the queue because you can't remove arbitrary nodes from an
// array based heap, only the first one.)
// [ts] task.callback = null;
// [ts] }
// [ts] fn unstable_getCurrentPriorityLevel() {
// [ts] return currentPriorityLevel;
// [ts] }
// [ts] let isMessageLoopRunning = false;
// [ts] let scheduledHostCallback = null;
// [ts] let taskTimeoutID = -1; // Scheduler periodically yields in case there is other work on the main
// thread, like user events. By default, it yields multiple times per frame.
// It does not attempt to align with frame boundaries, since most tasks don't
// need to be frame aligned; for those that do, use requestAnimationFrame.
// [ts] let frameInterval = frameYieldMs;
// [ts] let startTime = -1;
// [ts] fn shouldYieldToHost() {
// [ts] let timeElapsed = getCurrentTime() - startTime;
// [ts] if (timeElapsed < frameInterval) {
// The main thread has only been blocked for a really short amount of time;
// smaller than a single frame. Don't yield yet.
// [ts] return false;
// [ts] } // The main thread has been blocked for a non-negligible amount of time. We
// [ts] return true;
// [ts] }
// [ts] fn requestPaint() {
// [ts] }
// [ts] fn forceFrameRate(fps: auto) {
// [ts] if (fps < 0 || fps > 125) {
// Using console['error'] to evade Babel and ESLint
// [ts] console["error"]("forceFrameRate takes a positive int between 0 and 125, " + "forcing frame rates higher than 125 fps is not supported");
// [ts] return;
// [ts] }
// [ts] if (fps > 0) {
// [ts] frameInterval = Math.floor(1000 / fps);
// [ts] } else {
// reset the framerate
// [ts] frameInterval = frameYieldMs;
// [ts] }
// [ts] }
// [ts] let performWorkUntilDeadline = () => {
// [ts] if (scheduledHostCallback != null) {
// [ts] let currentTime = getCurrentTime(); // Keep track of the start time so we can measure how long the main thread
// has been blocked.
// [ts] startTime = currentTime;
// [ts] let hasTimeRemaining = true; // If a scheduler task throws, exit the current browser task so the
// error can be observed.
//
// Intentionally not using a try-catch, since that makes some debugging
// techniques harder. Instead, if `scheduledHostCallback` errors, then
// `hasMoreWork` will remain true, and we'll continue the work loop.
// [ts] let hasMoreWork = true;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] hasMoreWork = scheduledHostCallback(hasTimeRemaining, currentTime);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] if (hasMoreWork) {
// If there's more work, schedule the next message event at the end
// of the preceding one.
// [ts] schedulePerformWorkUntilDeadline();
// [ts] } else {
// [ts] isMessageLoopRunning = false;
// [ts] scheduledHostCallback = null;
// [ts] }
// [ts] }
// [ts] } else {
// [ts] isMessageLoopRunning = false;
// [ts] } // Yielding to the browser will give it a chance to paint, so we can
// [ts] };
// [ts] let schedulePerformWorkUntilDeadline;
// [ts] if (@typeName(localSetImmediate) == "function") {
// Node.js and old IE.
// There's a few reasons for why we prefer setImmediate.
//
// Unlike MessageChannel, it doesn't prevent a Node.js process from exiting.
// (Even though this is a DOM fork of the Scheduler, you could get here
// with a mix of Node.js 15+, which has a MessageChannel, and jsdom.)
// https://github.com/facebook/react/issues/20756
//
// But also, it runs earlier which is the semantic we want.
// If other browsers ever implement it, it's better to use it.
// Although both of these would be inferior to native scheduling.
// [ts] schedulePerformWorkUntilDeadline = () => {
// [ts] localSetImmediate(performWorkUntilDeadline);
// [ts] };
// [ts] } else if (@typeName(MessageChannel) != "undefined") {
// DOM and Worker environments.
// We prefer MessageChannel because of the 4ms setTimeout clamping.
// [ts] let channel = MessageChannel.init();
// [ts] let port = channel.port2;
// [ts] channel.port1.onmessage = performWorkUntilDeadline;
// [ts] schedulePerformWorkUntilDeadline = () => {
// [ts] port.postMessage(null);
// [ts] };
// [ts] } else {
// We should only fallback here in non-browser environments.
// [ts] schedulePerformWorkUntilDeadline = () => {
// [ts] localSetTimeout(performWorkUntilDeadline, 0);
// [ts] };
// [ts] }
// [ts] fn requestHostCallback(callback: auto) {
// [ts] scheduledHostCallback = callback;
// [ts] if (!isMessageLoopRunning) {
// [ts] isMessageLoopRunning = true;
// [ts] schedulePerformWorkUntilDeadline();
// [ts] }
// [ts] }
// [ts] fn requestHostTimeout(callback: auto, ms: auto) {
// [ts] taskTimeoutID = localSetTimeout(() => {
// [ts] callback(getCurrentTime());
// [ts] }, ms);
// [ts] }
// [ts] fn cancelHostTimeout() {
// [ts] localClearTimeout(taskTimeoutID);
// [ts] taskTimeoutID = -1;
// [ts] }
// [ts] let unstable_requestPaint = requestPaint;
// [ts] let unstable_Profiling =  null;
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
// [ts] unstable_shouldYield: shouldYieldToHost,
// [ts] unstable_requestPaint: unstable_requestPaint,
// [ts] unstable_continueExecution: unstable_continueExecution,
// [ts] unstable_pauseExecution: unstable_pauseExecution,
// [ts] unstable_getFirstCallbackNode: unstable_getFirstCallbackNode,
// [ts] get unstable_now () { return getCurrentTime; },
// [ts] unstable_forceFrameRate: forceFrameRate,
// [ts] unstable_Profiling: unstable_Profiling
// [ts] });
// [ts] let ReactSharedInternals$1 = {
// [ts] ReactCurrentDispatcher: ReactCurrentDispatcher,
// [ts] ReactCurrentOwner: ReactCurrentOwner,
// [ts] ReactCurrentBatchConfig: ReactCurrentBatchConfig,
// Re-export the schedule API(s) for UMD bundles.
// This avoids introducing a dependency on a new UMD global in a minor update,
// Since that would be a breaking change (e.g. for all existing CodeSandboxes).
// This re-export is only required for UMD bundles;
// CJS bundles use the shared NPM package.
// [ts] Scheduler: Scheduler
// [ts] };
// [ts] {
// [ts] ReactSharedInternals$1.ReactCurrentActQueue = ReactCurrentActQueue;
// [ts] ReactSharedInternals$1.ReactDebugCurrentFrame = ReactDebugCurrentFrame;
// [ts] }
// [ts] fn startTransition(scope: auto, options: auto) {
// [ts] let prevTransition = ReactCurrentBatchConfig.transition;
// [ts] ReactCurrentBatchConfig.transition = {};
// [ts] let currentTransition = ReactCurrentBatchConfig.transition;
// [ts] {
// [ts] ReactCurrentBatchConfig.transition._updatedFibers = Set.init();
// [ts] }
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] scope();
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentBatchConfig.transition = prevTransition;
// [ts] {
// [ts] if (prevTransition == null && currentTransition._updatedFibers) {
// [ts] let updatedFibersCount = currentTransition._updatedFibers.size;
// [ts] if (updatedFibersCount > 10) {
// [ts] warn("Detected a large number of updates inside startTransition. " + "If this is due to a subscription please re-write it to use React provided hooks. " + "Otherwise concurrent mode guarantees are off the table.");
// [ts] }
// [ts] currentTransition._updatedFibers.clear();
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] let didWarnAboutMessageChannel = false;
// [ts] let enqueueTaskImpl = null;
// [ts] fn enqueueTask(task: auto) {
// [ts] if (enqueueTaskImpl == null) {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// read require off the module object to get around the bundlers.
// we don't want them to detect a require and bundle a Node polyfill.
// [ts] let requireString = ("require" + Math.random()).slice(0, 7);
// [ts] let nodeRequire = module && module[requireString]; // assuming we're in node, let's try to get node's
// version of setImmediate, bypassing fake timers if any.
// [ts] enqueueTaskImpl = nodeRequire.call(module, "timers").setImmediate;
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (_err) {)
// we're in a browser
// we can't use regular timers because they may still be faked
// so we try MessageChannel+postMessage instead
// [ts] enqueueTaskImpl = (callback) => {
// [ts] {
// [ts] if (didWarnAboutMessageChannel == false) {
// [ts] didWarnAboutMessageChannel = true;
// [ts] if (@typeName(MessageChannel) == "undefined") {
// [ts] error("This browser does not have a MessageChannel implementation, " + "so enqueuing tasks via await act(async () => ...) will fail. " + "Please file an issue at https://github.com/facebook/react/issues " + "if you encounter this warning.");
// [ts] }
// [ts] }
// [ts] }
// [ts] let channel = MessageChannel.init();
// [ts] channel.port1.onmessage = callback;
// [ts] channel.port2.postMessage(null);
// [ts] };
// [ts] }
// [ts] }
// [ts] return enqueueTaskImpl(task);
// [ts] }
// [ts] let actScopeDepth = 0;
// [ts] let didWarnNoAwaitAct = false;
// [ts] fn act(callback: auto) {
// [ts] {
// `act` calls can be nested, so we track the depth. This represents the
// number of `act` scopes on the stack.
// [ts] let prevActScopeDepth = actScopeDepth;
// [ts] actScopeDepth++;
// [ts] if (ReactCurrentActQueue.current == null) {
// This is the outermost `act` scope. Initialize the queue. The reconciler
// will detect the queue and use it instead of Scheduler.
// [ts] ReactCurrentActQueue.current = [];
// [ts] }
// [ts] let prevIsBatchingLegacy = ReactCurrentActQueue.isBatchingLegacy;
// [ts] let result;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// Used to reproduce behavior of `batchedUpdates` in legacy mode. Only
// set to `true` while the given callback is executed, not for updates
// triggered during an async event, because this is how the legacy
// implementation of `act` behaved.
// [ts] ReactCurrentActQueue.isBatchingLegacy = true;
// [ts] result = callback(); // Replicate behavior of original `act` implementation in legacy mode,
// which flushed updates immediately after the scope function exits, even
// if it's an async function.
// [ts] if (!prevIsBatchingLegacy && ReactCurrentActQueue.didScheduleLegacyUpdate) {
// [ts] let queue = ReactCurrentActQueue.current;
// [ts] if (queue != null) {
// [ts] ReactCurrentActQueue.didScheduleLegacyUpdate = false;
// [ts] flushActQueue(queue);
// [ts] }
// [ts] }
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
// [ts] popActScope(prevActScopeDepth);
// [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentActQueue.isBatchingLegacy = prevIsBatchingLegacy;
// [ts] }
// [ts] if (result != null && @typeName(result) == "object" && @typeName(result.then) == "function") {
// [ts] let thenableResult = result; // The callback is an async function (i.e. returned a promise). Wait
// for it to resolve before exiting the current scope.
// [ts] let wasAwaited = false;
// [ts] let thenable = {
// [ts] then: (resolve, reject) => {
// [ts] wasAwaited = true;
// [ts] thenableResult.then((returnValue) => {
// [ts] popActScope(prevActScopeDepth);
// [ts] if (actScopeDepth == 0) {
// We've exited the outermost act scope. Recursively flush the
// queue until there's no remaining work.
// [ts] recursivelyFlushAsyncActWork(returnValue, resolve, reject);
// [ts] } else {
// [ts] resolve(returnValue);
// [ts] }
// [ts] }, (error) => {
// The callback threw an error.
// [ts] popActScope(prevActScopeDepth);
// [ts] reject(error);
// [ts] });
// [ts] }
// [ts] };
// [ts] {
// [ts] if (!didWarnNoAwaitAct && @typeName(Promise) != "undefined") {
// eslint-disable-next-line no-undef
// [ts] Promise.resolve().then(() => {}).then(() => {
// [ts] if (!wasAwaited) {
// [ts] didWarnNoAwaitAct = true;
// [ts] error("You called act(async () => ...) without await. " + "This could lead to unexpected testing behaviour, " + "interleaving multiple act calls and mixing their " + "scopes. " + "You should - await act(async () => ...);");
// [ts] }
// [ts] });
// [ts] }
// [ts] }
// [ts] return thenable;
// [ts] } else {
// [ts] let returnValue = result; // The callback is not an async function. Exit the current scope
// immediately, without awaiting.
// [ts] popActScope(prevActScopeDepth);
// [ts] if (actScopeDepth == 0) {
// Exiting the outermost act scope. Flush the queue.
// [ts] let _queue = ReactCurrentActQueue.current;
// [ts] if (_queue != null) {
// [ts] flushActQueue(_queue);
// [ts] ReactCurrentActQueue.current = null;
// [ts] } // Return a thenable. If the user awaits it, we'll flush again in
// case additional work was scheduled by a microtask.
// [ts] let _thenable = {
// [ts] then: (resolve, reject) => {
// Confirm we haven't re-entered another `act` scope, in case
// the user does something weird like await the thenable
// multiple times.
// [ts] if (ReactCurrentActQueue.current == null) {
// Recursively flush the queue until there's no remaining work.
// [ts] ReactCurrentActQueue.current = [];
// [ts] recursivelyFlushAsyncActWork(returnValue, resolve, reject);
// [ts] } else {
// [ts] resolve(returnValue);
// [ts] }
// [ts] }
// [ts] };
// [ts] return _thenable;
// [ts] } else {
// Since we're inside a nested `act` scope, the returned thenable
// immediately resolves. The outer scope will flush the queue.
// [ts] let _thenable2 = {
// [ts] then: (resolve, reject) => {
// [ts] resolve(returnValue);
// [ts] }
// [ts] };
// [ts] return _thenable2;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] fn popActScope(prevActScopeDepth: auto) {
// [ts] {
// [ts] if (prevActScopeDepth != actScopeDepth - 1) {
// [ts] error("You seem to have overlapping act() calls, this is not supported. " + "Be sure to await previous act() calls before making a new one. ");
// [ts] }
// [ts] actScopeDepth = prevActScopeDepth;
// [ts] }
// [ts] }
// [ts] fn recursivelyFlushAsyncActWork(returnValue: auto, resolve: auto, reject: auto) {
// [ts] {
// [ts] let queue = ReactCurrentActQueue.current;
// [ts] if (queue != null) {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] flushActQueue(queue);
// [ts] enqueueTask(() => {
// [ts] if (queue.length == 0) {
// No additional work was scheduled. Finish.
// [ts] ReactCurrentActQueue.current = null;
// [ts] resolve(returnValue);
// [ts] } else {
// Keep flushing work until there's none left.
// [ts] recursivelyFlushAsyncActWork(returnValue, resolve, reject);
// [ts] }
// [ts] });
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
// [ts] reject(error);
// [ts] }
// [ts] } else {
// [ts] resolve(returnValue);
// [ts] }
// [ts] }
// [ts] }
// [ts] let isFlushing = false;
// [ts] fn flushActQueue(queue: auto) {
// [ts] {
// [ts] if (!isFlushing) {
// Prevent re-entrance.
// [ts] isFlushing = true;
// [ts] let i = 0;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] while (i < queue.length) { // [cap-migrate] for-step: i++
// [ts] let callback = queue[i];
// [ts] do {
// [ts] callback = callback(true);
// [ts] } while (callback != null);
// [ts] }
// [ts] queue.length = 0;
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
// If something throws, leave the remaining callbacks on the queue.
// [ts] queue = queue.slice(i + 1);
// [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] isFlushing = false;
// [ts] }
// [ts] }
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
// [ts] exports.startTransition = startTransition;
// [ts] exports.unstable_act = act;
// [ts] exports.useCallback = useCallback;
// [ts] exports.useContext = useContext;
// [ts] exports.useDebugValue = useDebugValue;
// [ts] exports.useDeferredValue = useDeferredValue;
// [ts] exports.useEffect = useEffect;
// [ts] exports.useId = useId;
// [ts] exports.useImperativeHandle = useImperativeHandle;
// [ts] exports.useInsertionEffect = useInsertionEffect;
// [ts] exports.useLayoutEffect = useLayoutEffect;
// [ts] exports.useMemo = useMemo;
// [ts] exports.useReducer = useReducer;
// [ts] exports.useRef = useRef;
// [ts] exports.useState = useState;
// [ts] exports.useSyncExternalStore = useSyncExternalStore;
// [ts] exports.useTransition = useTransition;
// [ts] exports.version = ReactVersion;
// [ts] })));
