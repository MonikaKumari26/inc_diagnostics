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
/// @brief UDS typed serialization adapter templates.
///
/// Includes `serialization_base.h` (abstract bases `Serializable`, `WriteHandler<T>`,
/// `RoutineHandler<T>`) and adds the concrete adapter templates:
///   - `SerializedReadDataByIdentifier<T>`       — read-only DID (Service 0x22)
///   - `SerializedWriteDataByIdentifier<T,H>`    — write-only DID (Service 0x2E)
///   - `SerializedGenericDataIdentifier<T,H>`    — combined read+write DID (0x22 + 0x2E)
///   - `SerializedRoutineControl<T,H>`           — RoutineControl adapter (Service 0x31)
///
/// Free helper: `DeserializeRequest<T>(ByteView, Callable)`.
///
/// Implementors of handler interfaces only need `serialization_base.h`.

#ifndef SCORE_MW_DIAG_UDS_SERIALIZATION_H
#define SCORE_MW_DIAG_UDS_SERIALIZATION_H

#include "score/mw/diag/uds/serialization_base.h"

#include "score/mw/diag/uds/generic_data_identifier.h"
#include "score/mw/diag/uds/read_data_by_identifier.h"
#include "score/mw/diag/uds/routine_control.h"
#include "score/mw/diag/uds/write_data_by_identifier.h"

#include <optional>
#include <type_traits>

namespace score::mw::diag::uds
{

/************************************/
/* SerializedReadDataByIdentifier<T>*/
/************************************/

/// Adapts a `Serializable` `DataPayload` to the `ReadDataByIdentifier` interface.
/// Owns the value and returns `DataPayload::Serialize()` on each `Read()` call.
/// `DataPayload` must derive from `Serializable`.
///
/// @note The adapter stores a **snapshot** of the value provided at construction time.
///       Every `Read()` call serializes the same stored instance. If the diagnostic
///       data must reflect live/changing state, implement `ReadDataByIdentifier` directly
///       and call your data source inside `Read()` instead of using this adapter.
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

    [[nodiscard]] Result<ByteVector> Read() override { return serializable_value_.Serialize(); }

    ~SerializedReadDataByIdentifier() noexcept override = default;

    SerializedReadDataByIdentifier(const SerializedReadDataByIdentifier&) = delete;
    SerializedReadDataByIdentifier(SerializedReadDataByIdentifier&&) noexcept = delete;
    SerializedReadDataByIdentifier& operator=(const SerializedReadDataByIdentifier&) = delete;
    SerializedReadDataByIdentifier& operator=(SerializedReadDataByIdentifier&&) noexcept = delete;

  private:
    DataPayload serializable_value_;
};

/*************************************/
/* SerializedWriteDataByIdentifier   */
/* <DataPayload, HandlerImpl>        */
/*************************************/

/// Adapts a `WriteHandler<DataPayload>` to the `WriteDataByIdentifier` interface.
///
/// On `Write(input)`: calls `DataPayload::FromBytes(input)`, then on success
/// calls `HandlerImpl::HandleWrite(typedValue)`.
///
/// @note `DataPayload` does **not** need to derive from `Serializable` — write-only DIDs
///       only require `FromBytes()` for deserialization; `Serialize()` is never called.
///
/// Requires: `DataPayload::FromBytes(ByteView)` → `Result<DataPayload>` static factory;
///           `HandlerImpl` must derive from `WriteHandler<DataPayload>`.
template <typename DataPayload, typename HandlerImpl>
class SerializedWriteDataByIdentifier final : public WriteDataByIdentifier
{
    static_assert(std::is_base_of_v<WriteHandler<DataPayload>, HandlerImpl>,
                  "HandlerImpl must derive from score::mw::diag::uds::WriteHandler<DataPayload>");
    static_assert(detail::HasFromBytesFactory<DataPayload>::value,
                  "DataPayload must provide a static FromBytes(ByteView) factory");

  public:
    explicit SerializedWriteDataByIdentifier(HandlerImpl write_handler) noexcept(
        std::is_nothrow_move_constructible_v<HandlerImpl>)
        : handler_{std::move(write_handler)}
    {
    }

    [[nodiscard]] Result<score::cpp::blank> Write(ByteView input) override
    {
        auto parsed_payload = DataPayload::FromBytes(input);

        if (!parsed_payload.has_value())
        {
            return Result<score::cpp::blank>(score::cpp::make_unexpected(NegativeResponseCode::IncorrectMessageLengthOrInvalidFormat));
        }

        return handler_.HandleWrite(std::move(*parsed_payload));
    }

    ~SerializedWriteDataByIdentifier() noexcept override = default;

    SerializedWriteDataByIdentifier(const SerializedWriteDataByIdentifier&) = delete;
    SerializedWriteDataByIdentifier(SerializedWriteDataByIdentifier&&) noexcept = delete;
    SerializedWriteDataByIdentifier& operator=(const SerializedWriteDataByIdentifier&) = delete;
    SerializedWriteDataByIdentifier& operator=(SerializedWriteDataByIdentifier&&) noexcept = delete;

  private:
    HandlerImpl handler_;
};

/*************************************/
/* SerializedGenericDataIdentifier   */
/* <DataPayload, HandlerImpl>        */
/*************************************/

/// Adapts a `Serializable` `DataPayload` and a `WriteHandler<DataPayload>` to the
/// `GenericDataIdentifier` interface, covering both UDS Service 0x22 (read) and
/// Service 0x2E (write) through a single typed adapter.
///
/// On `Read()`:        returns `DataPayload::Serialize()` of the owned value.
/// On `Write(input)`:  calls `DataPayload::FromBytes(input)`, then on success
///                     calls `WriteHandlerImpl::HandleWrite(typedValue)`.
///
/// @note Holds a **snapshot** (see `SerializedReadDataByIdentifier`). `Write()` does **not**
///       update the stored readable value — written data is delivered to `HandleWrite()` only.
///       To have `Read()` reflect written data, implement `GenericDataIdentifier` directly.
///
/// Requires: `DataPayload` derives from `Serializable` and provides `FromBytes(ByteView)`;
///           `WriteHandlerImpl` derives from `WriteHandler<DataPayload>`.
template <typename DataPayload, typename WriteHandlerImpl>
class SerializedGenericDataIdentifier final : public GenericDataIdentifier
{
    static_assert(std::is_base_of_v<Serializable, DataPayload>,
                  "DataPayload must derive from score::mw::diag::uds::Serializable");
    static_assert(detail::HasFromBytesFactory<DataPayload>::value,
                  "DataPayload must provide a static FromBytes(ByteView) factory");
    static_assert(std::is_base_of_v<WriteHandler<DataPayload>, WriteHandlerImpl>,
                  "WriteHandlerImpl must derive from score::mw::diag::uds::WriteHandler<DataPayload>");

  public:
    /// @param value         Initial readable value; returned by Read() after serialization.
    /// @param write_handler Handler invoked with the deserialized value on each Write() call.
    explicit SerializedGenericDataIdentifier(
        DataPayload read_value,
        WriteHandlerImpl write_handler) noexcept(std::is_nothrow_move_constructible_v<DataPayload> &&
                                                 std::is_nothrow_move_constructible_v<WriteHandlerImpl>)
        : serializable_value_{std::move(read_value)}, handler_{std::move(write_handler)}
    {
    }

    [[nodiscard]] Result<ByteVector> Read() override { return serializable_value_.Serialize(); }

    [[nodiscard]] Result<score::cpp::blank> Write(ByteView input) override
    {
        auto parsed_payload = DataPayload::FromBytes(input);

        if (!parsed_payload.has_value())
        {
            return Result<score::cpp::blank>(
                score::cpp::make_unexpected(NegativeResponseCode::IncorrectMessageLengthOrInvalidFormat));
        }

        return handler_.HandleWrite(std::move(*parsed_payload));
    }

    ~SerializedGenericDataIdentifier() noexcept override = default;

    SerializedGenericDataIdentifier(const SerializedGenericDataIdentifier&) = delete;
    SerializedGenericDataIdentifier(SerializedGenericDataIdentifier&&) noexcept = delete;
    SerializedGenericDataIdentifier& operator=(const SerializedGenericDataIdentifier&) = delete;
    SerializedGenericDataIdentifier& operator=(SerializedGenericDataIdentifier&&) noexcept = delete;

  private:
    DataPayload serializable_value_;
    WriteHandlerImpl handler_;
};

/*************************************/
/* SerializedRoutineControl          */
/* <DataPayload, HandlerImpl>        */
/*************************************/

/// Adapts a `RoutineHandler<DataPayload>` to the `RoutineControl` interface.
///
/// `Start(input)`: empty input → passes `nullopt` to the handler; non-empty input →
///   deserializes via `DataPayload::FromBytes()`. Calls `HandlerImpl::Start()`, then
///   serializes the typed reply into a `ByteVector` (empty `ByteVector` if no reply).
///
/// `Stop(input)`: same deserialization pattern → calls `HandlerImpl::Stop()` → serializes reply.
///
/// Requires: `DataPayload` derives from `Serializable` and provides `FromBytes(ByteView)`;
///           `HandlerImpl` derives from `RoutineHandler<DataPayload>`.
template <typename DataPayload, typename HandlerImpl>
class SerializedRoutineControl final : public RoutineControl
{
    static_assert(std::is_base_of_v<Serializable, DataPayload>,
                  "DataPayload must derive from score::mw::diag::uds::Serializable");
    static_assert(detail::HasFromBytesFactory<DataPayload>::value,
                  "DataPayload must provide a static FromBytes(ByteView) factory");
    static_assert(std::is_base_of_v<RoutineHandler<DataPayload>, HandlerImpl>,
                  "HandlerImpl must derive from score::mw::diag::uds::RoutineHandler<DataPayload>");

  public:
    explicit SerializedRoutineControl(HandlerImpl routine_handler) noexcept(
        std::is_nothrow_move_constructible_v<HandlerImpl>)
        : handler_{std::move(routine_handler)}
    {
    }

    [[nodiscard]] Result<ByteVector> Start(ByteView input) override
    {
        auto deserialized_params = DeserializeOptionalInput(input);
        if (!deserialized_params.has_value())
        {
            return Result<ByteVector>(score::cpp::make_unexpected(deserialized_params.error()));
        }

        auto start_outcome = handler_.Start(std::move(*deserialized_params));

        if (!start_outcome.has_value())
        {
            return Result<ByteVector>(score::cpp::make_unexpected(start_outcome.error()));
        }

        if (!start_outcome->has_value())
        {
            return ByteVector{};
        }

        auto serialized_reply = start_outcome->value().Serialize();

        if (!serialized_reply.has_value())
        {
            return Result<ByteVector>(score::cpp::make_unexpected(NegativeResponseCode::FailurePreventsExecutionOfRequestedAction));
        }

        return std::move(*serialized_reply);
    }

    [[nodiscard]] Result<ByteVector> Stop(ByteView input) override
    {
        auto deserialized_params = DeserializeOptionalInput(input);
        if (!deserialized_params.has_value())
        {
            return Result<ByteVector>(score::cpp::make_unexpected(deserialized_params.error()));
        }

        auto stop_outcome = handler_.Stop(std::move(*deserialized_params));

        if (!stop_outcome.has_value())
        {
            return Result<ByteVector>(score::cpp::make_unexpected(stop_outcome.error()));
        }

        if (!stop_outcome->has_value())
        {
            return ByteVector{};
        }

        auto serialized_reply = stop_outcome->value().Serialize();

        if (!serialized_reply.has_value())
        {
            return Result<ByteVector>(score::cpp::make_unexpected(NegativeResponseCode::FailurePreventsExecutionOfRequestedAction));
        }

        return std::move(*serialized_reply);
    }

    [[nodiscard]] std::optional<std::uint8_t> CompletionPercentage() const noexcept override
    {
        return handler_.CompletionPercentage();
    }

    ~SerializedRoutineControl() noexcept override = default;

    SerializedRoutineControl(const SerializedRoutineControl&) = delete;
    SerializedRoutineControl(SerializedRoutineControl&&) noexcept = delete;
    SerializedRoutineControl& operator=(const SerializedRoutineControl&) = delete;
    SerializedRoutineControl& operator=(SerializedRoutineControl&&) noexcept = delete;

  private:
    /// Deserialize a raw input into an optional typed DataPayload.
    /// Returns Ok(nullopt) when input is empty (no parameters sent),
    /// Ok(value) on successful parse, or Err(IncorrectMessageLengthOrInvalidFormat) on parse failure.
    [[nodiscard]] static Result<std::optional<DataPayload>>
    DeserializeOptionalInput(ByteView input)
    {
        if (input.empty())
        {
            return std::optional<DataPayload>{std::nullopt};
        }
        auto parsed = DataPayload::FromBytes(input);
        if (!parsed.has_value())
        {
            return Result<std::optional<DataPayload>>(score::cpp::make_unexpected(NegativeResponseCode::IncorrectMessageLengthOrInvalidFormat));
        }
        return std::optional<DataPayload>{std::move(*parsed)};
    }

    HandlerImpl handler_;
};

/************************************/
/* Free-function helper             */
/************************************/

/// Deserialize `data` into `RequestPayload` via `RequestPayload::FromBytes()`, then
/// invoke `handler(typedValue)`. Any parse failure is normalized to
/// `IncorrectMessageLengthOrInvalidFormat` before being returned.
/// `Callable` must accept `RequestPayload` and return `Result<score::cpp::blank>`.
template <typename RequestPayload, typename Callable>
Result<score::cpp::blank> DeserializeRequest(ByteView data, Callable&& on_parsed)
{
    static_assert(detail::HasFromBytesFactory<RequestPayload>::value,
                  "RequestPayload must provide a static FromBytes(ByteView) factory");
    auto parsed_request = RequestPayload::FromBytes(data);

    if (!parsed_request.has_value())
    {
        // Normalize any parse error to the correct UDS wire-level NRC.
        return Result<score::cpp::blank>(score::cpp::make_unexpected(NegativeResponseCode::IncorrectMessageLengthOrInvalidFormat));
    }

    return std::forward<Callable>(on_parsed)(std::move(*parsed_request));
}

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_UDS_SERIALIZATION_H
