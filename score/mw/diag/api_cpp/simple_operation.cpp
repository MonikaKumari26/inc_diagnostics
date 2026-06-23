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

/// @file simple_operation.cpp
/// @brief Out-of-line definitions for SimpleOperation and SimpleOperationAdapter.

#include "score/mw/diag/simple_operation.h"

namespace score
{
namespace mw
{
namespace diag
{

SimpleOperation::~SimpleOperation() noexcept = default;

// ---------------------------------------------------------------------------
// SimpleOperationAdapter
// ---------------------------------------------------------------------------

Result<ExecutionHandle> SimpleOperationAdapter::execute(
    ExecuteArguments input, ExecutionControl& control)
{
    // Exclusive execution: reject if already running.
    if (active_exec_id_.has_value())
    {
        return Result<ExecutionHandle>{score::unexpect,
                Error::from_error(sovd::GenericError::from_code(
                    sovd::ErrorCode::PreconditionNotFulfilled,
                    "operation is already executing"))};
    }
    active_exec_id_ = control.exec_id();

    // Delegate start to the wrapped SimpleOperation.
    auto handle_result = wrapped_operation_->start(std::move(input));
    if (!handle_result.has_value())
    {
        active_exec_id_.reset();
        return handle_result;
    }

    ExecutionHandle exec_handle = std::move(*handle_result);

    // Wrap the op's future in a synchronous event-processing loop.
    // Lifetime note: operation_ptr and exec_id alias into this adapter — the adapter MUST
    // outlive the returned future.  control is captured by reference and MUST also
    // outlive the future.
    SimpleOperation*                 operation_ptr = wrapped_operation_.get();
    std::optional<ExecutionId>*      exec_id       = &active_exec_id_;
    std::function<ExecutionResult()> start_future  = std::move(exec_handle.future);

    exec_handle.future = [operation_ptr, exec_id, &control,
                          fut    = std::move(start_future),
                          errors = std::vector<Error>{}]() mutable -> ExecutionResult
    {
        while (true)
        {
            ExecutionEvent event = control.next_exec_event();

            switch (event.kind)
            {
                case ExecutionEventKind::ControlGone:
                {
                    exec_id->reset();
                    // Report any accumulated error events before returning.
                    if (!errors.empty())
                    {
                        event.status_reporter.put(
                            ExecutionStatus::Completed,
                            ExecutionStatusDetails{}.with_exec_errors(std::move(errors)));
                    }
                    return fut();
                }

                case ExecutionEventKind::Stop:
                {
                    auto stop_result = operation_ptr->stop(std::move(event.args));
                    exec_id->reset();
                    if (!stop_result.has_value())
                    {
                        return ExecutionResult{score::unexpect, stop_result.error()};
                    }
                    // Only attach a reply payload when the stop handler produced one.
                    auto details = ExecutionStatusDetails{};
                    if (stop_result->has_value())
                    {
                        details = std::move(details).with_reply_data(std::move(**stop_result));
                    }
                    event.status_reporter.put(ExecutionStatus::Stopped, std::move(details));
                    return ExecutionResult{score::unexpect,
                            Error::from_error(sovd::GenericError::from_code(
                                sovd::ErrorCode::ErrorResponse,
                                "operation was stopped"))};
                }

                case ExecutionEventKind::ReportStatus:
                {
                    auto details = ExecutionStatusDetails{};
                    const auto maybe_completion_pct = operation_ptr->completion_percentage();
                    if (maybe_completion_pct.has_value())
                    {
                        details = std::move(details).with_completion_percentage(*maybe_completion_pct);
                    }
                    event.status_reporter.put(ExecutionStatus::Running, std::move(details));
                    break;
                }

                default:
                {
                    // HandleCustomCapability: surface the capability name in status details.
                    // Error: accumulate for the final exec_errors report on ControlGone.
                    // Other unhandled events (Interrupt, Resume, Reset): report as UnsupportedCapability.
                    if (event.kind == ExecutionEventKind::Error)
                    {
                        if (event.error_payload.has_value())
                        {
                            errors.push_back(std::move(*event.error_payload));
                        }
                        break;
                    }
                    auto details = ExecutionStatusDetails{};
                    if (event.kind == ExecutionEventKind::HandleCustomCapability &&
                        event.capability_name.has_value())
                    {
                        details.last_executed_capability = *event.capability_name;
                    }
                    event.status_reporter.put(ExecutionStatus::UnsupportedCapability,
                                              std::move(details));
                    break;
                }
            }
        }
    };

    return exec_handle;
}

}  // namespace diag
}  // namespace mw
}  // namespace score