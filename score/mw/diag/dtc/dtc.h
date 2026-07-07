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

/// @file dtc.h
/// @brief DTC interface and associated types (Status, InitReason, FormatType).

#ifndef SCORE_MW_DIAG_DTC_DTC_H
#define SCORE_MW_DIAG_DTC_DTC_H

#include "score/mw/diag/diag_result.h"
#include "score/move_only_function.hpp"

#include <cstdint>

namespace score::mw::diag::dtc
{

/************************************/
/* FormatType                       */
/************************************/

/// @brief DTC number format to request via DTC::Number().
enum class FormatType : std::uint8_t
{
    kObd = 0U,    ///< OBD-II (J1979).
    kUds = 1U,    ///< ISO 14229-1 (UDS).
    kJ1939 = 2U,  ///< SAE J1939.
};

/************************************/
/* InitReason                       */
/************************************/

/// @brief Reason code delivered to the DTC::OnInit() callback.
///        Reset internal debouncing state whenever the callback is invoked.
enum class InitReason : std::uint8_t
{
    kClear,             ///< DTC storage was cleared by a tester.
    kRestart,           ///< ECU restarted (power-on or warm reset).
    kReenabled,         ///< DTC monitoring re-enabled after an enable-condition change.
    kStorageReenabled,  ///< DTC storage re-enabled after a storage-condition change.
};

/// @brief Fault-monitor outcome for a single monitoring cycle.
enum class Status : std::uint8_t
{
    kPassed,  ///< Signal within healthy range.
    kFailed,  ///< Signal outside healthy range (fault detected).
};

/// @brief Abstract interface for reporting fault status on a single DTC.
class DTC
{
  public:
    /// @brief Report the fault status for one monitoring cycle.
    /// @param status Outcome of the current monitoring cycle (kPassed or kFailed).
    /// @return Ok on success; Err if the middleware could not process the report.
    [[nodiscard]] virtual Result<score::cpp::blank> Report(Status status) = 0;

    /// @brief Allow this DTC to be cleared by a tester ClearDiagnosticInformation request (default).
    virtual void MakeClearable() noexcept = 0;

    /// @brief Prevent this DTC from being cleared by a tester ClearDiagnosticInformation request.
    virtual void MakeNotClearable() noexcept = 0;

    /// @brief Return the DTC number in the requested format.
    /// @param format The numeric format to use (OBD, UDS, or J1939).
    /// @return The DTC number on success; Err if the requested format is unsupported.
    [[nodiscard]] virtual Result<std::uint32_t> Number(FormatType format) const noexcept = 0;

    /// @brief Instruct the runtime to re-enter this DTC immediately after a tester
    ///        ClearDiagnosticInformation request.
    virtual void ReenterAfterCleared() noexcept = 0;

    /// @brief Register a callback invoked with an InitReason on startup, clear, or re-enable.
    /// @param callback Invoked with the InitReason each time the DTC is (re-)initialised.
    virtual void OnInit(score::cpp::move_only_function<void(InitReason)> callback) = 0;

    DTC(const DTC&) = delete;
    DTC(DTC&&) noexcept = delete;
    DTC& operator=(const DTC&) = delete;
    DTC& operator=(DTC&&) noexcept = delete;
    virtual ~DTC() noexcept = default;

  protected:
    DTC() = default;
};

}  // namespace score::mw::diag::dtc

#endif  // SCORE_MW_DIAG_DTC_DTC_H
