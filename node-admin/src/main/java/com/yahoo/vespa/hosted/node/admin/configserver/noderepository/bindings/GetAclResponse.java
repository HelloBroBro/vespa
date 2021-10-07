// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.hosted.node.admin.configserver.noderepository.bindings;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;

import java.util.List;

/**
 * This class represents a response from the /nodes/v2/acl/ API.
 *
 * @author mpolden
 */
@JsonIgnoreProperties(ignoreUnknown = true)
public class GetAclResponse {

    @JsonProperty("trustedNodes")
    public final List<Node> trustedNodes;

    @JsonProperty("trustedNetworks")
    public final List<Network> trustedNetworks;

    @JsonProperty("trustedPorts")
    public final List<Port> trustedPorts;

    @JsonCreator
    public GetAclResponse(@JsonProperty("trustedNodes") List<Node> trustedNodes,
                          @JsonProperty("trustedNetworks") List<Network> trustedNetworks,
                          @JsonProperty("trustedPorts") List<Port> trustedPorts) {
        this.trustedNodes = trustedNodes == null ? List.of() : trustedNodes;
        this.trustedNetworks = trustedNetworks == null ? List.of() : trustedNetworks;
        this.trustedPorts = trustedPorts == null ? List.of() : trustedPorts;
    }

    @JsonIgnoreProperties(ignoreUnknown = true)
    public static class Node {

        @JsonProperty("hostname")
        public final String hostname;

        @JsonProperty("ipAddress")
        public final String ipAddress;

        @JsonProperty("trustedBy")
        public final String trustedBy;

        @JsonCreator
        public Node(@JsonProperty("hostname") String hostname, @JsonProperty("ipAddress") String ipAddress,
                    @JsonProperty("trustedBy") String trustedBy) {
            this.hostname = hostname;
            this.ipAddress = ipAddress;
            this.trustedBy = trustedBy;
        }

        public String getTrustedBy() {
            return trustedBy;
        }
    }

    @JsonIgnoreProperties(ignoreUnknown = true)
    public static class Network {

        @JsonProperty("network")
        public final String network;

        @JsonProperty("trustedBy")
        public final String trustedBy;

        @JsonCreator
        public Network(@JsonProperty("network") String network, @JsonProperty("trustedBy") String trustedBy) {
            this.network = network;
            this.trustedBy = trustedBy;
        }

        public String getTrustedBy() {
            return trustedBy;
        }
    }

    @JsonIgnoreProperties(ignoreUnknown = true)
    public static class Port {

        @JsonProperty("port")
        public final Integer port;

        @JsonProperty("trustedBy")
        public final String trustedBy;

        @JsonCreator
        public Port(@JsonProperty("port") Integer port, @JsonProperty("trustedBy") String trustedBy) {
            this.port = port;
            this.trustedBy = trustedBy;
        }

        public String getTrustedBy() {
            return trustedBy;
        }
    }
}
