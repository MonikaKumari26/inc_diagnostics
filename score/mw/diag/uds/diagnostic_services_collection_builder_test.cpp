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

/// @file diagnostic_services_collection_builder_test.cpp
/// @brief Unit tests for DiagnosticJobCollection, DiagnosticServicesCollection, and
///        DiagnosticServicesCollectionBuilder.

#include "score/mw/diag/uds/diagnostic_services_collection_builder.h"

#include "score/mw/diag/uds/diagnostic_job_collection_mock.h"
#include "score/mw/diag/uds/generic_data_identifier_mock.h"
#include "score/mw/diag/uds/generic_service_mock.h"
#include "score/mw/diag/uds/read_data_by_identifier_mock.h"
#include "score/mw/diag/uds/routine_control_mock.h"
#include "score/mw/diag/uds/write_data_by_identifier_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

namespace score::mw::diag::uds
{

// ── DiagnosticJobCollection ──────────────────────────────────────────────────

TEST(DiagnosticJobCollectionTest, VirtualDestructorFiresThroughBasePointer)
{
    // Verify that releasing a base-class unique_ptr correctly dispatches
    // to the derived destructor (proves the vtable is set up correctly).
    auto mock = std::make_unique<test::DiagnosticJobCollectionMock>();
    EXPECT_CALL(*mock, Destruct());

    std::unique_ptr<DiagnosticJobCollection> base = std::move(mock);
    base.reset();  // ~DiagnosticJobCollectionMock() must fire here
}

// ── DiagnosticServicesCollectionBuilder — successful build paths ─────────────

TEST(DiagnosticServicesCollectionBuilderTest, BuildEmptyCollectionSucceeds)
{
    DiagnosticServicesCollectionBuilder builder{};

    auto result = builder.Build();

    ASSERT_TRUE(result.has_value());
    EXPECT_NE(result->get(), nullptr);
}

TEST(DiagnosticServicesCollectionBuilderTest, BuildWithReadDidSucceeds)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.WithReadDid("F190", std::make_unique<test::ReadDataByIdentifierMock>());

    auto result = builder.Build();

    EXPECT_TRUE(result.has_value());
}

TEST(DiagnosticServicesCollectionBuilderTest, BuildWithWriteDidSucceeds)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.WithWriteDid("F190", std::make_unique<test::WriteDataByIdentifierMock>());

    auto result = builder.Build();

    EXPECT_TRUE(result.has_value());
}

TEST(DiagnosticServicesCollectionBuilderTest, BuildWithDataIdSucceeds)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.WithDataId("F190", std::make_unique<test::GenericDataIdentifierMock>());

    auto result = builder.Build();

    EXPECT_TRUE(result.has_value());
}

TEST(DiagnosticServicesCollectionBuilderTest, BuildWithRoutineSucceeds)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.WithRoutine("0301", std::make_unique<test::RoutineControlMock>());

    auto result = builder.Build();

    EXPECT_TRUE(result.has_value());
}

TEST(DiagnosticServicesCollectionBuilderTest, BuildWithGenericServiceSucceeds)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.WithGenericService("B200", std::make_unique<test::GenericServiceMock>());

    auto result = builder.Build();

    EXPECT_TRUE(result.has_value());
}

TEST(DiagnosticServicesCollectionBuilderTest, BuildWithAllHandlerTypesCombinedSucceeds)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.WithReadDid("F190", std::make_unique<test::ReadDataByIdentifierMock>())
        .WithWriteDid("F191", std::make_unique<test::WriteDataByIdentifierMock>())
        .WithDataId("F192", std::make_unique<test::GenericDataIdentifierMock>())
        .WithRoutine("0301", std::make_unique<test::RoutineControlMock>())
        .WithGenericService("B200", std::make_unique<test::GenericServiceMock>());

    auto result = builder.Build();

    EXPECT_TRUE(result.has_value());
}

TEST(DiagnosticServicesCollectionBuilderTest, MultipleHandlersPerTypeSucceed)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.WithReadDid("F190", std::make_unique<test::ReadDataByIdentifierMock>());
    builder.WithReadDid("F191", std::make_unique<test::ReadDataByIdentifierMock>());
    builder.WithRoutine("0301", std::make_unique<test::RoutineControlMock>());
    builder.WithRoutine("0302", std::make_unique<test::RoutineControlMock>());

    auto result = builder.Build();

    EXPECT_TRUE(result.has_value());
}

TEST(DiagnosticServicesCollectionBuilderTest, DuplicateIdentifierAcrossHandlerTypesIsAccepted)
{
    // The builder does not validate duplicate identifiers across handler types.
    // Dispatch ambiguity is the ServiceRegistrar's responsibility (see class doc).
    DiagnosticServicesCollectionBuilder builder{};
    builder.WithReadDid("F190", std::make_unique<test::ReadDataByIdentifierMock>());
    builder.WithWriteDid("F190", std::make_unique<test::WriteDataByIdentifierMock>());
    builder.WithDataId("F190", std::make_unique<test::GenericDataIdentifierMock>());

    auto result = builder.Build();

    EXPECT_TRUE(result.has_value());
}

// ── DiagnosticServicesCollectionBuilder — null-handler failure paths ──────────

TEST(DiagnosticServicesCollectionBuilderTest, BuildWithNullReadDidFails)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.WithReadDid("F190", nullptr);

    auto result = builder.Build();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

TEST(DiagnosticServicesCollectionBuilderTest, BuildWithNullWriteDidFails)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.WithWriteDid("F190", nullptr);

    auto result = builder.Build();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

TEST(DiagnosticServicesCollectionBuilderTest, BuildWithNullDataIdFails)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.WithDataId("F190", nullptr);

    auto result = builder.Build();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

TEST(DiagnosticServicesCollectionBuilderTest, BuildWithNullRoutineFails)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.WithRoutine("0301", nullptr);

    auto result = builder.Build();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

TEST(DiagnosticServicesCollectionBuilderTest, BuildWithNullGenericServiceFails)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.WithGenericService("B200", nullptr);

    auto result = builder.Build();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

TEST(DiagnosticServicesCollectionBuilderTest, NullHandlerMixedWithValidHandlerFails)
{
    // A single null entry among multiple valid handlers should still fail.
    DiagnosticServicesCollectionBuilder builder{};
    builder.WithReadDid("F190", std::make_unique<test::ReadDataByIdentifierMock>())
        .WithReadDid("F191", nullptr);

    auto result = builder.Build();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

// ── DiagnosticServicesCollectionBuilder — post-build state ───────────────────

TEST(DiagnosticServicesCollectionBuilderTest, BuildResultIsUsableAsDiagnosticJobCollection)
{
    // Verify the production use case: the runtime stores the result as a
    // unique_ptr<DiagnosticJobCollection> base pointer and destructs through it.
    DiagnosticServicesCollectionBuilder builder{};
    builder.WithReadDid("F190", std::make_unique<test::ReadDataByIdentifierMock>());

    auto result = builder.Build();
    ASSERT_TRUE(result.has_value());

    // Implicit upcast — must compile and destruct cleanly through the base pointer.
    std::unique_ptr<DiagnosticJobCollection> base = std::move(*result);
    base.reset();
}

TEST(DiagnosticServicesCollectionBuilderTest, BuildClearsBuilderState)
{
    // After a successful Build() the builder vectors are moved-from (empty).
    // A second Build() call should therefore also succeed and return an
    // empty-but-valid collection.
    DiagnosticServicesCollectionBuilder builder{};
    builder.WithReadDid("F190", std::make_unique<test::ReadDataByIdentifierMock>());

    auto result1 = builder.Build();
    ASSERT_TRUE(result1.has_value());

    auto result2 = builder.Build();  // builder is now empty
    EXPECT_TRUE(result2.has_value());
}

}  // namespace score::mw::diag::uds
