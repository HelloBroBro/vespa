// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.schema.processing;

import com.yahoo.config.application.api.DeployLogger;
import com.yahoo.document.Field;
import com.yahoo.schema.RankProfileRegistry;
import com.yahoo.schema.Schema;
import com.yahoo.schema.document.Attribute;
import com.yahoo.tensor.TensorType;
import com.yahoo.vespa.model.container.search.QueryProfiles;

import java.util.Optional;

/**
 * Validates the 'paged' attribute setting and throws if specified on unsupported types.
 *
 * @author geirst
 */
public class PagedAttributeValidator extends Processor {

    private static final String disavantagesUrl =
            "https://docs.vespa.ai/en/attributes.html#paged-attributes-disadvantages";

    public PagedAttributeValidator(Schema schema,
                                   DeployLogger deployLogger,
                                   RankProfileRegistry rankProfileRegistry,
                                   QueryProfiles queryProfiles) {
        super(schema, deployLogger, rankProfileRegistry, queryProfiles);
    }

    @Override
    public void process(boolean validate, boolean documentsOnly) {
        if (!validate) {
            return;
        }
        for (var field : schema.allConcreteFields()) {
            for (var attribute : field.getAttributes().values()) {
                if (attribute.isPaged()) {
                    validatePagedSetting(field, attribute);
                }
            }
        }
    }

    private void validatePagedSetting(Field field, Attribute attribute) {
        if (!isSupportedType(attribute)) {
            fail(schema, field, "The 'paged' attribute setting is not supported for fast-rank tensor and predicate types");
        }
        if (attribute.getType() == Attribute.Type.TENSOR && attribute.hnswIndexParams().isPresent()) {
            warn(schema.getName(), field.getName(), "The 'paged' attribute setting in combination with " +
                            "HNSW indexing is strongly discouraged, see " + disavantagesUrl + " for details");
        }
    }

    private boolean isSupportedType(Attribute attribute) {
        var type = attribute.getType();
        return (type != Attribute.Type.PREDICATE) &&
                (isSupportedTensorType(attribute.tensorType(), attribute.isFastRank()));
    }

    private boolean isSupportedTensorType(Optional<TensorType> tensorType, boolean fastRank) {
        if (tensorType.isPresent()) {
            return !fastRank;
        }
        return true;
    }

}
