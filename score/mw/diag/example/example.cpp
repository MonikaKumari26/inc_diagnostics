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

/// @file example.cpp
/// @brief Demonstrates how to use the C++ Diagnostic Middleware APIs.
///
/// Every class below is compiled — if the API changes and the examples drift,
/// the build breaks immediately.
///
/// Sections:
///   1. UDS ReadDataByIdentifier (Service 0x22) — Simple variant
///   2. UDS WriteDataByIdentifier (Service 0x2E) — Simple variant
///   3. UDS GenericDataIdentifier  — combined RDBI + WDBI
///   4. UDS RoutineControl (Service 0x31) — Simple variant
///   5. UDS GenericService — vendor-specific raw handler
///   6. MetaData — session and security gating
///   7. NegativeResponseCode — vendor-specific extension
///   8. DTC — Identifier, Debounce, DTC::Report, SetClearBehaviour, GetNumber, OnInit
///
/// Run with:
///   bazel test //score/mw/diag/example:example_cpp --config=score_diag_x86_64_linux

#include "score/mw/diag/byte_types.h"
#include "score/mw/diag/diag_result.h"
#include "score/mw/diag/dtc/debounce.h"
#include "score/mw/diag/dtc/dtc.h"
#include "score/mw/diag/dtc/identifier.h"
#include "score/mw/diag/uds/generic_data_identifier.h"
#include "score/mw/diag/uds/generic_service.h"
#include "score/mw/diag/uds/meta_data.h"
#include "score/mw/diag/uds/negative_response_code.h"
#include "score/mw/diag/uds/read_data_by_identifier.h"
#include "score/mw/diag/uds/routine_control.h"
#include "score/mw/diag/uds/write_data_by_identifier.h"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>

using score::mw::diag::ByteVector;
using score::mw::diag::ByteView;
using score::mw::diag::uds::DiagnosticSession;
using score::mw::diag::uds::MetaData;
using score::mw::diag::uds::NegativeResponseCode;
using score::mw::diag::uds::Result;

namespace dtc = score::mw::diag::dtc;

// ============================================================================
// Section 1 — ReadDataByIdentifier (Service 0x22)
// ============================================================================

/// Implement SimpleReadDataByIdentifier when the operation is non-blocking.
/// The adapter base class handles the stop_token plumbing automatically.
class VoltageDidHandler : public score::mw::diag::uds::SimpleReadDataByIdentifier
{
  public:
    [[nodiscard]] Result<ByteVector> Read(const MetaData& /*meta_data*/) override
    {
        // 14.4 V as big-endian millivolts: 0x3840 = 14400
        constexpr std::uint16_t kVoltage_mv{14400U};
        ByteVector response;
        response.push_back(static_cast<std::byte>((kVoltage_mv >> 8U) & 0xFFU));
        response.push_back(static_cast<std::byte>(kVoltage_mv & 0xFFU));
        return response;
    }
};

// ============================================================================
// Section 2 — WriteDataByIdentifier (Service 0x2E)
// ============================================================================

/// Implement SimpleWriteDataByIdentifier for non-blocking writes.
class ConfigDidHandler : public score::mw::diag::uds::SimpleWriteDataByIdentifier
{
  public:
    std::uint32_t stored_value{0U};

    [[nodiscard]] Result<void> Write(ByteView input, const MetaData& /*meta_data*/) override
    {
        constexpr std::size_t kExpectedSize{4U};
        if (input.size() != kExpectedSize)
        {
            return score::MakeUnexpected(
                NegativeResponseCode::IncorrectMessageLengthOrInvalidFormat);
        }
        stored_value =
            (static_cast<std::uint32_t>(input[0]) << 24U) |
            (static_cast<std::uint32_t>(input[1]) << 16U) |
            (static_cast<std::uint32_t>(input[2]) <<  8U) |
             static_cast<std::uint32_t>(input[3]);
        return {};
    }
};

// ============================================================================
// Section 3 — GenericDataIdentifier (RDBI + WDBI combined)
// ============================================================================

/// Implement SimpleGenericDataIdentifier to handle both read and write on a
/// single DID without managing two separate handler objects.
class OdometerDid : public score::mw::diag::uds::SimpleGenericDataIdentifier
{
  public:
    std::uint32_t odometer_km{0U};

    [[nodiscard]] Result<ByteVector> Read(const MetaData& /*meta_data*/) override
    {
        ByteVector buf(4U);
        buf[0] = static_cast<std::byte>((odometer_km >> 24U) & 0xFFU);
        buf[1] = static_cast<std::byte>((odometer_km >> 16U) & 0xFFU);
        buf[2] = static_cast<std::byte>((odometer_km >>  8U) & 0xFFU);
        buf[3] = static_cast<std::byte>( odometer_km         & 0xFFU);
        return buf;
    }

    [[nodiscard]] Result<void> Write(ByteView input, const MetaData& /*meta_data*/) override
    {
        if (input.size() != 4U)
        {
            return score::MakeUnexpected(
                NegativeResponseCode::IncorrectMessageLengthOrInvalidFormat);
        }
        odometer_km =
            (static_cast<std::uint32_t>(input[0]) << 24U) |
            (static_cast<std::uint32_t>(input[1]) << 16U) |
            (static_cast<std::uint32_t>(input[2]) <<  8U) |
             static_cast<std::uint32_t>(input[3]);
        return {};
    }
};

// ============================================================================
// Section 4 — RoutineControl (Service 0x31)
// ============================================================================

/// Implement SimpleRoutineControl for routines that respond synchronously.
class CalibrationRoutine : public score::mw::diag::uds::SimpleRoutineControl
{
  public:
    enum class State { Idle, Running, Done };
    State state{State::Idle};
    std::uint8_t progress{0U};

    [[nodiscard]] Result<ByteVector> Start(
        ByteView /*input*/, const MetaData& /*meta_data*/) override
    {
        state = State::Running;
        progress = 0U;
        return ByteVector{std::byte{0x00}};
    }

    [[nodiscard]] Result<ByteVector> Stop(
        ByteView /*input*/, const MetaData& /*meta_data*/) override
    {
        state = State::Done;
        return ByteVector{std::byte{0x01}};
    }

    [[nodiscard]] Result<ByteVector> RequestResults(
        ByteView /*input*/, const MetaData& /*meta_data*/) override
    {
        return ByteVector{std::byte{static_cast<std::byte>(progress)}};
    }

    [[nodiscard]] std::optional<std::uint8_t> CompletionPercentage() const noexcept override
    {
        return progress;
    }
};

// ============================================================================
// Section 5 — GenericService (vendor-specific raw handler)
// ============================================================================

/// Implement SimpleGenericService for proprietary service IDs.
class EchoService : public score::mw::diag::uds::SimpleGenericService
{
  public:
    [[nodiscard]] Result<ByteVector> HandleMessage(
        ByteView input, const MetaData& /*meta_data*/) override
    {
        return ByteVector{input.begin(), input.end()};
    }
};

// ============================================================================
// Section 6 — MetaData — session and security gating
// ============================================================================

/// Gate access on session and security level by inspecting MetaData.
class ProtectedDidHandler : public score::mw::diag::uds::SimpleReadDataByIdentifier
{
  public:
    [[nodiscard]] Result<ByteVector> Read(const MetaData& meta_data) override
    {
        if (meta_data.session == DiagnosticSession::Default)
        {
            return score::MakeUnexpected(
                NegativeResponseCode::ServiceNotSupportedInActiveSession);
        }
        if (!meta_data.security_level.has_value())
        {
            return score::MakeUnexpected(NegativeResponseCode::SecurityAccessDenied);
        }
        return ByteVector{std::byte{0xAB}, std::byte{0xCD}};
    }
};

// ============================================================================
// Section 8 — DTC: DTC::Report, SetClearBehaviour, GetNumber, OnInit
// ============================================================================

/// In production the runtime injects a real DTC handle via the Builder.
/// This function demonstrates the full DTC interface — it is compiled to
/// guarantee API correctness but not called from main() (no runtime present).
static void demonstrate_dtc(dtc::DTC& dtc_handle)
{
    // Always register OnInit — reset application debounce state on
    // restart, tester clear, and re-enable events.
    dtc_handle.OnInit([](dtc::InitReason /*reason*/) {
        // reset internal fault counters here
    });

    // Report every monitoring cycle — the runtime's debouncer needs
    // both kPassed and kFailed to track transitions correctly.
    const float voltage_v{12.0F};
    const auto status = (voltage_v > 16.0F || voltage_v < 9.5F)
        ? dtc::Status::kFailed
        : dtc::Status::kPassed;
    static_cast<void>(dtc_handle.Report(status));

    // Query the DTC number in UDS encoding.
    const auto num = dtc_handle.GetNumber(dtc::FormatType::kUds);
    static_cast<void>(num);

    // Change clear behaviour at runtime if needed.
    dtc_handle.SetClearBehaviour(dtc::ClearBehaviour::kNotClearable);
}

// ============================================================================
// Entry point — exercises each API section and verifies correctness
// ============================================================================

int main()
{
    // ── Section 1: ReadDataByIdentifier ──────────────────────────────────────
    {
        VoltageDidHandler handler{};
        const auto r = handler.Read(MetaData{});
        assert(r.has_value());
        assert(r.value()[0] == std::byte{0x38U});  // 14400 = 0x3840
        assert(r.value()[1] == std::byte{0x40U});
    }

    // ── Section 2: WriteDataByIdentifier ─────────────────────────────────────
    {
        ConfigDidHandler handler{};
        const std::array<std::byte, 4> payload{
            std::byte{0x00}, std::byte{0x00}, std::byte{0x01}, std::byte{0x00}};
        assert(handler.Write(ByteView{payload.data(), payload.size()}, MetaData{}).has_value());
        assert(handler.stored_value == 256U);
    }

    // ── Section 3: GenericDataIdentifier (write → read round-trip) ───────────
    {
        OdometerDid did{};
        constexpr std::uint32_t kKm{123456U};
        const std::array<std::byte, 4> payload{
            static_cast<std::byte>((kKm >> 24U) & 0xFFU),
            static_cast<std::byte>((kKm >> 16U) & 0xFFU),
            static_cast<std::byte>((kKm >>  8U) & 0xFFU),
            static_cast<std::byte>( kKm         & 0xFFU)};
        assert(did.Write(ByteView{payload.data(), payload.size()}, MetaData{}).has_value());
        const auto r = did.Read(MetaData{});
        assert(r.has_value());
        const std::uint32_t readback =
            (static_cast<std::uint32_t>(r.value()[0]) << 24U) |
            (static_cast<std::uint32_t>(r.value()[1]) << 16U) |
            (static_cast<std::uint32_t>(r.value()[2]) <<  8U) |
             static_cast<std::uint32_t>(r.value()[3]);
        assert(readback == kKm);
    }

    // ── Section 4: RoutineControl ─────────────────────────────────────────────
    {
        CalibrationRoutine routine{};
        const ByteView empty{};
        assert(routine.Start(empty, MetaData{}).has_value());
        assert(routine.state == CalibrationRoutine::State::Running);
        routine.progress = 42U;
        const auto results = routine.RequestResults(empty, MetaData{});
        assert(results.has_value());
        assert(results.value()[0] == std::byte{42U});
        assert(routine.Stop(empty, MetaData{}).has_value());
        assert(routine.state == CalibrationRoutine::State::Done);
    }

    // ── Section 5: GenericService ─────────────────────────────────────────────
    {
        EchoService svc{};
        const std::array<std::byte, 3> payload{
            std::byte{0x11}, std::byte{0x22}, std::byte{0x33}};
        const auto r = svc.HandleMessage(ByteView{payload.data(), payload.size()}, MetaData{});
        assert(r.has_value());
        assert(r.value()[0] == std::byte{0x11});
        assert(r.value()[2] == std::byte{0x33});
    }

    // ── Section 6: MetaData ───────────────────────────────────────────────────
    {
        ProtectedDidHandler handler{};
        MetaData meta{};
        meta.session        = DiagnosticSession::Extended;
        meta.security_level = std::uint8_t{0x01};
        const auto r = handler.Read(meta);
        assert(r.has_value());
        assert(r.value().size() == 2U);
    }

    // ── Section 7: NegativeResponseCode ──────────────────────────────────────
    {
        // Compile-time construction — range violation is a compile error
        const auto nrc =
            score::mw::diag::uds::VehicleManufacturerSpecificCNC::FromValue<0xF2U>();
        assert(nrc.Value() == 0xF2U);

        // Runtime construction — returns nullopt when out of range
        assert(!score::mw::diag::uds::VehicleManufacturerSpecificCNC::FromValue(0x10U)
                    .has_value());
    }

    // ── Section 8: DTC identifiers and debounce ───────────────────────────────
    {
        // Strong identifier types prevent accidental argument swaps at compile time
        const dtc::MonitorIdentifier   monitor{"mon/voltage_sensor"};
        const dtc::EventIdentifier     event{"evt/voltage_out_of_range"};
        const dtc::ConditionIdentifier condition{"cond/ignition_on"};
        assert(monitor.GetValue()   == "mon/voltage_sensor");
        assert(event.GetValue()     == "evt/voltage_out_of_range");
        assert(condition.GetValue() == "cond/ignition_on");

        // Debounce: None (pass-through), TimeBased, or CounterBased
        const dtc::Debounce none_d{};
        assert(!none_d.IsSet());

        const dtc::Debounce time_d{dtc::Debounce::TimeBased{
            std::chrono::milliseconds{200},
            std::chrono::milliseconds{100}}};
        assert(time_d.IsSet());
        assert(std::holds_alternative<dtc::Debounce::TimeBased>(time_d.GetAlgorithm()));
        const auto& t = std::get<dtc::Debounce::TimeBased>(time_d.GetAlgorithm());
        assert(t.failed_duration == std::chrono::milliseconds{200});
        assert(t.passed_duration == std::chrono::milliseconds{100});
    }

    // DTC::Report / SetClearBehaviour / GetNumber / OnInit require a runtime-
    // provided handle.  demonstrate_dtc() is compiled to verify API correctness;
    // it is not called here because no runtime is present in this example.
    static_cast<void>(demonstrate_dtc);

    return 0;
}
