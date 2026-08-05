# Eclipse SCORE — Diagnostic Middleware C++ API

`score::mw::diag` is a C++ middleware library for vehicle diagnostics. It
provides pure-virtual abstract interfaces that your application code
implements. The diagnostic runtime supplies the concrete implementations at
link time — your business logic never depends on runtime internals.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Architecture](#2-architecture)
3. [Build & Run](#3-build--run)
4. [Common Types](#4-common-types)
5. [API Mapping Summary](#5-api-mapping-summary)
6. [UDS Service Interfaces](#6-uds-service-interfaces)
   - [6.1 ReadDataByIdentifier (Service 0x22)](#61-readdatabyidentifier-service-0x22)
   - [6.2 WriteDataByIdentifier (Service 0x2E)](#62-writedatabyidentifier-service-0x2e)
   - [6.3 GenericDataIdentifier (RDBI + WDBI combined)](#63-genericdataidentifier-rdbi--wdbi-combined)
   - [6.4 RoutineControl (Service 0x31)](#64-routinecontrol-service-0x31)
   - [6.5 GenericService (vendor-specific services)](#65-genericservice-vendor-specific-services)
7. [DTC Interfaces](#7-dtc-interfaces)
   - [7.1 Identifiers](#71-identifiers)
   - [7.2 Debounce Configuration](#72-debounce-configuration)
   - [7.3 DTC Interface](#73-dtc-interface)
8. [Error Handling](#8-error-handling)
9. [Request / Response Lifecycle](#9-request--response-lifecycle)
10. [Best Practices](#10-best-practices)
11. [Key Headers](#11-key-headers)

---

## 1. Overview

The library covers two independent API families:

| Area | ISO Reference | Purpose |
| :--- | :--- | :--- |
| **UDS** | ISO 14229-1:2020 | Implement handlers for tester-initiated diagnostic services |
| **DTC** | ISO 14229-1 | Register fault monitors and report fault status to the runtime |

### Key Design Principles

- **Decoupled business logic** — implement clean abstract interfaces; no
  raw diagnostic frames in your code.
- **Dual abstraction** — choose fast synchronous (`Simple*`) adapters or
  full async handlers with `stop_token` cancellation support.
- **Compile-time safety** — strong identifier types (`MonitorIdentifier`,
  `EventIdentifier`, `ConditionIdentifier`) prevent argument-order mistakes.

### Compilable Example

A compiler-verified C++ usage example lives alongside the Rust example at:
[`score/mw/diag/example/example.cpp`](example/example.cpp)

---

## 2. Architecture

```text
┌─────────────────────────────────────────────────────────────────┐
│                    User Application Software                     │
│  ┌────────────────────────┐      ┌─────────────────────────┐    │
│  │  UDS Handler Classes   │      │   Fault Monitor Classes  │    │
│  │  (Simple* or Async*)   │      │   (dtc::DTC via runtime) │    │
│  └──────────┬─────────────┘      └────────────┬────────────┘    │
└─────────────┼──────────────────────────────────┼────────────────┘
              │  implements                       │  reports
┌─────────────▼──────────────────────────────────▼────────────────┐
│              score::mw::diag Abstract Interfaces                  │
│      (this library — headers only, no runtime dependencies)      │
└─────────────────────────────────┬───────────────────────────────┘
                                  │  injected at link time
┌─────────────────────────────────▼───────────────────────────────┐
│              Diagnostic Middleware Runtime                        │
│         (provided by the platform, not this repository)          │
└─────────────────────────────────────────────────────────────────┘
```

> **This library contains only abstract interfaces and common types.**
> The runtime that the platform provides supplies all concrete implementations.

---

## 3. Build & Run

```sh
# Build all diagnostic middleware targets
bazel build --config=score_diag_x86_64_linux //score/mw/diag/...

# Run all tests (unit + example)
bazel test --config=score_diag_x86_64_linux //score/mw/diag/...

# Run only the C++ example
bazel test --config=score_diag_x86_64_linux //score/mw/diag/example:example_cpp
```

### Key Platform Symbols

| Symbol | Header | Purpose |
| :--- | :--- | :--- |
| `score::MakeUnexpected(nrc)` | `score/result/result.h` | Construct an error-carrying `Result` |
| `score::cpp::stop_token` | transitively via `read_data_by_identifier.h` | Cooperative cancellation for async handlers |
| `score::cpp::span<const std::byte>` | `score/mw/diag/byte_types.h` | C++17 non-owning byte range; `ByteView` is a typedef |

---

## 4. Common Types

```cpp
#include "score/mw/diag/byte_types.h"       // ByteVector, ByteView
#include "score/mw/diag/diag_result.h"       // Result<T>
#include "score/mw/diag/uds/negative_response_code.h"  // NegativeResponseCode
#include "score/mw/diag/uds/meta_data.h"     // MetaData
```

| Type | Definition | Ownership |
| :--- | :--- | :--- |
| `ByteVector` | `std::vector<std::byte>` | **Owning** — use for response data you build |
| `ByteView` | `score::cpp::span<const std::byte>` | **Non-owning** — use for input; never store beyond the call |
| `Result<T>` | `score::Result<T>` | Holds either a value of `T` or a `NegativeResponseCode` error |
| `Result<void>` | `score::Result<void>` | Success with no payload; return `{}` on success |
| `MetaData` | `struct MetaData` | Request context: `session`, `security_level`, `addressing` |

### Result\<T\> Usage

```cpp
// Return success:
return ByteVector{std::byte{0x01}, std::byte{0x02}};   // implicit conversion

// Return success (void):
return {};

// Return failure:
return score::MakeUnexpected(NegativeResponseCode::RequestOutOfRange);

// Check at call site:
auto result = handler.Read(meta_data);
if (result.has_value()) {
    ByteVector data = result.value();
} else {
    NegativeResponseCode nrc = result.error().Code();
}
```

### MetaData

`MetaData` is passed by the runtime to every handler call. Inspect it to
implement session or security-level gating:

```cpp
Result<ByteVector> Read(const MetaData& meta_data) override {
    if (meta_data.session == DiagnosticSession::Default) {
        return score::MakeUnexpected(
            NegativeResponseCode::ServiceNotSupportedInActiveSession);
    }
    if (!meta_data.security_level.has_value()) {
        return score::MakeUnexpected(NegativeResponseCode::SecurityAccessDenied);
    }
    // proceed...
}
```

---

## 5. API Mapping Summary

For every UDS service the library provides two variants. **Use `Simple*`
unless your operation may block** (e.g. NVM write, hardware polling).

| Service | ISO SID | Full Async Interface | Simplified Adapter |
| :--- | :--- | :--- | :--- |
| Read DID | 0x22 | `ReadDataByIdentifier` | `SimpleReadDataByIdentifier` |
| Write DID | 0x2E | `WriteDataByIdentifier` | `SimpleWriteDataByIdentifier` |
| Combined DID | 0x22 + 0x2E | `GenericDataIdentifier` | `SimpleGenericDataIdentifier` |
| Routine Control | 0x31 | `RoutineControl` | `SimpleRoutineControl` |
| Generic Service | vendor | `GenericService` | `SimpleGenericService` |

**Full Async** — override a method returning `std::future<Result<T>>` with a
`stop_token` parameter; suitable for long-running or cancellable work.

**Simple Adapter** — override a synchronous method returning `Result<T>`
directly; the base class wraps it in a `std::promise`/`std::future`
internally and ignores the `stop_token`.

---

## 6. UDS Service Interfaces

### 6.1 ReadDataByIdentifier (Service 0x22)

**Header:** `score/mw/diag/uds/read_data_by_identifier.h`

Implement `SimpleReadDataByIdentifier` for fast, non-blocking reads:

```cpp
class VoltageDidHandler : public score::mw::diag::uds::SimpleReadDataByIdentifier
{
public:
    [[nodiscard]] Result<ByteVector> Read(const MetaData& /*meta_data*/) override
    {
        constexpr std::uint16_t kVoltage_mv{14400U};  // 14.4 V
        ByteVector response;
        response.push_back(static_cast<std::byte>((kVoltage_mv >> 8U) & 0xFFU));
        response.push_back(static_cast<std::byte>(kVoltage_mv & 0xFFU));
        return response;  // runtime sends: 0x62 <DID> 0x38 0x40
    }
};
```

Implement `ReadDataByIdentifier` when the read requires async execution or
must honour cancellation:

```cpp
class SlowNvmDidHandler : public score::mw::diag::uds::ReadDataByIdentifier
{
public:
    [[nodiscard]] std::future<Result<ByteVector>> Read(
        const MetaData& /*meta_data*/,
        score::cpp::stop_token stop_token) override
    {
        return std::async(std::launch::async,
            [st = std::move(stop_token)]() -> Result<ByteVector> {
                if (st.stop_requested()) {
                    return score::MakeUnexpected(
                        NegativeResponseCode::ConditionsNotCorrect);
                }
                return ReadFromNvm();
            });
    }
};
```

---

### 6.2 WriteDataByIdentifier (Service 0x2E)

**Header:** `score/mw/diag/uds/write_data_by_identifier.h`

```cpp
class ConfigDidHandler : public score::mw::diag::uds::SimpleWriteDataByIdentifier
{
public:
    [[nodiscard]] Result<void> Write(
        ByteView input, const MetaData& /*meta_data*/) override
    {
        if (input.size() != 4U) {
            return score::MakeUnexpected(
                NegativeResponseCode::IncorrectMessageLengthOrInvalidFormat);
        }
        StoreConfig(input);
        return {};  // runtime sends: 0x6E <DID>
    }
};
```

---

### 6.3 GenericDataIdentifier (RDBI + WDBI combined)

**Header:** `score/mw/diag/uds/generic_data_identifier.h`

Use `SimpleGenericDataIdentifier` when a single DID must handle both read
(0x22) and write (0x2E):

```cpp
class OdometerDid : public score::mw::diag::uds::SimpleGenericDataIdentifier
{
public:
    [[nodiscard]] Result<ByteVector> Read(const MetaData& /*meta_data*/) override
    {
        std::uint32_t km = GetOdometer();
        ByteVector buf(4U);
        buf[0] = static_cast<std::byte>((km >> 24U) & 0xFFU);
        buf[1] = static_cast<std::byte>((km >> 16U) & 0xFFU);
        buf[2] = static_cast<std::byte>((km >>  8U) & 0xFFU);
        buf[3] = static_cast<std::byte>( km         & 0xFFU);
        return buf;
    }

    [[nodiscard]] Result<void> Write(
        ByteView input, const MetaData& /*meta_data*/) override
    {
        if (input.size() != 4U) {
            return score::MakeUnexpected(
                NegativeResponseCode::IncorrectMessageLengthOrInvalidFormat);
        }
        SetOdometer(
            (static_cast<std::uint32_t>(input[0]) << 24U) |
            (static_cast<std::uint32_t>(input[1]) << 16U) |
            (static_cast<std::uint32_t>(input[2]) <<  8U) |
             static_cast<std::uint32_t>(input[3]));
        return {};
    }
};
```

---

### 6.4 RoutineControl (Service 0x31)

**Header:** `score/mw/diag/uds/routine_control.h`

| Method | Sub-function | Purpose |
| :--- | :--- | :--- |
| `Start()` | 0x01 | Start the routine |
| `Stop()` | 0x02 | Signal routine cancellation |
| `RequestResults()` | 0x03 | Retrieve final execution status |
| `CompletionPercentage()` | *(optional)* | Report 0–100 progress for SOVD tools |

```cpp
class CalibrationRoutine : public score::mw::diag::uds::SimpleRoutineControl
{
public:
    [[nodiscard]] Result<ByteVector> Start(
        ByteView /*input*/, const MetaData& /*meta_data*/) override
    {
        StartCalibration();
        return ByteVector{std::byte{0x00}};  // status: started
    }

    [[nodiscard]] Result<ByteVector> Stop(
        ByteView /*input*/, const MetaData& /*meta_data*/) override
    {
        StopCalibration();
        return ByteVector{std::byte{0x01}};  // status: stopped
    }

    [[nodiscard]] Result<ByteVector> RequestResults(
        ByteView /*input*/, const MetaData& /*meta_data*/) override
    {
        return ByteVector{static_cast<std::byte>(GetStatus())};
    }

    [[nodiscard]] std::optional<std::uint8_t>
    CompletionPercentage() const noexcept override
    {
        return progress_;
    }

private:
    std::uint8_t progress_{0U};
};
```

---

### 6.5 GenericService (vendor-specific services)

**Header:** `score/mw/diag/uds/generic_service.h`

Use `SimpleGenericService` for proprietary UDS service identifiers:

```cpp
class EchoService : public score::mw::diag::uds::SimpleGenericService
{
public:
    [[nodiscard]] Result<ByteVector> HandleMessage(
        ByteView input, const MetaData& /*meta_data*/) override
    {
        if (input.empty()) {
            return score::MakeUnexpected(
                NegativeResponseCode::IncorrectMessageLengthOrInvalidFormat);
        }
        return ByteVector{input.begin(), input.end()};
    }
};
```

---

## 7. DTC Interfaces

DTC headers live in `score/mw/diag/dtc/`. The `DTC` handle and the `Builder`
that creates it are **provided by the diagnostic middleware runtime** — your
application code never constructs them directly.

### 7.1 Identifiers

**Header:** `score/mw/diag/dtc/identifier.h`

Three strong-typed identifier wrappers prevent argument-order mistakes at
compile time. Passing a `MonitorIdentifier` where an `EventIdentifier` is
expected is a compile error.

| Type | Purpose | Example |
| :--- | :--- | :--- |
| `MonitorIdentifier` | Identifies the fault monitor | `"mon/voltage_sensor"` |
| `EventIdentifier` | Identifies the diagnostic event | `"evt/voltage_out_of_range"` |
| `ConditionIdentifier` | Identifies the clear condition | `"cond/ignition_on"` |

```cpp
#include "score/mw/diag/dtc/identifier.h"

using namespace score::mw::diag::dtc;

MonitorIdentifier   monitor{"mon/voltage_sensor"};    // non-empty; contract violation otherwise
EventIdentifier     event{"evt/voltage_out_of_range"};
ConditionIdentifier condition{"cond/ignition_on"};

// All three support ==, !=, < — safe to store in std::map / std::set
EXPECT_LT(MonitorIdentifier{"mon/a"}, MonitorIdentifier{"mon/b"});
```

---

### 7.2 Debounce Configuration

**Header:** `score/mw/diag/dtc/debounce.h`

`Debounce` configures how the runtime filters rapidly fluctuating signals
before recording a fault. Three modes are available:

#### No debouncing (default — pass-through)

```cpp
score::mw::diag::dtc::Debounce d{};  // every Report() forwards immediately
```

#### Time-based

The fault is recorded only after it persists continuously for
`failed_duration`. Recovery requires the healthy signal to persist for
`passed_duration`.

```cpp
score::mw::diag::dtc::Debounce d{
    score::mw::diag::dtc::Debounce::TimeBased{
        std::chrono::milliseconds{200},  // failed_duration
        std::chrono::milliseconds{100}   // passed_duration
    }
};
```

#### Counter-based

An internal counter is incremented on `kFailed` and decremented on `kPassed`
according to the configured step sizes and thresholds.

| Field | Description |
| :--- | :--- |
| `failed_threshold` | Counter threshold to qualify as Failed (positive) |
| `passed_threshold` | Counter threshold to qualify as Passed (negative) |
| `failed_stepsize` / `passed_stepsize` | Counter increment/decrement per Report() |
| `failed_jump_value` / `passed_jump_value` | Optional counter jump on first report |
| `use_jump_to_failed` / `use_jump_to_passed` | Enable the jump feature |

```cpp
score::mw::diag::dtc::Debounce d{
    score::mw::diag::dtc::Debounce::CounterBased{
        /* failed_threshold  = */  10,
        /* passed_threshold  = */ -5,
        /* failed_stepsize   = */  2U,
        /* passed_stepsize   = */  1U,
        /* failed_jump_value = */  5,
        /* passed_jump_value = */ -3,
        /* use_jump_to_failed = */ true,
        /* use_jump_to_passed = */ false
    }
};
```

---

### 7.3 DTC Interface

**Header:** `score/mw/diag/dtc/dtc.h`

`DTC` is the handle returned by the Builder (injected by the runtime).
Keep it alive for the lifetime of the monitoring cycle.

```cpp
class DTC {
public:
    // Report the outcome of one monitoring cycle — call every cycle
    [[nodiscard]] virtual Result<void> Report(Status status) = 0;

    // Change clearing behaviour at runtime
    virtual void SetClearBehaviour(ClearBehaviour behaviour) noexcept = 0;

    // Query the DTC number in a specific encoding format
    [[nodiscard]] virtual Result<std::uint32_t>
    GetNumber(FormatType format) const noexcept = 0;

    // Register a callback invoked on restart, tester clear, or re-enable
    virtual void OnInit(
        score::cpp::move_only_function<void(InitReason)> callback) = 0;
};
```

#### Status

| Value | Meaning |
| :--- | :--- |
| `Status::kPassed` | Signal within the healthy range |
| `Status::kFailed` | Signal outside the healthy range (fault detected) |

#### ClearBehaviour

| Value | Meaning |
| :--- | :--- |
| `ClearBehaviour::kClearable` | (Default) Tester may clear this DTC |
| `ClearBehaviour::kNotClearable` | Tester clear requests are ignored |
| `ClearBehaviour::kReenterAfterCleared` | DTC re-enters storage immediately after a tester clear |

#### FormatType

| Value | Standard |
| :--- | :--- |
| `FormatType::kObd` | OBD-II (J1979) |
| `FormatType::kUds` | ISO 14229-1 (UDS) |
| `FormatType::kJ1939` | SAE J1939 |

#### InitReason (OnInit callback argument)

| Value | Trigger |
| :--- | :--- |
| `InitReason::kRestart` | ECU power-on or warm reset |
| `InitReason::kClear` | Tester sent `ClearDiagnosticInformation` |
| `InitReason::kReenabled` | DTC monitoring re-enabled |
| `InitReason::kStorageReenabled` | DTC storage re-enabled |

#### Complete Monitoring Example

```cpp
class VoltageMonitor {
public:
    explicit VoltageMonitor(score::mw::diag::dtc::DTC& dtc) : dtc_{dtc}
    {
        // Always register OnInit — reset internal state on clear/restart
        dtc_.OnInit([this](score::mw::diag::dtc::InitReason /*reason*/) {
            fault_counter_ = 0;
        });
    }

    void RunCycle(float voltage_v)
    {
        // Report every cycle — the runtime's debouncer needs both kPassed
        // and kFailed reports to track transitions correctly
        const auto status = (voltage_v > 16.0F || voltage_v < 9.5F)
            ? score::mw::diag::dtc::Status::kFailed
            : score::mw::diag::dtc::Status::kPassed;

        if (auto r = dtc_.Report(status); !r) {
            // handle error (e.g. storage inhibited)
        }
    }

private:
    score::mw::diag::dtc::DTC& dtc_;
    int fault_counter_{0};
};
```

#### Builder (provided by the runtime)

The `Builder` is injected by the diagnostic runtime — you never construct it.
Chain configuration calls and call `Build()` once to obtain the `DTC` handle:

```cpp
std::unique_ptr<score::mw::diag::dtc::DTC> dtc =
    builder
        .WithMonitor(MonitorIdentifier{"mon/voltage_sensor"})
        .WithEvent(EventIdentifier{"evt/voltage_out_of_range"})
        .WithClearCondition(ConditionIdentifier{"cond/ignition_on"})
        .ConfigureClearBehaviour(ClearBehaviour::kClearable)
        .ConfigureDebouncing(Debounce::TimeBased{
            std::chrono::milliseconds{200},
            std::chrono::milliseconds{100}})
        .Build();
```

All three `With*()` calls are required. `Build()` must be called exactly once.

---

## 8. Error Handling

All service methods return `Result<T>`. The runtime translates a returned
`NegativeResponseCode` into a standard UDS negative response
`0x7F <SID> <NRC>` to the tester.

### Standard NRC Reference

| NRC | Value | When to Use |
| :--- | :--- | :--- |
| `GeneralReject` | 0x10 | Generic failure with no more specific code |
| `ServiceNotSupported` | 0x11 | The service SID is not implemented |
| `SubFunctionNotSupported` | 0x12 | Sub-function byte is not implemented |
| `IncorrectMessageLengthOrInvalidFormat` | 0x13 | Payload size or structure is wrong |
| `ConditionsNotCorrect` | 0x22 | Precondition not met (wrong session, engine running, etc.) |
| `RequestOutOfRange` | 0x31 | DID or RID not supported, or value out of valid range |
| `SecurityAccessDenied` | 0x33 | Insufficient security level |
| `GeneralProgrammingFailure` | 0x72 | NVM write or hardware operation failed |
| `ServiceNotSupportedInActiveSession` | 0x7F | Valid service, but not in this session |

### Vendor-Specific NRCs (0xF0–0xFE)

```cpp
#include "score/mw/diag/uds/negative_response_code.h"

// Compile-time construction — out-of-range is a compile error
const auto nrc = score::mw::diag::uds::VehicleManufacturerSpecificCNC::FromValue<0xF2U>();

// Runtime construction — returns std::nullopt when out of range
auto maybe = score::mw::diag::uds::VehicleManufacturerSpecificCNC::FromValue(value);
if (!maybe.has_value()) {
    return score::MakeUnexpected(NegativeResponseCode::ConditionsNotCorrect);
}
return score::MakeUnexpected(static_cast<NegativeResponseCode>(*maybe));
```

### Common Failure Scenarios

| Scenario | Condition | NRC |
| :--- | :--- | :--- |
| Invalid payload size | `input.size()` ≠ expected | `0x13` `IncorrectMessageLengthOrInvalidFormat` |
| Unknown DID / RID | Not registered or not recognised | `0x31` `RequestOutOfRange` |
| Wrong diagnostic session | Session too low for the operation | `0x7F` `ServiceNotSupportedInActiveSession` |
| Security not unlocked | Missing or insufficient security level | `0x33` `SecurityAccessDenied` |
| Precondition not met | Engine running, vehicle moving, etc. | `0x22` `ConditionsNotCorrect` |
| NVM write failure | Persistent storage write failed | `0x72` `GeneralProgrammingFailure` |
| DTC report ignored | Storage inhibited | `Result<void>` with error |

---

## 9. Request / Response Lifecycle

### UDS Request → Response

```text
Tester / ECU Tool         Diagnostic Runtime             Your Handler
      │                          │                             │
      │──── 0x22 <DID> ─────────>│                             │
      │                          │──── Read(meta, stop) ──────>│
      │                          │<─── Result<ByteVector> ─────│
      │<─── 0x62 <DID> <data> ───│                             │
      │                                                         │
      │──── 0x2E <DID> <bad> ────>│                             │
      │                          │──── Write(input, meta) ────>│
      │                          │<─── Err(0x13) ──────────────│
      │<─── 0x7F 0x2E 0x13 ──────│                             │
```

### DTC Lifecycle

```text
ECU Power-On
    │
    ├─► runtime fires OnInit(kRestart) ──► app resets debounce state
    │
    ├─► app calls Report(kFailed) every cycle
    │        │
    │        └─► debounce threshold reached ──► DTC stored
    │
    ├─► tester reads via 0x19 (ReadDTCInformation)
    │
    ├─► tester clears via 0x14 (ClearDiagnosticInformation)
    │        │
    │        └─► runtime fires OnInit(kClear) ──► app resets debounce state
    │
    └─► monitoring continues...
```

---

## 10. Best Practices

### Handler Implementation

| Practice | Reason |
| :--- | :--- |
| Prefer `Simple*` unless the operation may block | The runtime calls handlers on its thread; blocking stalls all diagnostics |
| Always mark overrides `[[nodiscard]]` | Ensures `Result<T>` values are not silently discarded |
| Validate `input.size()` before accessing bytes | Prevents undefined behaviour on malformed requests |
| Never store `ByteView` beyond the handler's scope | The runtime owns the underlying buffer |
| Do not throw exceptions inside handlers | Always communicate failures via `Result` |

### DTC Reporting

| Practice | Reason |
| :--- | :--- |
| Call `Report()` **every** monitoring cycle | The runtime's debouncer needs both `kPassed` and `kFailed` to track transitions |
| Always register an `OnInit` callback | Reset internal debounce state on restart, clear, and re-enable |
| Use `kNotClearable` only for safety-critical DTCs | Tester clear is ignored — use deliberately |
| Use `kReenterAfterCleared` only for permanent faults | DTC immediately re-enters storage after a tester clear |
| Check the `Result<void>` returned by `Report()` | Storage may be inhibited; the runtime signals this via the error |

### Thread Safety

- Consult your platform runtime documentation for threading guarantees on
  handler calls.
- Do not share a single `DTC` instance across threads without external
  synchronisation.

---

## 11. Key Headers

| Header | Contents |
| :--- | :--- |
| `score/mw/diag/byte_types.h` | `ByteVector` (owning) and `ByteView` (non-owning span) |
| `score/mw/diag/diag_result.h` | `Result<T>` alias (`score::Result<T>`) in `score::mw::diag::uds` |
| `score/mw/diag/uds/negative_response_code.h` | `NegativeResponseCode` enum (ISO 14229-1:2020 Table A.1) and `RangedNrc<>` |
| `score/mw/diag/uds/meta_data.h` | `MetaData`, `DiagnosticSession`, `AddressingMode` |
| `score/mw/diag/uds/read_data_by_identifier.h` | `ReadDataByIdentifier`, `SimpleReadDataByIdentifier` |
| `score/mw/diag/uds/write_data_by_identifier.h` | `WriteDataByIdentifier`, `SimpleWriteDataByIdentifier` |
| `score/mw/diag/uds/generic_data_identifier.h` | `GenericDataIdentifier`, `SimpleGenericDataIdentifier` |
| `score/mw/diag/uds/routine_control.h` | `RoutineControl`, `SimpleRoutineControl` |
| `score/mw/diag/uds/generic_service.h` | `GenericService`, `SimpleGenericService` |
| `score/mw/diag/dtc/dtc.h` | `DTC` interface, `Status`, `ClearBehaviour`, `InitReason`, `FormatType` |
| `score/mw/diag/dtc/identifier.h` | `MonitorIdentifier`, `EventIdentifier`, `ConditionIdentifier` |
| `score/mw/diag/dtc/debounce.h` | `Debounce` with `TimeBased` and `CounterBased` variants |
