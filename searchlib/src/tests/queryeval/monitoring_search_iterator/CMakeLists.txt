# Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
vespa_add_executable(searchlib_monitoring_search_iterator_test_app TEST
    SOURCES
    monitoring_search_iterator_test.cpp
    DEPENDS
    vespa_searchlib
    searchlib_test
)
vespa_add_test(NAME searchlib_monitoring_search_iterator_test_app COMMAND searchlib_monitoring_search_iterator_test_app)
