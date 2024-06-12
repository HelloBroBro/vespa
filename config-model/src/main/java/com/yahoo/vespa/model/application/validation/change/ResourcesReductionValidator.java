// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.model.application.validation.change;

import com.yahoo.config.application.api.ValidationId;
import com.yahoo.config.provision.ClusterResources;
import com.yahoo.config.provision.ClusterSpec;
import com.yahoo.config.provision.NodeResources;
import com.yahoo.vespa.model.VespaModel;
import com.yahoo.vespa.model.application.validation.Validation.ChangeContext;

/**
 * Checks that no cluster sizes are reduced too much in one go.
 *
 * @author bratseth
 */
public class ResourcesReductionValidator implements ChangeValidator {

    @Override
    public void validate(ChangeContext context) {
        for (var clusterId : context.previousModel().allClusters()) {
            if (context.model().allClusters().contains(clusterId))
                validate(clusterId, context);
        }
    }

    private void validate(ClusterSpec.Id clusterId, ChangeContext context) {
        ClusterResources current = clusterResources(clusterId, context.previousModel());
        ClusterResources next = clusterResources(clusterId, context.model());
        if (current == null || next == null) return; // No request recording - test
        if (current.nodeResources().isUnspecified() || next.nodeResources().isUnspecified()) {
            // Self-hosted - unspecified resources; compare node count
            int currentNodes = current.nodes();
            int nextNodes = next.nodes();
            if (nextNodes < 0.5 * currentNodes && nextNodes != currentNodes - 1) {
                context.invalid(ValidationId.resourcesReduction,
                                "Size reduction in '" + clusterId.value() + "' is too large: " +
                                "To guard against mistakes, the new max nodes must be at least 50% of the current nodes. " +
                                "Current nodes: " + currentNodes + ", new nodes: " + nextNodes);
            }
        }
        else {
            NodeResources currentResources = current.totalResources();
            NodeResources nextResources = next.totalResources();
            if (nextResources.vcpu() < 0.5 * currentResources.vcpu() ||
                nextResources.memoryGiB() < 0.5 * currentResources.memoryGiB() ||
                nextResources.diskGb() < 0.5 * currentResources.diskGb())
                context.invalid(ValidationId.resourcesReduction,
                                "Resource reduction in '" + clusterId.value() + "' is too large: " +
                                "To guard against mistakes, the new max resources must be at least 50% of the current " +
                                "max resources in all dimensions. " +
                                "Current: " + currentResources.withBandwidthGbps(0) + // (don't output bandwidth here)
                                ", new: " + nextResources.withBandwidthGbps(0));
        }

    }

    /**
     * If the given requested cluster resources does not specify node resources, return them with
     * the current node resources of the cluster, as that is what unspecified resources actually resolved to.
     * This will always yield specified node resources on hosted instances and never on self-hosted instances.
     */
    private ClusterResources clusterResources(ClusterSpec.Id id, VespaModel model) {
        if ( ! model.provisioned().capacities().containsKey(id)) return null;

        ClusterResources resources = model.provisioned().capacities().get(id).maxResources();
        if ( ! resources.nodeResources().isUnspecified()) return resources;

        var containerCluster = model.getContainerClusters().get(id.value());
        if (containerCluster != null) {
            if ( ! containerCluster.getContainers().isEmpty())
                return resources.with(containerCluster.getContainers().get(0).getHostResource().advertisedResources());
        }

        var contentCluster = model.getContentClusters().get(id.value());
        if (contentCluster != null) {
            var searchCluster = contentCluster.getSearch();
            if ( ! searchCluster.getSearchNodes().isEmpty())
                return resources.with(searchCluster.getSearchNodes().get(0).getHostResource().advertisedResources());
        }

        return resources; // only expected for admin clusters
    }

}
