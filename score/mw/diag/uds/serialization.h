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

/// @file serialization.h
/// @brief UDS binary serialization interfaces and typed adapter templates.
///
/// Abstract bases: `Serializable` (serialize to bytes), `WriteHandler<T>` (typed write
/// callback), `RoutineHandler<T>` (typed routine callback with default SubFunctionNotSupported).
///
/// Template adapters: `SerializedReadDataByIdentifier<T>`, `SerializedWriteDataByIdentifier<T,H>`,
/// `SerializedRoutineControl<T,H>` — each wraps the corresponding UDS interface and handles
/// serialization/deserialization internally.
///
/// Deserialization contract: `T` must provide
/// `static Result<T> from_bytes(ByteView data);`
///
/// Free helper: `deserialize_request<T>(ByteView, Callable)`.

#ifndef SCORE_MW_DIAG_UDS_SERIALIZATION_H
#define SCORE_MW_DIAG_UDS_SERIALIZATION_H

#include "score/mw/diag/byte_types.h"
#include "score/mw/diag/diag_result.h"
#include "score/mw/diag/uds/read_data_by_identifier.h"
#include "score/mw/diag/uds/routine_control.h"
#include "score/mw/diag/uds/write_data_by_identifier.h"

#include <optional>
#include <type_traits>

namespace score::mw::diag::uds
{

/************************************/
/* Serializable                     */
/************************************/

/// Abstract base for types that can encode themselves into a raw UDS byte payload.
///
/// Derive from this and implement `serialize()`. The `from_bytes()` static factory
/// is required by the `Serialized*` adapters that use the type as a `DataPayload`.
///
/// @code
/// class VehicleSpeed : public Serializable {
/// public:
///     Result<ByteVector> serialize() const override { /* encode value_ */ }
///     static Result<VehicleSpeed> from_bytes(ByteView data) { /* decode or return NRC */ }
/// private:
///     std::uint16_t value_;
/// };
/// @endcode
class Serializable
{
  public:
    /// Encode this object into a UDS byte payload.
    /// @return Ok(ByteVector) on success, Err(NegativeResponseCode) on serialization failure.
    virtual Result<ByteVector> serialize() const = 0;

    virtual ~Serializable() noexcept = default;

  protected:
    Serializable() = default;
    Serializable(const Serializable&) = default;
    Serializable(Serializable&&) noexcept = default;
    Serializable& operator=(const Serializable&) & = default;
    Serializable& operator=(Serializable&&) & noexcept = default;
};

/************************************/
/* WriteHandler<T>                  */
/************************************/

/// Abstract callback for processing a typed, already-deserialized write value.
///
/// Implement this and pass it to `SerializedWriteDataByIdentifier<DataPayload, HandlerImpl>`.
template <typename DataPayload>
class WriteHandler
{
  public:
    /// Process the deserialized write value.
    /// @param value  Typed value obtained from DataPayload::from_bytes(raw_input).
    /// @return ResultBlank — Ok on success, Err(NegativeResponseCode) on failure.
    virtual ResultBlank handle_write(DataPayload value) = 0;

    virtual ~WriteHandler() noexcept = default;

  protected:
    WriteHandler() = default;
    WriteHandler(const WriteHandler&) = default;
    WriteHandler(WriteHandler&&) noexcept = default;
    WriteHandler& operator=(const WriteHandler&) & = default;
    WriteHandler& operator=(WriteHandler&&) & noexcept = default;
};

/************************************/
/* RoutineHandler<T>                */
/************************************/

/// Abstract callback for typed routine execution.
///
/// Implement this and pass it to `SerializedRoutineControl<DataPayload, HandlerImpl>`.
/// Override only the sub-functions the routine handles; unoverridden methods return
/// `SubFunctionNotSupported` by default.
template <typename DataPayload>
class RoutineHandler
{
  public:
    /// Start the routine with optional typed parameters.
    /// @return Ok(Some(DataPayload)) to include a typed start reply,
    ///         Ok(None)             for no start reply,
    ///         Err with SubFunctionNotSupported if start is not supported (default).
    virtual Result<std::optional<DataPayload>> start(std::optional<DataPayload> /*params*/)
    {
        return Result<std::optional<DataPayload>>{score::unexpect, NegativeResponseCode::SubFunctionNotSupported};
    }

    /// Stop the routine with optional typed parameters.
    /// @return Ok(Some(DataPayload)) for a typed stop reply,
    ///         Ok(None)             for no stop reply,
    ///         Err with SubFunctionNotSupported if stop is not supported (default).
    virtual Result<std::optional<DataPayload>> stop(std::optional<DataPayload> /*params*/)
    {
        return Result<std::optional<DataPayload>>{score::unexpect, NegativeResponseCode::SubFunctionNotSupported};
    }

    /// Retrieve the current or final results of the routine.
    /// @return Ok(Some(DataPayload)) when results are available,
    ///         Ok(None)             while the routine is still running,
    ///         Err with SubFunctionNotSupported if results polling is not supported (default).
    virtual Result<std::optional<DataPayload>> results() const
    {
        return Result<std::optional<DataPayload>>{score::unexpect, NegativeResponseCode::SubFunctionNotSupported};
    }

    /// Current completion percentage [0, 100], or nullopt if unavailable.
    virtual std::optional<std::uint8_t> completion_percentage() const noexcept
    {
        return std::nullopt;
    }

    virtual ~RoutineHandler() noexcept = default;

  protected:
    RoutineHandler() = default;
    RoutineHandler(const RoutineHandler&) = default;
    RoutineHandler(RoutineHandler&&) noexcept = default;
    RoutineHandler& operator=(const RoutineHandler&) & = default;
    RoutineHandler& operator=(RoutineHandler&&) & noexcept = default;
};

/************************************/
/* SerializedReadDataByIdentifier<T>*/
/************************************/

/// Adapts a `Serializable` `DataPayload` to the `ReadDataByIdentifier` interface.
/// Owns the value and returns `DataPayload::serialize()` on each `Read()` call.
/// `DataPayload` must derive from `Serializable`.
template <typename DataPayload>
class SerializedReadDataByIdentifier final : public ReadDataByIdentifier
{
    static_assert(std::is_base_of_v<Serializable, DataPayload>,
                  "DataPayload must derive from score::mw::diag::uds::Serializable");

  public:
    explicit SerializedReadDataByIdentifier(DataPayload value) noexcept(
        std::is_nothrow_move_constructible_v<DataPayload>)
        : serializable_value_{std::move(value)}
    {
    }

    [[nodiscard]] ResultWithData Read() override
    {
        return serializable_value_.serialize();
    }

    ~SerializedReadDataByIdentifier() noexcept override = default;

  private:
    DataPayload serializable_value_;
};

/*************************************/
/* SerializedWriteDataByIdentifier   */
/* <DataPayload, HandlerImpl>        */
/*************************************/

/// Adapts a `WriteHandler<DataPayload>` to the `WriteDataByIdentifier` interface.
///
/// On `Write(input)`: calls `DataPayload::from_bytes(input)`, then on success
/// calls `HandlerImpl::handle_write(typed_value)`.
///
/// Requires: `DataPayload::from_bytes(ByteView)` static factory;
///           `HandlerImpl` must derive from `WriteHandler<DataPayload>`.
template <typename DataPayload, typename HandlerImpl>
class SerializedWriteDataByIdentifier final : public WriteDataByIdentifier
{
    static_assert(std::is_base_of_v<WriteHandler<DataPayload>, HandlerImpl>,
                  "HandlerImpl must derive from score::mw::diag::uds::WriteHandler<DataPayload>");

  public:
    explicit SerializedWriteDataByIdentifier(HandlerImpl handler) noexcept(
        std::is_nothrow_move_constructible_v<HandlerImpl>)
        : handler_{std::move(handler)}
    {
    }

    [[nodiscard]] ResultBlank Write(ByteView input) override
    {
        auto parse_result = DataPayload::from_bytes(input);
        if (!parse_result.has_value())
        {
            return ResultBlank{score::unexpect, parse_result.error()};
        }
        return handler_.handle_write(std::move(*parse_result));
    }

    ~SerializedWriteDataByIdentifier() noexcept override = default;

  private:
    HandlerImpl handler_;
};

/*************************************/
/* SerializedRoutineControl          */
/* <DataPayload, HandlerImpl>        */
/*************************************/

/// Adapts a `RoutineHandler<DataPayload>` to the `RoutineControl` interface.
///
/// `Start(input)`: deserializes optional bytes → calls `HandlerImpl::start()` → serializes
/// typed reply into `StartRoutine::reply`; sets `result_provider` to poll `HandlerImpl::results()`.
///
/// `Stop(input)`: deserializes optional bytes → calls `HandlerImpl::stop()` → serializes reply.
///
/// Requires: `DataPayload` derives from `Serializable` and provides `from_bytes(ByteView)`;
///           `HandlerImpl` derives from `RoutineHandler<DataPayload>`.
///
/// @note `result_provider` in the returned `StartRoutine` captures `this`. The owning
///       `DiagnosticServicesCollection` MUST outlive the invocation of `result_provider`.
template <typename DataPayload, typename HandlerImpl>
class SerializedRoutineControl final : public RoutineControl
{
    static_assert(std::is_base_of_v<Serializable, DataPayload>,
                  "DataPayload must derive from score::mw::diag::uds::Serializable");
    static_assert(std::is_base_of_v<RoutineHandler<DataPayload>, HandlerImpl>,
                  "HandlerImpl must derive from score::mw::diag::uds::RoutineHandler<DataPayload>");

  public:
    explicit SerializedRoutineControl(HandlerImpl handler) noexcept(
        std::is_nothrow_move_constructible_v<HandlerImpl>)
        : handler_{std::move(handler)}
    {
    }

    [[nodiscard]] StartResult Start(std::optional<ByteView> input) override
    {
        std::optional<DataPayload> deserialized_params;
        if (input.has_value())
        {
            auto parse_result = DataPayload::from_bytes(*input);
            if (!parse_result.has_value())
            {
                return StartResult{score::unexpect, parse_result.error()};
            }
            deserialized_params = std::move(*parse_result);
        }

        auto start_result = handler_.start(std::move(deserialized_params));
        if (!start_result.has_value())
        {
            return StartResult{score::unexpect, start_result.error()};
        }

        StartRoutine start_routine{};

        // Serialize the optional typed start reply into raw bytes.
        if (start_result->has_value())
        {
            auto serialized_reply = (*start_result)->serialize();
            if (!serialized_reply.has_value())
            {
                return StartResult{score::unexpect, serialized_reply.error()};
            }
            start_routine.reply = std::move(*serialized_reply);
        }

        // result_provider polls HandlerImpl::results(). Precondition: *this must outlive the callable.
        start_routine.result_provider = [this]() -> StopResult {
            auto routine_result = handler_.results();
            if (!routine_result.has_value())
            {
                return StopResult{score::unexpect, routine_result.error()};
            }
            if (!routine_result->has_value())
            {
                return std::optional<ByteVector>{std::nullopt};
            }
            auto serialized_result = (*routine_result)->serialize();
            if (!serialized_result.has_value())
            {
                return StopResult{score::unexpect, serialized_result.error()};
            }
            return std::optional<ByteVector>{std::move(*serialized_result)};
        };

        return start_routine;
    }

    [[nodiscard]] StopResult Stop(std::optional<ByteView> input) override
    {
        std::optional<DataPayload> deserialized_params;
        if (input.has_value())
        {
            auto parse_result = DataPayload::from_bytes(*input);
            if (!parse_result.has_value())
            {
                return StopResult{score::unexpect, parse_result.error()};
            }
            deserialized_params = std::move(*parse_result);
        }

        auto stop_result = handler_.stop(std::move(deserialized_params));
        if (!stop_result.has_value())
        {
            return StopResult{score::unexpect, stop_result.error()};
        }
        if (!stop_result->has_value())
        {
            return std::optional<ByteVector>{std::nullopt};
        }

        auto serialized_reply = (*stop_result)->serialize();
        if (!serialized_reply.has_value())
        {
            return StopResult{score::unexpect, serialized_reply.error()};
        }
        return std::optional<ByteVector>{std::move(*serialized_reply)};
    }

    [[nodiscard]] std::optional<std::uint8_t> CompletionPercentage() const noexcept override
    {
        return handler_.completion_percentage();
    }

    ~SerializedRoutineControl() noexcept override = default;

  private:
    HandlerImpl handler_;
};

/************************************/
/* Free-function helper             */
/************************************/

/// Deserialize `data` into `RequestPayload` via `RequestPayload::from_bytes()`, then
/// invoke `handler(typed_value)`. Any parse failure is normalized to
/// `IncorrectMessageLengthOrInvalidFormat` before being returned.
/// `Callable` must accept `RequestPayload` and return `ResultBlank`.
template <typename RequestPayload, typename Callable>
ResultBlank deserialize_request(ByteView data, Callable&& handler)
{
    auto parse_result = RequestPayload::from_bytes(data);
    if (!parse_result.has_value())
    {
        // Normalize any parse error to the correct UDS wire-level NRC.
        return ResultBlank{score::unexpect, NegativeResponseCode::IncorrectMessageLengthOrInvalidFormat};
    }
    return std::forward<Callable>(handler)(std::move(*parse_result));
}

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_UDS_SERIALIZATION_H
