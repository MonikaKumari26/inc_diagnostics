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

/// @file builder.h
/// @brief Declares the Builder interface.
///
/// Obtain a concrete builder from the diagnostic middleware, chain configuration calls,
/// and call Build() once to get a fully-configured DTC.
///
/// Example:
/// @code
///   auto dtc = builder
///       .WithMonitor(MonitorIdentifier{"mon/example"})
///       .WithEvent(EventIdentifier{"evt/example"})
///       .WithClearCondition(ConditionIdentifier{"cond/example"})
///       .ConfigureClearBehaviour(ClearBehaviour::kNotClearable)
///       .ConfigureDebouncing(Debounce::TimeBased{std::chrono::milliseconds{200}, std::chrono::milliseconds{100}})
///       .Build();
/// @endcode

#ifndef SCORE_MW_DIAG_DTC_BUILDER_H
#define SCORE_MW_DIAG_DTC_BUILDER_H

#include "score/mw/diag/dtc/debounce.h"
#include "score/mw/diag/dtc/identifier.h"
#include "score/mw/diag/dtc/dtc.h"

#include <memory>

namespace score::mw::diag::dtc
{

/// @brief Abstract builder interface for constructing fully-configured DTC instances.
/// @note At most one debouncing algorithm may be configured per DTC.
///       A second call to ConfigureDebouncing() overwrites the first.
///       If ConfigureDebouncing() is never called, every Report() call to a DTC will get passed through.
class Builder
{
  public:
    /// @brief Set the monitor identifier that owns this DTC.
    /// @param monitor Identifier of the owning monitor (must be non-empty).
    virtual Builder& WithMonitor(MonitorIdentifier monitor) = 0;

    /// @brief Set the diagnostic event identifier linked to this DTC.
    /// @param event Identifier of the associated diagnostic event (must be non-empty).
    virtual Builder& WithEvent(EventIdentifier event) = 0;

    /// @brief Set the condition under which a tester may clear this DTC.
    /// @param condition Identifier of the clear condition (must be non-empty).
    virtual Builder& WithClearCondition(ConditionIdentifier condition) = 0;

    /// @brief Set the clearing behaviour for this DTC.
    ///        The three options are mutually exclusive — passing a single ClearBehaviour value
    ///        is the only way to configure this, making conflicting combinations a compile error.
    /// @param behaviour One of ClearBehaviour::kClearable (default), kNotClearable,
    ///                  or kReenterAfterCleared.
    virtual Builder& ConfigureClearBehaviour(ClearBehaviour behaviour) = 0;

    /// @brief Configure the debouncing algorithm — TimeBased (time-based) or CounterBased (counter-based).
    ///        Pass a default-constructed Debounce{} to explicitly reset to pass-through.
    /// @param debouncing Debounce instance holding the chosen algorithm, or Debounce{} for pass-through.
    virtual Builder& ConfigureDebouncing(Debounce debouncing) = 0;

    /// @brief Build and return the configured DTC.
    /// @pre WithMonitor(), WithEvent(), and WithClearCondition() must have been called.
    /// @return Owning pointer to the configured DTC instance; never null when all preconditions are met.
    [[nodiscard]] virtual std::unique_ptr<DTC> Build() = 0;

    virtual ~Builder() noexcept = default;
};

}  // namespace score::mw::diag::dtc

#endif  // SCORE_MW_DIAG_DTC_BUILDER_H
