// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.tensor.functions;

import com.yahoo.tensor.evaluation.Name;

import java.util.List;
import java.util.Objects;

/**
 * @author bratseth
 */
public class Argmin<NAMETYPE extends Name> extends CompositeTensorFunction<NAMETYPE> {

    private final TensorFunction<NAMETYPE> argument;
    private final List<String> dimensions;

    public Argmin(TensorFunction<NAMETYPE> argument) {
        this(argument, List.of());
    }

    public Argmin(TensorFunction<NAMETYPE> argument, String dimension) {
        this(argument, List.of(dimension));
    }

    public Argmin(TensorFunction<NAMETYPE> argument, List<String> dimensions) {
        Objects.requireNonNull(dimensions, "The dimensions cannot be null");
        this.argument = argument;
        this.dimensions = List.copyOf(dimensions);
    }

    @Override
    public List<TensorFunction<NAMETYPE>> arguments() { return List.of(argument); }

    @Override
    public TensorFunction<NAMETYPE> withArguments(List<TensorFunction<NAMETYPE>> arguments) {
        if (arguments.size() != 1)
            throw new IllegalArgumentException("Argmin must have 1 argument, got " + arguments.size());
        return new Argmin<>(arguments.get(0), dimensions);
    }

    @Override
    public PrimitiveTensorFunction<NAMETYPE> toPrimitive() {
        TensorFunction<NAMETYPE> primitiveArgument = argument.toPrimitive();
        TensorFunction<NAMETYPE> reduce = new Reduce<>(primitiveArgument, Reduce.Aggregator.min, dimensions);
        return new Join<>(primitiveArgument, reduce, ScalarFunctions.equal());
    }

    @Override
    public String toString(ToStringContext<NAMETYPE> context) {
        return "argmin(" + argument.toString(context) + Reduce.commaSeparatedNames(dimensions, context) + ")";
    }

    @Override
    public int hashCode() { return Objects.hash("argmin", argument, dimensions); }

}
