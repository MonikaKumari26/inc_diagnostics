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

/// @file diagnostic_job_collection_mock.h
/// @brief GMock implementation of score::mw::diag::uds::DiagnosticJobCollection.
///
/// `DiagnosticJobCollectionMock` exposes a `Destruct()` mock method that is called
/// from the overridden destructor, allowing tests to verify that the collection is
/// released at the expected point in time.

#ifndef SCORE_MW_DIAG_UDS_DIAGNOSTIC_JOB_COLLECTION_MOCK_H
#define SCORE_MW_DIAG_UDS_DIAGNOSTIC_JOB_COLLECTION_MOCK_H

#include "score/mw/diag/uds/diagnostic_job_collection.h"

#include <gmock/gmock.h>

namespace score::mw::diag::uds
{

/// Mock for score::mw::diag::uds::DiagnosticJobCollection.
///
/// Because `DiagnosticJobCollection` has no pure-virtual business methods, the only
/// observable behaviour worth mocking is the object's lifetime.  `Destruct()` is
/// called unconditionally from `~DiagnosticJobCollectionMock()`, so tests can set
/// `EXPECT_CALL(*mock, Destruct())` to assert that the collection is destroyed
/// exactly when expected (e.g. when a `unique_ptr` goes out of scope).
///
/// ### Example
/// @code
///   auto mock = std::make_unique<DiagnosticJobCollectionMock>();
///   EXPECT_CALL(*mock, Destruct());
///   // Transfer ownership; assert Destruct() fires on release.
///   std::unique_ptr<DiagnosticJobCollection> base = std::move(mock);
///   base.reset();  // triggers Destruct()
/// @endcode
class DiagnosticJobCollectionMock : public DiagnosticJobCollection
{
  public:
    /// Called from the destructor — set expectations to verify lifetime.
    MOCK_METHOD(void, Destruct, ());

    ~DiagnosticJobCollectionMock() noexcept override { Destruct(); }
};

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_UDS_DIAGNOSTIC_JOB_COLLECTION_MOCK_H
