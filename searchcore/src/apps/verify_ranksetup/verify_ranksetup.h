// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <vespa/searchlib/fef/verify_feature.h>

enum class SearchMode { INDEXED, STREAMING };

std::pair<bool, std::vector<search::fef::Message>> verifyRankSetup(const char * configId, SearchMode mode);
