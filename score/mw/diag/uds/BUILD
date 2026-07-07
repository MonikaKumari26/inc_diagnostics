# *****************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *****************************************************************************

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

# ---------------------------------------------------------------------------
# Libraries
# ---------------------------------------------------------------------------

# Kept as a standalone target: diag_result depends on this, so merging
# it into :uds would create a circular dependency (uds → diag_result → uds).
cc_library(
    name = "negative_response_code",
    hdrs = ["negative_response_code.h"],
    srcs = ["negative_response_code.cpp"],
    visibility = ["//score:__subpackages__"],
)

cc_library(
    name = "uds",
    hdrs = [
        "generic_data_identifier.h",
        "generic_service.h",
        "read_data_by_identifier.h",
        "routine_control.h",
        "write_data_by_identifier.h",
    ],
    srcs = ["uds.cpp"],
    visibility = ["//score:__subpackages__"],
    deps = [
        ":negative_response_code",
        "//score/mw/diag:byte_types",
        "//score/mw/diag:diag_result",
        "@score_baselibs//score/language/futurecpp",
    ],
)

# ---------------------------------------------------------------------------
# Mock libraries (testonly)
# ---------------------------------------------------------------------------

cc_library(
    name = "mocks",
    testonly = True,
    hdrs = [
        "generic_data_identifier_mock.h",
        "generic_service_mock.h",
        "read_data_by_identifier_mock.h",
        "routine_control_mock.h",
        "write_data_by_identifier_mock.h",
    ],
    visibility = ["//score:__subpackages__"],
    deps = [
        ":uds",
        "@googletest//:gtest",
    ],
)

# ---------------------------------------------------------------------------
# Unit tests
# ---------------------------------------------------------------------------

cc_test(
    name = "negative_response_code_test",
    srcs = ["negative_response_code_test.cpp"],
    tags = ["unit"],
    deps = [
        ":negative_response_code",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "routine_control_test",
    srcs = ["routine_control_test.cpp"],
    tags = ["unit"],
    deps = [
        ":uds",
        "@googletest//:gtest_main",
    ],
)

test_suite(
    name = "unit_tests",
    tests = [
        ":negative_response_code_test",
        ":routine_control_test",
    ],
    visibility = ["//score/mw/diag:__pkg__"],
)
