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

/// @file diag_result.h
/// @brief Result<T> type alias for the diagnostic API.
///
/// The error type is NegativeResponseCode.
/// For SOVD support a new common error type can be added to score::mw::diag
/// without touching existing UDS code that uses this alias.

#ifndef SCORE_MW_DIAG_DIAG_RESULT_H
#define SCORE_MW_DIAG_DIAG_RESULT_H

<<<<<<< HEAD
=======
#include "score/mw/diag/byte_types.h"
#include "score/mw/diag/uds/negative_response_code.h"

>>>>>>> 71e6d8e (replace deprecated score::cpp::blank with Result<void>)
#include "score/result/result.h"

namespace score::mw::diag::uds
{

/// Result type: either a success value T or a score::result::Error with NegativeResponseCode as the error type.
template <typename T>
<<<<<<< HEAD
<<<<<<< HEAD
using Result = score::Result<T>;
=======
using Result = score::cpp::expected<T, NegativeResponseCode>;
>>>>>>> ff4464f (removed redundant members, added cpp files for byte_types and diag_result)
=======
using Result = score::Result<T>;
>>>>>>> 71e6d8e (replace deprecated score::cpp::blank with Result<void>)

}  // namespace score::mw::diag::uds

#endif  // SCORE_MW_DIAG_DIAG_RESULT_H
