// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <vespa/document/bucket/bucketspace.h>
#include <vespa/vdslib/distribution/distribution_config_bundle.h>
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace storage::lib {

class ClusterState;

/**
 * Class representing the baseline cluster state and the derived cluster
 * state for each bucket space.
 */
class ClusterStateBundle {
public:
    /**
     * Represents feed blocking status of the entire cluster.
     *
     * Feed blocking only applies to client feed, and is turned on
     * when this object is present and _block_feed_in_cluster is true.
     * Feed generated by internal maintenance operations (e.g. merging) is not affected.
     */
    class FeedBlock {
    private:
        bool _block_feed_in_cluster;
        std::string _description;

    public:
        FeedBlock(bool block_feed_in_cluster_in,
                  const std::string& description_in);
        [[nodiscard]] bool block_feed_in_cluster() const noexcept { return _block_feed_in_cluster; }
        [[nodiscard]] const std::string& description() const noexcept { return _description; }
        bool operator==(const FeedBlock& rhs) const noexcept;
        bool operator!=(const FeedBlock& rhs) const noexcept { return !operator==(rhs); }
    };

    using BucketSpaceStateMapping = std::unordered_map<
        document::BucketSpace,
        std::shared_ptr<const ClusterState>,
        document::BucketSpace::hash
    >;
    std::shared_ptr<const ClusterState>             _baselineClusterState;
    BucketSpaceStateMapping                         _derivedBucketSpaceStates;
    std::optional<FeedBlock>                        _feed_block;
    std::shared_ptr<const DistributionConfigBundle> _distribution_bundle;
    bool                                            _deferredActivation;
public:
    explicit ClusterStateBundle(std::shared_ptr<const ClusterState> baseline_cluster_state);
    explicit ClusterStateBundle(const ClusterState& baselineClusterState);
    ClusterStateBundle(const ClusterState& baselineClusterState,
                       BucketSpaceStateMapping derivedBucketSpaceStates);
    ClusterStateBundle(const ClusterState& baselineClusterState,
                       BucketSpaceStateMapping derivedBucketSpaceStates,
                       bool deferredActivation);
    ClusterStateBundle(const ClusterState& baselineClusterState,
                       BucketSpaceStateMapping derivedBucketSpaceStates,
                       const FeedBlock& feed_block_in,
                       bool deferredActivation);
    ClusterStateBundle(std::shared_ptr<const ClusterState> baseline_cluster_state,
                       BucketSpaceStateMapping derived_bucket_space_states,
                       std::optional<FeedBlock> feed_block_in,
                       std::shared_ptr<const DistributionConfigBundle> distribution_bundle,
                       bool deferred_activation);

    ClusterStateBundle(const ClusterStateBundle&);
    ClusterStateBundle& operator=(const ClusterStateBundle&);
    ClusterStateBundle(ClusterStateBundle&&) noexcept;
    ClusterStateBundle& operator=(ClusterStateBundle&&) noexcept;

    ~ClusterStateBundle();

    // Factory function for "simulating" atomic cluster states with distribution config attached
    // when the cluster controller is not yet on version that sends bundled config.
    [[nodiscard]] std::shared_ptr<const ClusterStateBundle> clone_with_new_distribution(
            std::shared_ptr<const DistributionConfigBundle>) const;

    const std::shared_ptr<const ClusterState>& getBaselineClusterState() const;
    const std::shared_ptr<const ClusterState>& getDerivedClusterState(document::BucketSpace bucketSpace) const;
    const BucketSpaceStateMapping& getDerivedClusterStates() const noexcept {
        return _derivedBucketSpaceStates;
    }
    [[nodiscard]] bool block_feed_in_cluster() const noexcept {
        return _feed_block.has_value() && _feed_block->block_feed_in_cluster();
    }
    [[nodiscard]] bool has_distribution_config() const noexcept {
        return static_cast<bool>(_distribution_bundle);
    }
    [[nodiscard]] const DistributionConfigBundle* distribution_config_bundle_or_nullptr() const noexcept {
        return _distribution_bundle.get();
    }
    // Only guaranteed to not be nullptr iff has_distribution_config() == true
    [[nodiscard]] const std::shared_ptr<const DistributionConfigBundle>& distribution_config_bundle() const noexcept {
        return _distribution_bundle;
    }
    [[nodiscard]] std::shared_ptr<const Distribution> bucket_space_distribution_or_nullptr(document::BucketSpace space) const noexcept;
    [[nodiscard]] const std::optional<FeedBlock>& feed_block() const { return _feed_block; }
    [[nodiscard]] uint32_t getVersion() const;
    [[nodiscard]] bool deferredActivation() const noexcept { return _deferredActivation; }
    [[nodiscard]] std::string toString() const;
    bool operator==(const ClusterStateBundle& rhs) const noexcept;
    bool operator!=(const ClusterStateBundle& rhs) const noexcept { return !operator==(rhs); }
};

std::ostream& operator<<(std::ostream&, const ClusterStateBundle&);

}
