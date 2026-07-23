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

/// @file serialization_base.h
/// @brief Abstract base interfaces for the UDS serialization layer.
///
/// Defines the contracts that application code must implement:
///   - `Serializable`      — encode a typed value into raw UDS bytes.
///   - `WriteHandler<T>`   — receive a deserialized write value.
///   - `RoutineHandler<T>` — handle routine Start/Stop with typed parameters.
///
/// Also exposes `detail::HasFromBytesFactory<T>` — a compile-time trait used by
/// the `Serialized*` adapter templates to enforce the `FromBytes()` contract.
///
/// Implementors of the above interfaces should include **this** header only.
/// Users instantiating the `Serialized*` adapters should include `serialization.h`,
/// which includes this header automatically.

#ifndef SCORE_MW_DIAG_UDS_SERIALIZATION_BASE_H
#define SCORE_MW_DIAG_UDS_SERIALIZATION_BASE_H

#include "score/mw/diag/byte_types.h"
#include "score/mw/diag/diag_result.h"
#include "score/mw/diag/uds/negative_response_code.h"

#include <cstdint>
#include <optional>
#include <type_traits>

namespace score::mw::diag::uds
{

/************************************/
/* Compile-time helpers             */
/************************************/

namespace detail
{
/// Detects at compile time whether T provides a static `FromBytes(ByteView)` factory
/// returning `Result<T>`.
/// Used in static_assert to give a clear error when a DataPayload type is missing
/// the required deserialization entry point, instead of a deep template instantiation error.
template<typename T, typename = void> struct HasFromBytesFactory : std::false_type
{
};

template<typename T>
struct HasFromBytesFactory<T, std::void_t<decltype(T::FromBytes(std::declval<ByteView>()))>>
    : std::bool_constant<std::is_same_v<decltype(T::FromBytes(std::declval<ByteView>())), Result<T>>> {};
}  // namespace detail

/************************************/
/* Serializable                     */
/************************************/

/// Abstract base for types that can encode themselves into a raw UDS byte payload.
///
/// Derive from this and implement `Serialize()`. The `FromBytes()` static factory
/// is required by the `Serialized*` adapters that use the type as a `DataPayload`.
class Serializable
{
  public:
    /// Encode this object into a UDS byte payload.
    /// @return Ok(ByteVector) on success, Err(NegativeResponseCode) on serialization failure.
    [[nodiscard]] virtual Result<ByteVector> Serialize() const = 0;

    virtual ~Serializable() noexcept = default;
};

/************************************/
/* WriteHandler<T>                  */
/************************************/

/// Abstract callback for processing a typed, already-deserialized write value.
///
/// Implement this and pass it to `SerializedWriteDataByIdentifier<DataPayload, HandlerImpl>`
/// or `SerializedGenericDataIdentifier<DataPayload, HandlerImpl>`.
template<typename DataPayload> class WriteHandler
{
  public:
    /// Process the deserialized write value.
    /// @param value  Typed value obtained from DataPayload::FromBytes(raw_input).
    /// @return Result<score::cpp::blank> — Ok on success, Err(NegativeResponseCode) on failure.
    [[nodiscard]] virtual Result<score::cpp::blank> HandleWrite(DataPayload value) = 0;

    virtual ~WriteHandler() noexcept = default;
};

/************************************/
/* RoutineHandler<T>                */
/************************************/

/// Abstract callback for typed routine execution.
///
/// Implement this and pass it to `SerializedRoutineControl<DataPayload, HandlerImpl>`.
/// `Start()` is mandatory — every routine must handle it. `Stop()` and
/// `CompletionPercentage()` have default implementations and may be left unoverridden.
template<typename DataPayload> class RoutineHandler
{
  public:
    /// Start the routine with optional typed parameters.
    /// @param input  Deserialized input parameters (absent if the tester sent no payload).
    ///               Passed by value — implementations may move from it.
    [[nodiscard]] virtual Result<std::optional<DataPayload>> Start(std::optional<DataPayload> input) = 0;

    /// Stop the routine with optional typed parameters.
    /// @param input  Deserialized stop parameters (absent if the tester sent no payload).
    ///               Passed by value — implementations may move from it.
    [[nodiscard]] virtual Result<std::optional<DataPayload>> Stop(std::optional<DataPayload> /*input*/)
    {
        return Result<std::optional<DataPayload>>(
            score::cpp::make_unexpected(NegativeResponseCode::SubFunctionNotSupported));
    }

    /// Request the current routine results with optional typed parameters.
    /// @param input  Deserialized request parameters (absent if the tester sent no payload).
    ///               Passed by value — implementations may move from it.
    [[nodiscard]] virtual Result<std::optional<DataPayload>> RequestResults(std::optional<DataPayload> /*input*/)
    {
        return Result<std::optional<DataPayload>>(
            score::cpp::make_unexpected(NegativeResponseCode::SubFunctionNotSupported));
    }

    /// Current completion percentage [0, 100], or nullopt if unavailable.
    [[nodiscard]] virtual std::optional<std::uint8_t> CompletionPercentage() const noexcept
    {
        return std::nullopt;
    }

    virtual ~RoutineHandler() noexcept = default;
};

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_UDS_SERIALIZATION_BASE_H
