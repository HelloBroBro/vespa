// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include "testandsetmessage.h"

namespace document { class Document; }
namespace documentapi {

class PutDocumentMessage : public TestAndSetMessage {
private:
    using DocumentSP = std::shared_ptr<document::Document>;
    DocumentSP _document;
    uint64_t   _time;
    uint64_t   _persisted_timestamp;
    bool       _create_if_non_existent = false;

protected:
    DocumentReply::UP doCreateReply() const override;

public:
    using UP = std::unique_ptr<PutDocumentMessage>;
    using SP = std::shared_ptr<PutDocumentMessage>;

    /**
     * Constructs a new document message for deserialization.
     */
    PutDocumentMessage();

    /**
     * Constructs a new document put message.
     *
     * @param document The document to put.
     */
    explicit PutDocumentMessage(DocumentSP document);
    ~PutDocumentMessage() override;

    /**
     * Returns the document to put.
     *
     * @return The document.
     */
    const DocumentSP & getDocumentSP() const { return _document; }
    [[nodiscard]] DocumentSP stealDocument() { return std::move(_document); }
    const document::Document & getDocument() const { return *_document; }

    /**
     * Sets the document to put.
     *
     * @param document The document to set.
     */
    void setDocument(DocumentSP document);

    /**
     * Returns the timestamp of the document to put.
     *
     * @return The document timestamp.
     */
    [[nodiscard]] uint64_t getTimestamp() const { return _time; }

    /**
     * Sets the timestamp of the document to put.
     *
     * @param time The timestamp to set.
     */
    void setTimestamp(uint64_t time) { _time = time; }
    bool hasSequenceId() const override;
    uint64_t getSequenceId() const override;
    uint32_t getType() const override;
    string toString() const override { return "putdocumentmessage"; }

    void set_persisted_timestamp(uint64_t ts) noexcept { _persisted_timestamp = ts; }
    // When a visitor client receives a Put as part of the visiting operation, this
    // timestamp represents the wall clock time of the last mutating operation to the
    // document. If zero, the content node was too old to support this feature.
    [[nodiscard]] uint64_t persisted_timestamp() const noexcept { return _persisted_timestamp; }

    void set_create_if_non_existent(bool value) noexcept { _create_if_non_existent = value; }
    [[nodiscard]] bool get_create_if_non_existent() const noexcept { return _create_if_non_existent; }
};

}
