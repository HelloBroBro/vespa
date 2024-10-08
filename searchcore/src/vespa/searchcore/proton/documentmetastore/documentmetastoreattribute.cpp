// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "documentmetastoreattribute.h"
#include <vespa/searchcommon/attribute/config.h>

namespace proton {

namespace {

const std::string documentMetaStoreName("[documentmetastore]");

}

const std::string &
DocumentMetaStoreAttribute::getFixedName()
{
    return documentMetaStoreName;
}

DocumentMetaStoreAttribute::DocumentMetaStoreAttribute(const std::string &name)
    : NotImplementedAttribute(name, Config(BasicType::NONE))
{ }


DocumentMetaStoreAttribute::~DocumentMetaStoreAttribute() = default;

}
