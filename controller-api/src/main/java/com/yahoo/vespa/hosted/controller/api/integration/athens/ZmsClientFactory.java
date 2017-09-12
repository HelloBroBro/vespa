// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.hosted.controller.api.integration.athens;

/**
 * @author bjorncs
 */
public interface ZmsClientFactory {
    ZmsClient createClientWithServicePrincipal();

    ZmsClient createClientWithAuthorizedServiceToken(NToken authorizedServiceToken);

    ZmsClient createClientWithoutPrincipal();
}
