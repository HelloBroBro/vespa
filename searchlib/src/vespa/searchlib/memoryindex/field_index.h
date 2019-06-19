// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "feature_store.h"
#include "field_index_remover.h"
#include "i_field_index.h"
#include "posting_list_entry.h"
#include "word_store.h"
#include <vespa/searchlib/index/docidandfeatures.h>
#include <vespa/searchlib/index/field_length_calculator.h>
#include <vespa/searchlib/index/indexbuilder.h>
#include <vespa/vespalib/btree/btree.h>
#include <vespa/vespalib/btree/btreenodeallocator.h>
#include <vespa/vespalib/btree/btreeroot.h>
#include <vespa/vespalib/btree/btreestore.h>
#include <vespa/vespalib/stllike/string.h>
#include <vespa/vespalib/util/memoryusage.h>

namespace search::memoryindex {

class IOrderedFieldIndexInserter;

/**
 * Implementation of memory index for a single field using lock-free B-Trees in underlying components.
 *
 * It consists of the following components:
 *   - WordStore containing all unique words in this field (across all documents).
 *   - B-Tree dictionary that maps from unique word (32-bit ref) -> posting list (32-bit ref).
 *   - B-Tree posting lists that maps from document id (32-bit) -> features (32-bit ref).
 *   - BTreeStore containing all the posting lists.
 *   - FeatureStore containing information on where a (word, document) pair matched this field.
 *     This information is unpacked and used during ranking.
 *
 * Elements in the three stores are accessed using 32-bit references / handles.
 */
class FieldIndex : public IFieldIndex {
public:
    // Mapping from docid -> feature ref
    using PostingListEntryType = PostingListEntry<false>;
    using PostingList = btree::BTreeRoot<uint32_t, PostingListEntryType, search::btree::NoAggregated>;
    using PostingListStore = btree::BTreeStore<uint32_t, PostingListEntryType,
                                               search::btree::NoAggregated,
                                               std::less<uint32_t>,
                                               btree::BTreeDefaultTraits>;
    using PostingListKeyDataType = PostingListStore::KeyDataType;

    struct WordKey {
        datastore::EntryRef _wordRef;

        explicit WordKey(datastore::EntryRef wordRef) : _wordRef(wordRef) { }
        WordKey() : _wordRef() { }

        friend vespalib::asciistream &
        operator<<(vespalib::asciistream & os, const WordKey & rhs);
    };

    class KeyComp {
    private:
        const WordStore          &_wordStore;
        const vespalib::stringref _word;

        const char *getWord(datastore::EntryRef wordRef) const {
            if (wordRef.valid()) {
                return _wordStore.getWord(wordRef);
            }
            return _word.data();
        }

    public:
        KeyComp(const WordStore &wordStore, const vespalib::stringref word)
            : _wordStore(wordStore),
              _word(word)
        { }

        bool operator()(const WordKey & lhs, const WordKey & rhs) const {
            int cmpres = strcmp(getWord(lhs._wordRef), getWord(rhs._wordRef));
            return cmpres < 0;
        }
    };

    using PostingListPtr = uint32_t;
    using DictionaryTree = btree::BTree<WordKey, PostingListPtr,
                                        search::btree::NoAggregated,
                                        const KeyComp>;
private:
    using GenerationHandler = vespalib::GenerationHandler;

    WordStore               _wordStore;
    uint64_t                _numUniqueWords;
    GenerationHandler       _generationHandler;
    DictionaryTree          _dict;
    PostingListStore        _postingListStore;
    FeatureStore            _featureStore;
    uint32_t                _fieldId;
    FieldIndexRemover       _remover;
    std::unique_ptr<IOrderedFieldIndexInserter> _inserter;
    index::FieldLengthCalculator _calculator;

public:
    datastore::EntryRef addWord(const vespalib::stringref word) {
        _numUniqueWords++;
        return _wordStore.addWord(word);
    }

    datastore::EntryRef addFeatures(const index::DocIdAndFeatures &features) {
        return _featureStore.addFeatures(_fieldId, features).first;
    }

    FieldIndex(const index::Schema& schema, uint32_t fieldId);
    FieldIndex(const index::Schema& schema, uint32_t fieldId, const index::FieldLengthInfo& info);
    ~FieldIndex();
    PostingList::Iterator find(const vespalib::stringref word) const;

    PostingList::ConstIterator
    findFrozen(const vespalib::stringref word) const;

    uint64_t getNumUniqueWords() const override { return _numUniqueWords; }
    const FeatureStore & getFeatureStore() const override { return _featureStore; }
    const WordStore &getWordStore() const override { return _wordStore; }
    IOrderedFieldIndexInserter &getInserter() override { return *_inserter; }
    index::FieldLengthCalculator &get_calculator() override { return _calculator; }

private:
    void freeze() {
        _postingListStore.freeze();
        _dict.getAllocator().freeze();
    }

    void trimHoldLists() {
        GenerationHandler::generation_t usedGen =
            _generationHandler.getFirstUsedGeneration();
        _postingListStore.trimHoldLists(usedGen);
        _dict.getAllocator().trimHoldLists(usedGen);
        _featureStore.trimHoldLists(usedGen);
    }

    void transferHoldLists() {
        GenerationHandler::generation_t generation =
            _generationHandler.getCurrentGeneration();
        _postingListStore.transferHoldLists(generation);
        _dict.getAllocator().transferHoldLists(generation);
        _featureStore.transferHoldLists(generation);
    }

    void incGeneration() {
        _generationHandler.incGeneration();
    }

public:
    GenerationHandler::Guard takeGenerationGuard() override {
        return _generationHandler.takeGuard();
    }

    void compactFeatures() override;

    void dump(search::index::IndexBuilder & indexBuilder) override;

    vespalib::MemoryUsage getMemoryUsage() const override;
    DictionaryTree &getDictionaryTree() { return _dict; }
    PostingListStore &getPostingListStore() { return _postingListStore; }
    FieldIndexRemover &getDocumentRemover() override { return _remover; }

    void commit() override {
        _remover.flush();
        freeze();
        transferHoldLists();
        incGeneration();
        trimHoldLists();
    }

    std::unique_ptr<queryeval::SimpleLeafBlueprint> make_term_blueprint(const vespalib::string& term,
                                                                        const queryeval::FieldSpecBase& field,
                                                                        uint32_t field_id) override;
};

}

namespace search::btree {

extern template
class BTreeNodeDataWrap<memoryindex::FieldIndex::WordKey,
                        BTreeDefaultTraits::LEAF_SLOTS>;

extern template
class BTreeNodeT<memoryindex::FieldIndex::WordKey,
                 BTreeDefaultTraits::INTERNAL_SLOTS>;

#if 0
extern template
class BTreeNodeT<memoryindex::FieldIndex::WordKey,
                 BTreeDefaultTraits::LEAF_SLOTS>;
#endif

extern template
class BTreeNodeTT<memoryindex::FieldIndex::WordKey,
                  datastore::EntryRef,
                  search::btree::NoAggregated,
                  BTreeDefaultTraits::INTERNAL_SLOTS>;

extern template
class BTreeNodeTT<memoryindex::FieldIndex::WordKey,
                  memoryindex::FieldIndex::PostingListPtr,
                  search::btree::NoAggregated,
                  BTreeDefaultTraits::LEAF_SLOTS>;

extern template
class BTreeInternalNode<memoryindex::FieldIndex::WordKey,
                        search::btree::NoAggregated,
                        BTreeDefaultTraits::INTERNAL_SLOTS>;

extern template
class BTreeLeafNode<memoryindex::FieldIndex::WordKey,
                    memoryindex::FieldIndex::PostingListPtr,
                    search::btree::NoAggregated,
                    BTreeDefaultTraits::LEAF_SLOTS>;

extern template
class BTreeNodeStore<memoryindex::FieldIndex::WordKey,
                     memoryindex::FieldIndex::PostingListPtr,
                     search::btree::NoAggregated,
                     BTreeDefaultTraits::INTERNAL_SLOTS,
                     BTreeDefaultTraits::LEAF_SLOTS>;

extern template
class BTreeIterator<memoryindex::FieldIndex::WordKey,
                    memoryindex::FieldIndex::PostingListPtr,
                    search::btree::NoAggregated,
                    const memoryindex::FieldIndex::KeyComp,
                    BTreeDefaultTraits>;

extern template
class BTree<memoryindex::FieldIndex::WordKey,
            memoryindex::FieldIndex::PostingListPtr,
            search::btree::NoAggregated,
            const memoryindex::FieldIndex::KeyComp,
            BTreeDefaultTraits>;

extern template
class BTreeRoot<memoryindex::FieldIndex::WordKey,
               memoryindex::FieldIndex::PostingListPtr,
                search::btree::NoAggregated,
               const memoryindex::FieldIndex::KeyComp,
               BTreeDefaultTraits>;

extern template
class BTreeRootBase<memoryindex::FieldIndex::WordKey,
                    memoryindex::FieldIndex::PostingListPtr,
                    search::btree::NoAggregated,
                    BTreeDefaultTraits::INTERNAL_SLOTS,
                    BTreeDefaultTraits::LEAF_SLOTS>;

extern template
class BTreeNodeAllocator<memoryindex::FieldIndex::WordKey,
                       memoryindex::FieldIndex::PostingListPtr,
                         search::btree::NoAggregated,
                       BTreeDefaultTraits::INTERNAL_SLOTS,
                       BTreeDefaultTraits::LEAF_SLOTS>;

}
