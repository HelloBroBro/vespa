# Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
vespa_define_module(
    DEPENDS
    vespalog
    vespa_config
    vespalib
    vespa_fnet
    vespa_slobrok

    LIBS
    src/vespa/messagebus
    src/vespa/messagebus/network
    src/vespa/messagebus/routing
    src/vespa/messagebus/testlib

    APPS
    src/apps/printversion

    TESTS
    src/tests
)
