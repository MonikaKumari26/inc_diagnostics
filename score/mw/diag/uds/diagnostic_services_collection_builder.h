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
///
/// ### DiagnosticServicesCollection
/// A concrete subclass of DiagnosticJobCollection that owns a set of
/// ReadDataByIdentifier, WriteDataByIdentifier, GenericDataIdentifier and
/// RoutineControl implementations keyed by string service IDs.  Accessors allow
/// the runtime (or a ServiceRegistrar adapter) to retrieve individual handlers.
///
/// ### DiagnosticServicesCollectionBuilder
/// A fluent builder that accumulates handlers and produces a validated
/// DiagnosticServicesCollection via build().
///
/// ### Typical usage
/// @code
///   DiagnosticServicesCollectionBuilder builder;
///   builder.with_read_did("F190", std::make_unique<VinDid>())
///          .with_routine("0301", std::make_unique<EraseRoutine>());
///   auto result = builder.build();
///   if (result.has_value()) {
///       // pass *result to the runtime ServiceRegistrar
///   }
/// @endcode

#ifndef SCORE_MW_DIAG_DIAGNOSTIC_SERVICES_COLLECTION_BUILDER_H
#define SCORE_MW_DIAG_DIAGNOSTIC_SERVICES_COLLECTION_BUILDER_H

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

/// String-view alias used as the service identifier type in builder methods.
using Identifier = std::string_view;

/************************************/
/* DiagnosticServicesCollection     */
/************************************/

/// Concrete service bundle returned by DiagnosticServicesCollectionBuilder::build().
///
/// Extends DiagnosticJobCollection with ownership of the registered UDS handlers.
/// Callers hold a unique_ptr<DiagnosticServicesCollection> to keep the handlers
/// active; the runtime binding layer (ServiceRegistrar) takes the collection and
/// dispatches incoming requests to the correct handler internally.
///
/// Instances are only constructible through DiagnosticServicesCollectionBuilder::build().
class DiagnosticServicesCollection final : public DiagnosticJobCollection
{
  public:
    ~DiagnosticServicesCollection() noexcept override = default;

  private:
    friend class DiagnosticServicesCollectionBuilder;

    DiagnosticServicesCollection() noexcept = default;

    std::vector<std::pair<std::string, std::unique_ptr<ReadDataByIdentifier>>> read_dids_;
    std::vector<std::pair<std::string, std::unique_ptr<WriteDataByIdentifier>>> write_dids_;
    std::vector<std::pair<std::string, std::unique_ptr<GenericDataIdentifier>>> data_ids_;
    std::vector<std::pair<std::string, std::unique_ptr<RoutineControl>>> routines_;
    std::vector<std::pair<std::string, std::unique_ptr<GenericService>>> uds_services_;
};

/************************************/
/* DiagnosticServicesCollectionBuilder */
/************************************/

/// Fluent builder for assembling a set of UDS diagnostic service handlers.
///
/// Each with_*() method takes ownership of the handler via std::unique_ptr<BaseInterface>
///
/// The string @p identifier is stored for use by the future ServiceRegistrar / find_* layer.
/// build() validates that no stored service pointer is nullptr.
///
/// The builder is non-copyable and non-movable; all with_*() methods return an
/// lvalue reference to support method chaining on a named variable.
class DiagnosticServicesCollectionBuilder final
{
  public:
    DiagnosticServicesCollectionBuilder() noexcept = default;

    /// Register a read-only DID handler (UDS Service 0x22).
    /// @param identifier  Service identifier string (e.g. "F190") — stored for future lookup.
    /// @param handler     Heap-allocated handler — ownership transferred to the builder.
    DiagnosticServicesCollectionBuilder& with_read_did(Identifier identifier,
                                                       std::unique_ptr<ReadDataByIdentifier> handler) &
    {
        read_dids_.emplace_back(std::string{identifier}, std::move(handler));
        return *this;
    }

    /// Register a write-only DID handler (UDS Service 0x2E).
    /// @param identifier  Service identifier string.
    /// @param handler     Heap-allocated handler — ownership transferred to the builder.
    DiagnosticServicesCollectionBuilder& with_write_did(Identifier identifier,
                                                        std::unique_ptr<WriteDataByIdentifier> handler) &
    {
        write_dids_.emplace_back(std::string{identifier}, std::move(handler));
        return *this;
    }

    /// Register a combined read+write DID handler (supports both 0x22 and 0x2E).
    /// @param identifier  Service identifier string.
    /// @param handler     Heap-allocated handler — ownership transferred to the builder.
    DiagnosticServicesCollectionBuilder& with_data_id(Identifier identifier,
                                                      std::unique_ptr<GenericDataIdentifier> handler) &
    {
        data_ids_.emplace_back(std::string{identifier}, std::move(handler));
        return *this;
    }

    /// Register a RoutineControl handler (UDS Service 0x31).
    /// @param identifier  Service identifier string (e.g. "0301").
    /// @param routine     Heap-allocated handler — ownership transferred to the builder.
    DiagnosticServicesCollectionBuilder& with_routine(Identifier identifier,
                                                      std::unique_ptr<RoutineControl> routine) &
    {
        routines_.emplace_back(std::string{identifier}, std::move(routine));
        return *this;
    }

    /// Register a raw UDS service handler for proprietary or vendor-specific services
    /// not covered by the DID or RoutineControl categories.
    /// @param identifier  Service identifier string.
    /// @param service     Heap-allocated handler — ownership transferred to the builder.
    DiagnosticServicesCollectionBuilder& with_uds_service(Identifier identifier,
                                                          std::unique_ptr<GenericService> service) &
    {
        uds_services_.emplace_back(std::string{identifier}, std::move(service));
        return *this;
    }

    /// Validate and finalise the collection.
    ///
    /// Validation rule: no registered service pointer may be nullptr.
    ///
    /// On success all handlers are moved into a new DiagnosticServicesCollection and
    /// the builder vectors are left empty (moved-from state).
    ///
    /// @note Calling build() again after a successful call returns a valid but empty
    ///       DiagnosticServicesCollection without error, because the builder is empty.
    ///
    /// @return Ok(unique_ptr<DiagnosticServicesCollection>) on success.
    ///         Err(NegativeResponseCode::ConditionsNotCorrect) if any registered handler pointer is nullptr.
    [[nodiscard]] Result<std::unique_ptr<DiagnosticServicesCollection>> build() &
    {
        using ReturnType = Result<std::unique_ptr<DiagnosticServicesCollection>>;

        if (has_null_entry(read_dids_) || has_null_entry(write_dids_) || has_null_entry(data_ids_) ||
            has_null_entry(routines_) || has_null_entry(uds_services_))
        {
            return ReturnType{score::unexpect, NegativeResponseCode::ConditionsNotCorrect};
        }

        auto collection = std::unique_ptr<DiagnosticServicesCollection>(new DiagnosticServicesCollection{});

        collection->read_dids_ = std::move(read_dids_);
        collection->write_dids_ = std::move(write_dids_);
        collection->data_ids_ = std::move(data_ids_);
        collection->routines_ = std::move(routines_);
        collection->uds_services_ = std::move(uds_services_);

        return collection;
    }

    DiagnosticServicesCollectionBuilder(const DiagnosticServicesCollectionBuilder&) = delete;
    DiagnosticServicesCollectionBuilder(DiagnosticServicesCollectionBuilder&&) noexcept = delete;
    DiagnosticServicesCollectionBuilder& operator=(const DiagnosticServicesCollectionBuilder&) & = delete;
    DiagnosticServicesCollectionBuilder& operator=(DiagnosticServicesCollectionBuilder&&) & noexcept = delete;
    ~DiagnosticServicesCollectionBuilder() noexcept = default;

  private:
    template <typename Container>
    static bool has_null_entry(const Container& container) noexcept
    {
        return std::any_of(container.begin(), container.end(), [](const auto& entry) {
            return entry.second == nullptr;
        });
    }

    std::vector<std::pair<std::string, std::unique_ptr<ReadDataByIdentifier>>> read_dids_;
    std::vector<std::pair<std::string, std::unique_ptr<WriteDataByIdentifier>>> write_dids_;
    std::vector<std::pair<std::string, std::unique_ptr<GenericDataIdentifier>>> data_ids_;
    std::vector<std::pair<std::string, std::unique_ptr<RoutineControl>>> routines_;
    std::vector<std::pair<std::string, std::unique_ptr<GenericService>>> uds_services_;
};

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_DIAGNOSTIC_SERVICES_COLLECTION_BUILDER_H
