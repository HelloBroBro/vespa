// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "querywrapper.h"

using namespace search::streaming;

namespace streaming {

QueryWrapper::PhraseList::PhraseList(Query & query)
    : _phrases()
{
    QueryNodeRefList phrases;
    query.getPhrases(phrases);
    for (size_t i = 0; i < phrases.size(); ++i) {
        _phrases.push_back(static_cast<PhraseQueryNode *>(phrases[i]));
    }
}

PhraseQueryNode *
QueryWrapper::PhraseList::findPhrase(QueryTerm * term, size_t & index)
{
    for (size_t i = 0; i < _phrases.size(); ++i) {
        auto& terms = _phrases[i]->get_terms();
        for (size_t j = 0; j < terms.size(); ++j) {
            if (terms[j].get() == term) {
                index = j;
                return _phrases[i];
            }
        }
    }
    return nullptr;
}

QueryWrapper::QueryWrapper(Query & query) :
    _phraseList(query),
    _termList()
{
    QueryTermList leaves;
    query.getLeaves(leaves);
    for (size_t i = 0; i < leaves.size(); ++i) {
        size_t index = 0;
        PhraseQueryNode * parent = _phraseList.findPhrase(leaves[i], index);
        _termList.push_back(Term(leaves[i], parent, index));
    }
}

QueryWrapper::~QueryWrapper() = default;

} // namespace streaming

