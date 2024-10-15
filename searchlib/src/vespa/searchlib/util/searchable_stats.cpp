// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "searchable_stats.h"
#include <ostream>

namespace search {

SearchableStats::SearchableStats()
    : _memoryUsage(),
      _docsInMemory(0),
      _sizeOnDisk(0),
      _fusion_size_on_disk(0),
      _field_stats()
{
}

SearchableStats::~SearchableStats() = default;

SearchableStats&
SearchableStats::merge(const SearchableStats &rhs) {
    _memoryUsage.merge(rhs._memoryUsage);
    _docsInMemory += rhs._docsInMemory;
    _sizeOnDisk += rhs._sizeOnDisk;
    _fusion_size_on_disk += rhs._fusion_size_on_disk;
    for (auto& rhs_field : rhs._field_stats) {
        _field_stats[rhs_field.first].merge(rhs_field.second);
    }
    return *this;
}

bool
SearchableStats::operator==(const SearchableStats& rhs) const noexcept
{
    return _memoryUsage == rhs._memoryUsage &&
    _docsInMemory == rhs._docsInMemory &&
    _sizeOnDisk == rhs._sizeOnDisk &&
    _fusion_size_on_disk == rhs._fusion_size_on_disk &&
    _field_stats == rhs._field_stats;
}

SearchableStats&
SearchableStats::add_field_stats(const std::string& name, const FieldIndexStats& stats)
{
    _field_stats[name].merge(stats);
    return *this;
}

std::ostream& operator<<(std::ostream& os, const SearchableStats& stats) {
    os << "{memory: " << stats.memoryUsage() << ", docsInMemory: " << stats.docsInMemory() <<
       ", disk: " << stats.sizeOnDisk() << ", fusion_size_on_disk: " << stats.fusion_size_on_disk() << ", ";
    os << "fields: {";
    bool first = true;
    for (auto& field : stats.get_field_stats()) {
        if (!first) {
            os << ", ";
        }
        first = false;
        os << "\"" << field.first << "\": " << field.second;
    }
    os << "}";
    return os;
}

}
