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

/// @file diagnostic_job_collection.h
/// @brief DiagnosticJobCollection — lifetime handle for a registered set of
///        diagnostic service handlers.
///
/// As long as a DiagnosticJobCollection instance is alive the diagnostic service
/// implementations registered with the runtime remain active.  Destroying the
/// collection deregisters them.

#ifndef SCORE_MW_DIAG_DIAGNOSTIC_JOB_COLLECTION_H
#define SCORE_MW_DIAG_DIAGNOSTIC_JOB_COLLECTION_H

namespace score::mw::diag::uds
{

/// @brief Lifetime handle for a registered collection of diagnostic service handlers.
///
/// The diagnostic runtime provides an instance of this class (or a subclass) when
/// service registration succeeds.  The caller holds the returned
/// `unique_ptr<DiagnosticJobCollection>` alive for as long as the handlers must remain
/// registered; releasing the pointer deregisters all associated services.
///
/// @note This class has no public business methods by design — the object's
///       lifetime IS the mechanism: constructing it activates the handlers,
///       destroying it deactivates them (RAII lifetime pattern).
class DiagnosticJobCollection
{
  public:
    DiagnosticJobCollection() = default;
    virtual ~DiagnosticJobCollection() noexcept = default;

  protected:
    DiagnosticJobCollection(const DiagnosticJobCollection&) = default;
    DiagnosticJobCollection(DiagnosticJobCollection&&) noexcept = default;
    DiagnosticJobCollection& operator=(const DiagnosticJobCollection&) & = default;
    DiagnosticJobCollection& operator=(DiagnosticJobCollection&&) & noexcept = default;
};

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_DIAGNOSTIC_JOB_COLLECTION_H
