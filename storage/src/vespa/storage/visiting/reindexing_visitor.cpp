// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#include "reindexing_visitor.h"
#include <vespa/document/fieldvalue/document.h>
#include <vespa/documentapi/messagebus/messages/putdocumentmessage.h>
#include <vespa/storage/common/reindexing_constants.h>
#include <vespa/persistence/spi/docentry.h>
#include <vespa/vespalib/stllike/string.h>

#include <vespa/log/log.h>
LOG_SETUP(".visitor.instance.reindexing_visitor");

namespace storage {

ReindexingVisitor::ReindexingVisitor(StorageComponent& component)
    : Visitor(component)
{
}

void ReindexingVisitor::handleDocuments(const document::BucketId& ,
                                        DocEntryList & entries,
                                        HitCounter& hitCounter)
{
    auto lock_token = make_lock_access_token();
    LOG(debug, "ReindexingVisitor %s handling block of %zu documents. Using access token '%s'",
        _id.c_str(), entries.size(), lock_token.c_str());
    for (auto& entry : entries) {
        if (entry->isRemove()) {
            // We don't reindex removed documents, as that would be very silly.
            continue;
        }
        const uint32_t doc_size = entry->getSize();
        hitCounter.addHit(*entry->getDocumentId(), doc_size);
        auto msg = std::make_unique<documentapi::PutDocumentMessage>(entry->releaseDocument());
        msg->setApproxSize(doc_size);
        msg->setCondition(documentapi::TestAndSetCondition(lock_token));
        sendMessage(std::move(msg));
    }
}

bool ReindexingVisitor::remap_docapi_message_error_code(api::ReturnCode& in_out_code) {
    if (in_out_code.getResult() == api::ReturnCode::TEST_AND_SET_CONDITION_FAILED) {
        in_out_code = api::ReturnCode(api::ReturnCode::ABORTED, "Got TaS failure from upstream, indicating visitor is "
                                                                "outdated. Aborting session to allow client to retry");
        return true;
    }
    return Visitor::remap_docapi_message_error_code(in_out_code);
}

std::string ReindexingVisitor::make_lock_access_token() const {
    std::string prefix = reindexing_bucket_lock_bypass_prefix();
    std::string_view passed_token = visitor_parameters().get(
            reindexing_bucket_lock_visitor_parameter_key(),
            std::string_view(""));
    if (passed_token.empty()) {
        return prefix;
    }
    return (prefix + "=" + passed_token);
}

}
