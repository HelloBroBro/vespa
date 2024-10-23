// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include "postinglistcounts.h"
#include <memory>
#include <cstdlib>

namespace search { class BitVector; }
namespace search::queryeval { class SearchIterator; }
namespace search::fef { class TermFieldMatchDataArray; }

namespace search::index {

class PostingListFileRandRead;

/**
 * Class for owning a posting list in memory after having read it from
 * posting list file, or referencing a chunk of memory containing the
 * posting list (if the file was memory mapped).
 */
class PostingListHandle {
public:
    using UP = std::unique_ptr<PostingListHandle>;
    // Key portion
    PostingListFileRandRead *_file; // File containing posting list
    uint64_t _bitOffset;    // posting list start relative to start of file
    uint64_t _bitLength;    // Length of posting list, in bits

    // Value portion
    uint64_t _bitOffsetMem; // _mem relative to start of file
    const void *_mem;       // Memory backing posting list after read/mmap
    void *_allocMem;        // What to free after posting list
    size_t _allocSize;      // Size of allocated memory
    uint64_t _read_bytes;   // Bytes read from disk (used by disk io stats)

    PostingListHandle()
    : _file(nullptr),
      _bitOffset(0),
      _bitLength(0),
      _bitOffsetMem(0),
      _mem(nullptr),
      _allocMem(nullptr),
      _allocSize(0),
      _read_bytes(0)
    { }

    ~PostingListHandle()
    {
        if (_allocMem != nullptr) {
            free(_allocMem);
        }
    }

    /**
     * Create iterator for single word.  Semantic lifetime of counts and
     * handle must exceed lifetime of iterator.
     */
    std::unique_ptr<search::queryeval::SearchIterator>
    createIterator(const PostingListCounts &counts,
                   const search::fef::TermFieldMatchDataArray &matchData) const;

    /**
     * Drop value portion of handle.
     */
    void drop() {
        _bitOffsetMem = 0;
        _mem = nullptr;
        if (_allocMem != nullptr) {
            free(_allocMem);
            _allocMem = nullptr;
        }
        _allocSize = 0;
        _read_bytes = 0;
    }
};

}
