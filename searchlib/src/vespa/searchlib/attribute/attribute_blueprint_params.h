// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <vespa/searchlib/fef/indexproperties.h>
#include <vespa/vespalib/fuzzy/fuzzy_matching_algorithm.h>

namespace search::attribute {

/**
 * Parameters for attribute blueprints from rank profile and query.
 */
struct AttributeBlueprintParams
{
    double global_filter_lower_limit;
    double global_filter_upper_limit;
    double target_hits_max_adjustment_factor;
    vespalib::FuzzyMatchingAlgorithm fuzzy_matching_algorithm;
    double weakand_range;
    uint32_t abs_weakand_stop_word_limit;

    AttributeBlueprintParams(double global_filter_lower_limit_in,
                             double global_filter_upper_limit_in,
                             double target_hits_max_adjustment_factor_in,
                             vespalib::FuzzyMatchingAlgorithm fuzzy_matching_algorithm_in,
                             double weakand_range_in,
                             uint32_t abs_wand_stop_word_limit_in)
        : global_filter_lower_limit(global_filter_lower_limit_in),
          global_filter_upper_limit(global_filter_upper_limit_in),
          target_hits_max_adjustment_factor(target_hits_max_adjustment_factor_in),
          fuzzy_matching_algorithm(fuzzy_matching_algorithm_in),
          weakand_range(weakand_range_in),
          abs_weakand_stop_word_limit(abs_wand_stop_word_limit_in)
    {
    }

    AttributeBlueprintParams()
        : AttributeBlueprintParams(fef::indexproperties::matching::GlobalFilterLowerLimit::DEFAULT_VALUE,
                                   fef::indexproperties::matching::GlobalFilterUpperLimit::DEFAULT_VALUE,
                                   fef::indexproperties::matching::TargetHitsMaxAdjustmentFactor::DEFAULT_VALUE,
                                   fef::indexproperties::matching::FuzzyAlgorithm::DEFAULT_VALUE,
                                   fef::indexproperties::temporary::WeakAndRange::DEFAULT_VALUE,
                                   fef::indexproperties::matching::WeakAndStopWordLimit::DEFAULT_VALUE)
    {
    }
};

}
