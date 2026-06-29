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

/// @file identifier.h
/// @brief Strong identifier types for Builder parameters.
///
/// Replaces raw std::string parameters to:
///   - Prevent argument-order mistakes at compile time: MonitorIdentifier cannot
///     be passed where EventIdentifier is expected and vice-versa.
///   - Centralise format validation in a single place (Validator::IsValid).
///
/// Example:
/// @code
///   builder
///       .WithMonitor(MonitorIdentifier{"mon/voltage_sensor"})
///       .WithEvent(EventIdentifier{"evt/voltage_out_of_range"})
///       .WithClearCondition(ConditionIdentifier{"cond/ignition_on"});
/// @endcode

#ifndef SCORE_MW_DIAG_DTC_IDENTIFIER_H
#define SCORE_MW_DIAG_DTC_IDENTIFIER_H

#include "score/assert.hpp"
#include <string>
#include <string_view>

namespace score::mw::diag::dtc
{

/************************************/
/* Validators                       */
/************************************/

/// @brief Shared validator for all DTC identifier types.
///        Tighten IsValid() here once the authoritative format specification is available.
struct NonEmptyValidator
{
    [[nodiscard]] static constexpr bool IsValid(std::string_view id) noexcept
    {
        return !id.empty();
    }
};

/************************************/
/* detail — internal impl           */
/************************************/

namespace detail
{

/// @brief Type-safe DTC string identifier.
///
/// @tparam Tag       Phantom type that makes MonitorIdentifier, EventIdentifier and
///                   ConditionIdentifier distinct, incompatible types at compile time.
/// @tparam Validator Policy struct providing a constexpr IsValid(std::string_view) predicate.
///                   String literals are validated at compile time (static_assert).
///                   Runtime strings are validated via SCORE_LANGUAGE_FUTURECPP_PRECONDITION.
template<typename Tag, typename Validator> class Identifier
{
  public:
    /// @brief Construct from a string literal — validated at compile time.
    /// @tparam N Length of the string literal including null terminator.
    /// @param  literal Non-empty string literal.
    /// @note   In C++17, function parameters are never constexpr, so Validator::IsValid()
    ///         cannot be called inside static_assert. N > 1U is the compile-time-provable
    ///         equivalent of NonEmptyValidator::IsValid for literals. If the validator is
    ///         tightened, update this check accordingly or migrate to C++20 consteval.
    template<std::size_t N> explicit Identifier(const char (&literal)[N]) : value_{literal, N - 1U}
    {
        static_assert(N > 1U, "Identifier: string literal must not be empty");
    }

    /// @brief Construct from a std::string_view or std::string — validated at runtime.
    ///        std::string implicitly converts to std::string_view, so this single
    ///        overload accepts all non-literal string-like types without ambiguity.
    /// @param id The identifier value; must satisfy Validator::IsValid().
    /// @pre Validator::IsValid(id) == true.
    explicit Identifier(std::string_view id) : value_{id}
    {
        SCORE_LANGUAGE_FUTURECPP_PRECONDITION(Validator::IsValid(id));
    }

    /// @brief Return the underlying string value.
    /// @return The identifier string passed at construction.
    [[nodiscard]] std::string_view GetValue() const noexcept { return value_; }

    /// @brief Returns true if both identifiers hold the same string value.
    [[nodiscard]] bool operator==(const Identifier& other) const noexcept
    {
        return value_ == other.value_;
    }

    /// @brief Returns true if the identifiers hold different string values.
    [[nodiscard]] bool operator!=(const Identifier& other) const noexcept
    {
        return !(*this == other);
    }

    /// @brief Lexicographic ordering — enables use in std::map and std::set.
    [[nodiscard]] bool operator<(const Identifier& other) const noexcept
    {
        return value_ < other.value_;
    }

  private:
    std::string value_;
};

struct Monitor;    ///< Phantom tag for MonitorIdentifier.
struct Event;      ///< Phantom tag for EventIdentifier.
struct Condition;  ///< Phantom tag for ConditionIdentifier.

}  // namespace detail

/************************************/
/* Public type aliases              */
/************************************/

/// @brief Strong type for a DTC monitor identifier (e.g. "mon/voltage_sensor").
using MonitorIdentifier = detail::Identifier<detail::Monitor, NonEmptyValidator>;

/// @brief Strong type for a DTC event identifier (e.g. "evt/voltage_out_of_range").
using EventIdentifier = detail::Identifier<detail::Event, NonEmptyValidator>;

/// @brief Strong type for a DTC clear condition identifier (e.g. "cond/ignition_on").
using ConditionIdentifier = detail::Identifier<detail::Condition, NonEmptyValidator>;

}  // namespace score::mw::diag::dtc

#endif  // SCORE_MW_DIAG_DTC_IDENTIFIER_H
