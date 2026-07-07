# *******************************************************************************
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
# *******************************************************************************

load("@rules_cc//cc:defs.bzl", "cc_library")

# ---------------------------------------------------------------------------
# Libraries
# ---------------------------------------------------------------------------

cc_library(
    name = "identifier",
    srcs = ["identifier.cpp"],
    hdrs = ["identifier.h"],
    deps = ["@score_baselibs//score/language/futurecpp"],
)

cc_library(
    name = "debounce",
    srcs = ["debounce.cpp"],
    hdrs = ["debounce.h"],
    visibility = ["//score:__subpackages__"],
    deps = ["@score_baselibs//score/language/futurecpp"],
)

cc_library(
    name = "dtc",
    srcs = ["dtc.cpp"],
    hdrs = ["dtc.h"],
    visibility = ["//score:__subpackages__"],
    deps = [
        "//score/mw/diag:diag_result",
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_library(
    name = "storage_guard",
    srcs = ["storage_guard.cpp"],
    hdrs = ["storage_guard.h"],
    visibility = ["//score:__subpackages__"],
)

cc_library(
    name = "builder",
    srcs = ["builder.cpp"],
    hdrs = ["builder.h"],
    visibility = ["//score:__subpackages__"],
    deps = [
        ":debounce",
        ":identifier",
        ":dtc",
    ],
)

# ---------------------------------------------------------------------------
# Mock libraries (testonly)
# ---------------------------------------------------------------------------

cc_library(
    name = "mocks",
    testonly = True,
    srcs = ["mocks.cpp"],
    hdrs = [
        "builder_mock.h",
        "dtc_mock.h",
    ],
    deps = [
        ":builder",
        ":dtc",
        "@googletest//:gtest",
    ],
)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

load("@rules_cc//cc:defs.bzl", "cc_test")

cc_test(
    name = "unit_test",
    timeout = "short",
    srcs = [
        "debounce_test.cpp",
        "identifier_test.cpp",
    ],
    tags = ["unit"],
    deps = [
        ":debounce",
        ":identifier",
        "@googletest//:gtest_main",
    ],
)

test_suite(
    name = "unit_tests",
    tests = [
        ":unit_test",
    ],
    visibility = ["//score/mw/diag:__pkg__"],
)
