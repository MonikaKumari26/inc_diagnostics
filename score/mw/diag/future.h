/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/

/// @file future.h
/// @brief Async promise/future type aliases for diagnostic service operations.

#ifndef SCORE_MW_DIAG_FUTURE_H
#define SCORE_MW_DIAG_FUTURE_H

#include "score/concurrency/future/interruptible_future.h"
#include "score/concurrency/future/interruptible_promise.h"

namespace score::mw::diag
{

/// Public type alias for async diagnostic promises.
template <typename Value>
using Promise = score::concurrency::InterruptiblePromise<Value>;

/// Public type alias for async diagnostic futures.
template <typename Value>
using Future = score::concurrency::InterruptibleFuture<Value>;

/// Continuation callback type for use with mw::diag::Future::Then().
template <typename Value>
using FutureContinuation = typename score::concurrency::InterruptibleState<Value>::ScopedContinuationCallback;

}  // namespace score::mw::diag

#endif  // SCORE_MW_DIAG_FUTURE_H
