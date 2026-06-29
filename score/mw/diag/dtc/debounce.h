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

/// @file debounce.h
/// @brief Debouncing configuration types for DTC monitoring.

#ifndef SCORE_MW_DIAG_DTC_DEBOUNCE_H
#define SCORE_MW_DIAG_DTC_DEBOUNCE_H

#include "score/assert.hpp"
#include <cstdint>
#include <variant>

namespace score::mw::diag::dtc
{

/// @brief Debouncing configuration for a DTC instance.
///
/// Holds at most one algorithm — time-based (Timer) or counter-based (Counter).
/// Default-constructed means no middleware debouncing: every Report() call is passed through.
///
/// @par Usage
/// @code
///   Debounce d{Debounce::Timer{200U, 100U}};
///   SCORE_LANGUAGE_FUTURECPP_PRECONDITION(d.IsSet());
///   auto& algo = d.GetAlgorithm(); // std::variant<std::monostate, Timer, Counter>
/// @endcode
class Debounce
{
  public:
    /************************************/
    /* Timer debouncing                 */
    /************************************/

    /// @brief Time-based debouncing: fault must persist for @p failed_ms ms to qualify as Failed,
    ///        or healthy for @p passed_ms ms to qualify as Passed.
    ///        Both values are application-specific and must be set explicitly.
    struct Timer
    {
        std::uint32_t failed_ms;  ///< Continuous fail duration (ms) to qualify as Failed.
        std::uint32_t passed_ms;  ///< Continuous pass duration (ms) to qualify as Passed.

        /// @brief Returns true if both durations are greater than zero.
        /// @return true if failed_ms > 0 and passed_ms > 0; false otherwise.
        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return (failed_ms > 0U) && (passed_ms > 0U);
        }
    };

    /************************************/
    /* Counter debouncing               */
    /************************************/

    /// @brief Counter-based debouncing parameters.
    ///        All thresholds, step sizes and jump values are application-specific and must be set explicitly.
    struct Counter
    {
        std::int16_t
            failed_threshold;  ///< Counter threshold to qualify as Failed (must be positive).
        std::int16_t
            passed_threshold;  ///< Counter threshold to qualify as Passed (must be negative).
        std::uint16_t failed_stepsize;   ///< Counter increment per Failed report.
        std::uint16_t passed_stepsize;   ///< Counter decrement per Passed report.
        std::int16_t failed_jump_value;  ///< Jump value on first Failed (if use_jump_to_failed).
        std::int16_t passed_jump_value;  ///< Jump value on first Passed (if use_jump_to_passed).
        bool use_jump_to_failed;         ///< Apply jump-to-failed on the first Failed report.
        bool use_jump_to_passed;         ///< Apply jump-to-passed on the first Passed report.

        /// @brief Returns true if thresholds and step sizes are semantically valid.
        /// @return true if failed_threshold > 0, passed_threshold < 0, and both step sizes > 0.
        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return (failed_threshold > 0) && (passed_threshold < 0) && (failed_stepsize > 0U) &&
                   (passed_stepsize > 0U);
        }
    };

    /************************************/
    /* Algorithm variant                */
    /************************************/

    /// @brief Selects the debouncing algorithm — pass-through (none), time-based, or counter-based.
    ///        std::monostate = pass-through: the middleware applies no algorithm and forwards
    ///        every Report() call as-is.
    using Algorithm = std::variant<std::monostate, Timer, Counter>;

    /************************************/
    /* Construction                     */
    /************************************/

    /// @brief Default: no debouncing — every Report() call is passed through; IsSet() returns false.
    Debounce() = default;

    /// @brief Configure time-based debouncing.
    /// @param timer Time-based algorithm parameters; must satisfy Timer::IsValid().
    // NOLINTNEXTLINE(google-explicit-constructor) — intentional: Timer IS a complete Debounce config
    Debounce(Timer timer) noexcept : algorithm_{timer} {}  // NOLINT(hicpp-explicit-conversions)

    /// @brief Configure counter-based debouncing.
    /// @param counter Counter-based algorithm parameters; must satisfy Counter::IsValid().
    // NOLINTNEXTLINE(google-explicit-constructor) — intentional: Counter IS a complete Debounce config
    Debounce(Counter counter) noexcept
        : algorithm_{counter} {}  // NOLINT(hicpp-explicit-conversions)

    /************************************/
    /* Queries                          */
    /************************************/

    /// @brief Returns true if a debouncing algorithm (Timer or Counter) has been configured.
    /// @return true if a Timer or Counter is active; false if no debouncing (pass-through).
    [[nodiscard]] constexpr bool IsSet() const noexcept
    {
        return !std::holds_alternative<std::monostate>(algorithm_);
    }

    /// @brief Returns the configured algorithm variant.
    ///        Inspect with std::holds_alternative<Timer> or std::get<Timer> / std::get<Counter>.
    /// @return const reference to the Algorithm variant; holds std::monostate when IsSet() == false.
    [[nodiscard]] const Algorithm& GetAlgorithm() const noexcept { return algorithm_; }

  private:
    Algorithm algorithm_{};
};

}  // namespace score::mw::diag::dtc

#endif  // SCORE_MW_DIAG_DTC_DEBOUNCE_H
