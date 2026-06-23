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

/// @file uds_adapters.cpp
/// @brief Out-of-line definitions for DataResourceAdapter and RoutineControlAdapter.

#include "score/mw/diag/uds_adapters.h"

namespace score
{
namespace mw
{
namespace diag
{

// ---------------------------------------------------------------------------
// DataResourceAdapter
// ---------------------------------------------------------------------------

DataResourceAdapter&
DataResourceAdapter::with_rdbi(std::unique_ptr<ReadDataByIdentifier> rdbi) & noexcept
{
    rdbi_ = std::move(rdbi);
    return *this;
}

DataResourceAdapter&
DataResourceAdapter::with_wdbi(std::unique_ptr<WriteDataByIdentifier> wdbi) & noexcept
{
    wdbi_ = std::move(wdbi);
    return *this;
}

Result<ReadValueReply> DataResourceAdapter::read(ReadValueArgs input)
{
    if (!rdbi_)
    {
        return Result<ReadValueReply>{score::unexpect,
                Error::from_error(sovd::GenericError::from_code(
                    sovd::ErrorCode::PreconditionNotFulfilled,
                    "no ReadDataByIdentifier service registered for this data resource"))};
    }
    if (!input.reply_encoding.is_binary())
    {
        return Result<ReadValueReply>{score::unexpect,
                Error::from_error(sovd::GenericError::from_code(
                    sovd::ErrorCode::PreconditionNotFulfilled,
                    "this data resource only supports binary encoding for its reply data"))};
    }
    auto read_result = rdbi_->read();
    if (!read_result.has_value())
    {
        return Result<ReadValueReply>{score::unexpect, read_result.error()};
    }
    return ReadValueReply{
        ReplyMessagePayload::from_byte_vector(std::move(*read_result)),
        std::nullopt};
}

WriteValueResult DataResourceAdapter::write(WriteValueArgs input)
{
    if (!wdbi_)
    {
        return sovd::DataError::from_error(
            sovd::GenericError::from_code(
                sovd::ErrorCode::PreconditionNotFulfilled,
                "no WriteDataByIdentifier service registered for this data resource"));
    }
    const ByteVector* binary_ptr =
        input.user_data.has_value()
            ? std::get_if<RequestMessagePayload::Binary>(&*input.user_data)
            : nullptr;
    if (binary_ptr == nullptr)
    {
        return sovd::DataError::from_error(
            sovd::GenericError::from_code(
                sovd::ErrorCode::IncompleteRequest,
                "this data resource requires binary encoding for its input data"));
    }
    const ByteView view{*binary_ptr};
    auto write_result = wdbi_->write(view);
    if (!write_result.has_value())
    {
        const Error& err = write_result.error();
        if (is_sovd_error(err.code))
        {
            return sovd::DataError{std::string{}, get_sovd_error(err.code)};
        }
        return sovd::DataError::from_error(
            sovd::GenericError::from_code(
                sovd::ErrorCode::ErrorResponse,
                "write operation failed with a UDS negative response code"));
    }
    return std::monostate{};
}

// ---------------------------------------------------------------------------
// RoutineControlAdapter
// ---------------------------------------------------------------------------

RoutineControlAdapter::RoutineControlAdapter(
    std::unique_ptr<RoutineControl> routine_control) noexcept
    : routine_control_{std::move(routine_control)}
{}

Result<ExecutionHandle> RoutineControlAdapter::start(ExecuteArguments input)
{
    // Extract binary payload from user_parameters (only Binary encoding supported).
    std::optional<ByteVector> byte_input;
    if (input.user_parameters.has_value())
    {
        if (!std::holds_alternative<RequestMessagePayload::Binary>(*input.user_parameters))
        {
            return Result<ExecutionHandle>{score::unexpect,
                    Error::from_error(sovd::GenericError::from_code(
                        sovd::ErrorCode::PreconditionNotFulfilled,
                        "UDS RoutineControl only supports binary encoding for its input"))};
        }
        byte_input = std::move(std::get<RequestMessagePayload::Binary>(*input.user_parameters));
    }

    const std::optional<ByteView> view =
        byte_input.has_value()
            ? std::optional<ByteView>{ByteView{*byte_input}}
            : std::nullopt;

    auto start_result = routine_control_->start(view);
    if (!start_result.has_value())
    {
        return Result<ExecutionHandle>{score::unexpect, start_result.error()};
    }

    // Move the StartRoutine out of the Result to avoid leaving a partially-moved value.
    StartRoutine sr = std::move(*start_result);

    // Build optional initial DiagnosticReply from the StartRoutine byte reply.
    std::optional<DiagnosticReply> initial_reply;
    if (sr.reply.has_value())
    {
        initial_reply = DiagnosticReply{
            ReplyMessagePayload::from_byte_vector(std::move(*sr.reply)),
            std::nullopt};
    }

    // Wrap the StartRoutine::result_provider in an ExecutionResult callable.
    ExecutionHandle handle{
        [rp = std::move(sr.result_provider)]() mutable -> ExecutionResult
        {
            if (!rp)
            {
                return DiagnosticReply{};
            }
            auto routine_result = rp();
            if (!routine_result.has_value())
            {
                return ExecutionResult{score::unexpect, routine_result.error()};
            }
            DiagnosticReply diag_reply{};
            if (routine_result->has_value())
            {
                diag_reply.message_payload =
                    ReplyMessagePayload::from_byte_vector(std::move(**routine_result));
            }
            return diag_reply;
        }};
    handle.reply = std::move(initial_reply);
    return handle;
}

Result<std::optional<DiagnosticReply>>
RoutineControlAdapter::stop(std::optional<ExecuteArguments> input)
{
    std::optional<ByteVector> byte_input;
    if (input.has_value() && input->user_parameters.has_value())
    {
        if (!std::holds_alternative<RequestMessagePayload::Binary>(*input->user_parameters))
        {
            return Result<std::optional<DiagnosticReply>>{score::unexpect,
                    Error::from_error(sovd::GenericError::from_code(
                        sovd::ErrorCode::PreconditionNotFulfilled,
                        "UDS RoutineControl only supports binary encoding for its input"))};
        }
        byte_input = std::move(std::get<ByteVector>(*input->user_parameters));
    }

    const std::optional<ByteView> view =
        byte_input.has_value()
            ? std::optional<ByteView>{ByteView{*byte_input}}
            : std::nullopt;

    auto stop_result = routine_control_->stop(view);
    if (!stop_result.has_value())
    {
        return Result<std::optional<DiagnosticReply>>{score::unexpect, stop_result.error()};
    }
    if (!stop_result->has_value())
    {
        return std::optional<DiagnosticReply>{std::nullopt};
    }
    DiagnosticReply reply{};
    reply.message_payload =
        ReplyMessagePayload::from_byte_vector(std::move(**stop_result));
    return std::optional<DiagnosticReply>{std::move(reply)};
}

std::optional<std::uint8_t> RoutineControlAdapter::completion_percentage() const noexcept
{
    return routine_control_->completion_percentage();
}

}  // namespace diag
}  // namespace mw
}  // namespace score
