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
# Common type aliases
# ---------------------------------------------------------------------------

cc_library(
    name = "byte_types",
    srcs = ["byte_types.cpp"],
    hdrs = ["byte_types.h"],
    visibility = ["//score:__subpackages__"],
    deps = [
        "@score_baselibs//score/language/futurecpp",
    ],
)

cc_library(
    name = "diag_result",
    srcs = ["diag_result.cpp"],
    hdrs = ["diag_result.h"],
    visibility = ["//score:__subpackages__"],
    deps = [
        "@score_baselibs//score/result",
    ],
)

cc_library(
    name = "future",
    hdrs = ["future.h"],
    visibility = ["//score:__subpackages__"],
    deps = [
        "@score_baselibs//score/concurrency/future",
    ],
)

# ---------------------------------------------------------------------------
# Test aggregation
# ---------------------------------------------------------------------------

test_suite(
    name = "unit_tests",
    tests = [
        "//score/mw/diag/uds:unit_tests",
    ],
    visibility = ["//score/mw:__pkg__"],
)
