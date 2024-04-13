// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.model.admin.otel;

import com.yahoo.cloud.config.OpenTelemetryConfig;
import com.yahoo.config.model.ApplicationConfigProducerRoot;
import com.yahoo.config.model.producer.TreeConfigProducer;
import com.yahoo.vespa.model.AbstractService;
import com.yahoo.vespa.model.PortAllocBridge;

import com.yahoo.cloud.config.ModelConfig;
import com.yahoo.config.model.producer.AnyConfigProducer;

import java.util.Optional;

public class OpenTelemetryCollector extends AbstractService implements OpenTelemetryConfig.Producer {

    public OpenTelemetryCollector(TreeConfigProducer<?> parent) {
        super(parent, "otelcol");
        setProp("clustertype", "admin");
        setProp("clustername", "admin");
    }

    /**
     * @return the startup command for the otelcol wrapper
     */
    @Override
    public Optional<String> getStartupCommand() {
        return Optional.of("exec $ROOT/bin/vespa-otelcol-start -c " + getConfigId());
    }

    @Override
    public void allocatePorts(int start, PortAllocBridge from) {}

    @Override
    public int getPortCount() {
        return 0;
    }

    @Override
    public void getConfig(OpenTelemetryConfig.Builder builder) {
        var generator = new OpenTelemetryConfigGenerator();
        AnyConfigProducer pp = this;
        AnyConfigProducer p = pp.getParent();
        while (p != null && p != pp) {
            if (pp instanceof ApplicationConfigProducerRoot root) {
                generator.addStatePorts(root.getStatePorts());
                break;
            }
            pp = p;
            p = pp.getParent();
        }
        builder.yaml(generator.generate());
        builder.refPaths(generator.referencedPaths());
    }
}
