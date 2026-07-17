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

// ── DiagnosticServicesCollectionBuilder — duplicate identifier failure paths ──

TEST(DiagnosticServicesCollectionBuilderTest, DuplicateIdentifierWithinSameTypeIsRejected)
{
    DiagnosticServicesCollectionBuilder builder{};
    builder.WithReadDid("F190", std::make_unique<test::ReadDataByIdentifierMock>());
    builder.WithReadDid("F190", std::make_unique<test::ReadDataByIdentifierMock>());

    auto result = builder.Build();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
}

TEST(DiagnosticServicesCollectionBuilderTest, DuplicateIdentifierAcrossHandlerTypesIsRejected)
{
    // FIX: Builder now correctly rejects duplicate identifiers at build time.
    DiagnosticServicesCollectionBuilder builder{};
    builder.WithReadDid("F190", std::make_unique<test::ReadDataByIdentifierMock>());
    builder.WithWriteDid("F190", std::make_unique<test::WriteDataByIdentifierMock>());

    auto result = builder.Build();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), NegativeResponseCode::ConditionsNotCorrect);
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

// ── DiagnosticServicesCollection — Public Get API Verification ────────────────

TEST(DiagnosticServicesCollectionBuilderTest, PublicGetAPIsExposeRegisteredHandlers)
{
    // FIX: Added validation tests to verify structural transparency via public const accessors
    DiagnosticServicesCollectionBuilder builder{};
    builder.WithReadDid("F190", std::make_unique<test::ReadDataByIdentifierMock>())
        .WithWriteDid("F191", std::make_unique<test::WriteDataByIdentifierMock>())
        .WithDataId("F192", std::make_unique<test::GenericDataIdentifierMock>())
        .WithRoutine("0301", std::make_unique<test::RoutineControlMock>())
        .WithGenericService("B200", std::make_unique<test::GenericServiceMock>());

    auto result = builder.Build();
    ASSERT_TRUE(result.has_value());
    const auto& collection = *result;

    ASSERT_EQ(collection->GetReadDids().size(), 1U);
    EXPECT_EQ(collection->GetReadDids()[0].first, "F190");
    EXPECT_NE(collection->GetReadDids()[0].second, nullptr);

    ASSERT_EQ(collection->GetWriteDids().size(), 1U);
    EXPECT_EQ(collection->GetWriteDids()[0].first, "F191");
    EXPECT_NE(collection->GetWriteDids()[0].second, nullptr);

    ASSERT_EQ(collection->GetGenericDataIds().size(), 1U);
    EXPECT_EQ(collection->GetGenericDataIds()[0].first, "F192");
    EXPECT_NE(collection->GetGenericDataIds()[0].second, nullptr);

    ASSERT_EQ(collection->GetRoutines().size(), 1U);
    EXPECT_EQ(collection->GetRoutines()[0].first, "0301");
    EXPECT_NE(collection->GetRoutines()[0].second, nullptr);

    ASSERT_EQ(collection->GetGenericServices().size(), 1U);
    EXPECT_EQ(collection->GetGenericServices()[0].first, "B200");
    EXPECT_NE(collection->GetGenericServices()[0].second, nullptr);
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

// ── In-place construction overloads ─────────────────────────────────────────

namespace
{

struct ConcreteReadHandler final : public ReadDataByIdentifier
{
    explicit ConcreteReadHandler(int) {}
    Result<ByteVector> Read() override { return ByteVector{}; }
};

struct ConcreteWriteHandler final : public WriteDataByIdentifier
{
    explicit ConcreteWriteHandler(int) {}
    Result<score::cpp::blank> Write(ByteView) override { return score::cpp::blank{}; }
};

struct ConcreteDataIdHandler final : public GenericDataIdentifier
{
    explicit ConcreteDataIdHandler(int) {}
    Result<ByteVector> Read() override { return ByteVector{}; }
    Result<score::cpp::blank> Write(ByteView) override { return score::cpp::blank{}; }
};

struct ConcreteRoutineHandler final : public RoutineControl
{
    explicit ConcreteRoutineHandler(int) {}
    Result<ByteVector> Start(ByteView) override { return ByteVector{}; }
    Result<ByteVector> Stop(ByteView) override { return ByteVector{}; }
    Result<ByteVector> RequestResults(ByteView) override { return ByteVector{}; }
};

struct ConcreteGenericService final : public GenericService
{
    explicit ConcreteGenericService(int) {}
    Result<ByteVector> HandleMessage(ByteView) override { return ByteVector{}; }
};

}  // namespace

TEST(DiagnosticServicesCollectionBuilderTest, InPlaceWithReadDidSucceeds)
{
    DiagnosticServicesCollectionBuilder builder;
    builder.WithReadDid<ConcreteReadHandler>("F190", 42);
    EXPECT_TRUE(builder.Build().has_value());
}

TEST(DiagnosticServicesCollectionBuilderTest, InPlaceWithWriteDidSucceeds)
{
    DiagnosticServicesCollectionBuilder builder;
    builder.WithWriteDid<ConcreteWriteHandler>("F190", 42);
    EXPECT_TRUE(builder.Build().has_value());
}

TEST(DiagnosticServicesCollectionBuilderTest, InPlaceWithDataIdSucceeds)
{
    DiagnosticServicesCollectionBuilder builder;
    builder.WithDataId<ConcreteDataIdHandler>("F190", 42);
    EXPECT_TRUE(builder.Build().has_value());
}

TEST(DiagnosticServicesCollectionBuilderTest, InPlaceWithRoutineSucceeds)
{
    DiagnosticServicesCollectionBuilder builder;
    builder.WithRoutine<ConcreteRoutineHandler>("0301", 42);
    EXPECT_TRUE(builder.Build().has_value());
}

TEST(DiagnosticServicesCollectionBuilderTest, InPlaceWithGenericServiceSucceeds)
{
    DiagnosticServicesCollectionBuilder builder;
    builder.WithGenericService<ConcreteGenericService>("B200", 42);
    EXPECT_TRUE(builder.Build().has_value());
}

TEST(DiagnosticServicesCollectionBuilderTest, InPlaceAndUniquePtrOverloadsMixSucceeds)
{
    DiagnosticServicesCollectionBuilder builder;
    builder.WithReadDid<ConcreteReadHandler>("F190", 1)
        .WithWriteDid("F191", std::make_unique<test::WriteDataByIdentifierMock>())
        .WithRoutine<ConcreteRoutineHandler>("0301", 2);
    EXPECT_TRUE(builder.Build().has_value());
}

}  // namespace score::mw::diag::uds
