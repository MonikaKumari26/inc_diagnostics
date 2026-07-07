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

// This translation unit exists solely to ensure all UDS headers are compiled as
// part of a bazel build (not only via test targets), so that header errors are
// caught during a plain `bazel build` without requiring a test run.
#include "score/mw/diag/uds/generic_data_identifier.h"
#include "score/mw/diag/uds/generic_service.h"
#include "score/mw/diag/uds/read_data_by_identifier.h"
#include "score/mw/diag/uds/routine_control.h"
#include "score/mw/diag/uds/write_data_by_identifier.h"
