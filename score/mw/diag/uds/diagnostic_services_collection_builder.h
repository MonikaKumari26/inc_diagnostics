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

/// @file diagnostic_services_collection_builder.h
/// @brief DiagnosticServicesCollectionBuilder and DiagnosticServicesCollection for
///        bundling UDS diagnostic service handlers before runtime registration.

#ifndef SCORE_MW_DIAG_UDS_DIAGNOSTIC_SERVICES_COLLECTION_BUILDER_H
#define SCORE_MW_DIAG_UDS_DIAGNOSTIC_SERVICES_COLLECTION_BUILDER_H

#include "score/mw/diag/diag_result.h"
#include "score/mw/diag/uds/diagnostic_job_collection.h"
#include "score/mw/diag/uds/generic_data_identifier.h"
#include "score/mw/diag/uds/generic_service.h"
#include "score/mw/diag/uds/read_data_by_identifier.h"
#include "score/mw/diag/uds/routine_control.h"
#include "score/mw/diag/uds/write_data_by_identifier.h"

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace score::mw::diag::uds
{

/************************************/
/* DiagnosticServicesCollection     */
/************************************/

/// Concrete service bundle returned by DiagnosticServicesCollectionBuilder::Build().
///
/// Extends DiagnosticJobCollection with ownership of the registered UDS handlers.
/// Callers hold a `unique_ptr<DiagnosticServicesCollection>` to keep the handlers
/// alive; destroying it signals that the handlers should no longer be active.
///
/// The runtime binding layer (ServiceRegistrar) consumes the collection through
/// the read-only accessors and dispatches incoming requests to the correct handler internally.
///
/// @note This collection is a **passive container** — `Build()` does not activate (offer)
///       the handlers. The caller must pass the result to the ServiceRegistrar to make
///       the handlers reachable by the diagnostic runtime.
///
/// Instances are only constructible through DiagnosticServicesCollectionBuilder::Build().
class DiagnosticServicesCollection final : public DiagnosticJobCollection
{
  public:
    ~DiagnosticServicesCollection() noexcept override = default;

    DiagnosticServicesCollection(const DiagnosticServicesCollection&) = delete;
    DiagnosticServicesCollection(DiagnosticServicesCollection&&) noexcept = delete;
    DiagnosticServicesCollection& operator=(const DiagnosticServicesCollection&) = delete;
    DiagnosticServicesCollection& operator=(DiagnosticServicesCollection&&) noexcept = delete;

  private:
    friend class DiagnosticServicesCollectionBuilder;

    DiagnosticServicesCollection() noexcept = default;

    std::vector<std::pair<std::string, std::unique_ptr<ReadDataByIdentifier>>> read_dids_;
    std::vector<std::pair<std::string, std::unique_ptr<WriteDataByIdentifier>>> write_dids_;
    std::vector<std::pair<std::string, std::unique_ptr<GenericDataIdentifier>>> data_ids_;
    std::vector<std::pair<std::string, std::unique_ptr<RoutineControl>>> routines_;
    std::vector<std::pair<std::string, std::unique_ptr<GenericService>>> generic_services_;
};

/****************************************/
/* DiagnosticServicesCollectionBuilder  */
/****************************************/

/// Fluent builder for assembling a set of UDS diagnostic service handlers.
///
/// Each With*() method takes ownership of the handler via `std::unique_ptr<BaseInterface>`.
///
/// The string identifier is stored as-is; duplicate identifiers across the same or different
/// handler types are **not** checked by the builder — the consuming ServiceRegistrar is
/// responsible for handling dispatch ambiguity if duplicates are registered.
/// Build() validates only that no stored handler pointer is nullptr.
///
/// The builder is non-copyable and non-movable. All With*() and Build() methods
/// are lvalue ref-qualified (`&`) — they must be called on a named variable, not
/// on a temporary.
class DiagnosticServicesCollectionBuilder final
{
  public:
    DiagnosticServicesCollectionBuilder() noexcept = default;

    /// Register a read-only DID handler (UDS Service 0x22).
    DiagnosticServicesCollectionBuilder& WithReadDid(std::string_view identifier,
                                                     std::unique_ptr<ReadDataByIdentifier> handler) &
    {
        read_dids_.emplace_back(std::string{identifier}, std::move(handler));
        return *this;
    }

    /// Register a write-only DID handler (UDS Service 0x2E).
    DiagnosticServicesCollectionBuilder& WithWriteDid(std::string_view identifier,
                                                      std::unique_ptr<WriteDataByIdentifier> handler) &
    {
        write_dids_.emplace_back(std::string{identifier}, std::move(handler));
        return *this;
    }

    /// Register a combined read+write DID handler (supports both 0x22 and 0x2E).
    DiagnosticServicesCollectionBuilder& WithDataId(std::string_view identifier,
                                                    std::unique_ptr<GenericDataIdentifier> handler) &
    {
        data_ids_.emplace_back(std::string{identifier}, std::move(handler));
        return *this;
    }

    /// Register a RoutineControl handler (UDS Service 0x31).
    DiagnosticServicesCollectionBuilder& WithRoutine(std::string_view identifier,
                                                     std::unique_ptr<RoutineControl> routine) &
    {
        routines_.emplace_back(std::string{identifier}, std::move(routine));
        return *this;
    }

    /// Register a raw UDS service handler for proprietary or vendor-specific services.
    DiagnosticServicesCollectionBuilder& WithGenericService(std::string_view identifier,
                                                            std::unique_ptr<GenericService> service) &
    {
        generic_services_.emplace_back(std::string{identifier}, std::move(service));
        return *this;
    }

    /// Validate and finalise the collection.
    ///
    /// Validation rule: no registered handler pointer may be nullptr.
    /// Duplicate identifiers are not rejected — see the class-level note.
    ///
    /// On success all handlers are moved into a new DiagnosticServicesCollection and
    /// the builder vectors are left empty (moved-from state).
    ///
    /// @note Calling Build() again after a successful call returns a valid but empty
    ///       DiagnosticServicesCollection without error, because the builder is empty.
    ///
    /// @return Ok(unique_ptr<DiagnosticServicesCollection>) on success.
    ///         Err(NegativeResponseCode::ConditionsNotCorrect) if any registered handler pointer is nullptr.
    [[nodiscard]] Result<std::unique_ptr<DiagnosticServicesCollection>> Build() &
    {
        if (HasNullEntry(read_dids_) || HasNullEntry(write_dids_) ||
            HasNullEntry(data_ids_) || HasNullEntry(routines_) || HasNullEntry(generic_services_))
        {
            return score::cpp::make_unexpected(NegativeResponseCode::ConditionsNotCorrect);
        }

        // Raw 'new' is necessary here because DiagnosticServicesCollection's default
        // constructor is private (only this builder may construct it); std::make_unique
        // cannot reach private constructors even from a friend class.
        auto collection = std::unique_ptr<DiagnosticServicesCollection>(new DiagnosticServicesCollection{});

        collection->read_dids_ = std::move(read_dids_);
        collection->write_dids_ = std::move(write_dids_);
        collection->data_ids_ = std::move(data_ids_);
        collection->routines_ = std::move(routines_);
        collection->generic_services_ = std::move(generic_services_);

        return collection;
    }

    DiagnosticServicesCollectionBuilder(const DiagnosticServicesCollectionBuilder&) = delete;
    DiagnosticServicesCollectionBuilder(DiagnosticServicesCollectionBuilder&&) noexcept = delete;
    DiagnosticServicesCollectionBuilder& operator=(const DiagnosticServicesCollectionBuilder&) = delete;
    DiagnosticServicesCollectionBuilder& operator=(DiagnosticServicesCollectionBuilder&&) noexcept = delete;
    ~DiagnosticServicesCollectionBuilder() noexcept = default;

  private:
    template <typename Container> [[nodiscard]] static bool HasNullEntry(const Container& container) noexcept
    {
        return std::any_of(container.begin(), container.end(),
                           [](const auto& handler_entry) { return handler_entry.second == nullptr; });
    }

    std::vector<std::pair<std::string, std::unique_ptr<ReadDataByIdentifier>>> read_dids_;
    std::vector<std::pair<std::string, std::unique_ptr<WriteDataByIdentifier>>> write_dids_;
    std::vector<std::pair<std::string, std::unique_ptr<GenericDataIdentifier>>> data_ids_;
    std::vector<std::pair<std::string, std::unique_ptr<RoutineControl>>> routines_;
    std::vector<std::pair<std::string, std::unique_ptr<GenericService>>> generic_services_;
};

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_UDS_DIAGNOSTIC_SERVICES_COLLECTION_BUILDER_H
