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

/// @file storage_guard.h
/// @brief Declares the StorageGuard class.

#ifndef SCORE_MW_DIAG_DTC_STORAGE_GUARD_H
#define SCORE_MW_DIAG_DTC_STORAGE_GUARD_H

#include <atomic>

namespace score::mw::diag::dtc
{

/// @brief Concrete guard for controlling DTC storage inhibition.
///
/// Backed by a std::atomic<bool> — safe for concurrent access from different threads
/// (e.g. a tester thread calling DoNotAllowStoringDtcs() while the application thread
/// checks IsStorageAllowed() before writing to NVM).
/// By default, storage is allowed. Call DoNotAllowStoringDtcs() to inhibit it.
///
/// Inhibiting storage prevents new DTC entries from being written to NVM
/// without affecting already-stored entries or active DTC monitoring.
class StorageGuard final
{
  public:
    /// @brief Default: DTC storage is allowed.
    StorageGuard() = default;

    /// @brief Inhibit DTC storage — new entries will not be written until AllowStoringDtcs() is called.
    ///        Safe to call from any thread; uses release semantics so any subsequent
    ///        IsStorageAllowed() call on another thread is guaranteed to observe the inhibition.
    void DoNotAllowStoringDtcs() noexcept
    {
        storage_allowed_.store(false, std::memory_order_release);
    }

    /// @brief Re-enable DTC storage after a prior DoNotAllowStoringDtcs() call.
    ///        Safe to call from any thread; uses release semantics so any subsequent
    ///        IsStorageAllowed() call on another thread is guaranteed to observe the re-enablement.
    void AllowStoringDtcs() noexcept { storage_allowed_.store(true, std::memory_order_release); }

    /// @brief Returns true if DTC storage is currently allowed.
    ///        Safe to call from any thread; uses acquire semantics so it observes the latest
    ///        store from DoNotAllowStoringDtcs() or AllowStoringDtcs().
    /// @return true if storage is enabled; false if inhibited.
    [[nodiscard]] bool IsStorageAllowed() const noexcept
    {
        return storage_allowed_.load(std::memory_order_acquire);
    }

    StorageGuard(const StorageGuard&) = delete;
    StorageGuard(StorageGuard&&) noexcept = delete;
    StorageGuard& operator=(const StorageGuard&) = delete;
    StorageGuard& operator=(StorageGuard&&) noexcept = delete;
    ~StorageGuard() noexcept = default;

  private:
    std::atomic<bool> storage_allowed_{true};
};

}  // namespace score::mw::diag::dtc

#endif  // SCORE_MW_DIAG_DTC_STORAGE_GUARD_H
