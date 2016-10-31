// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
// Copyright (C) 1998-2003 Fast Search & Transfer ASA
// Copyright (C) 2003 Overture Services Norway AS

#include <vespa/fastos/fastos.h>
#include <vespa/log/log.h>
#include <vespa/fastlib/text/normwordfolder.h>
#include <vespa/searchsummary/docsummary/docsumwriter.h>
#include <vespa/searchsummary/docsummary/docsumfieldwriter.h>
#include <vespa/searchsummary/docsummary/docsumstore.h>
#include <vespa/searchsummary/docsummary/keywordextractor.h>
#include <vespa/searchsummary/docsummary/docsumformat.h>
#include <vespa/searchlib/common/transport.h>
#include <vespa/vespalib/data/slime/slime.h>
#include <vespa/searchlib/util/slime_output_raw_buf_adapter.h>

using namespace vespalib::slime::convenience;

namespace search {
namespace docsummary {

LOG_SETUP(".searchlib.docsummary.docsumwriter");


uint32_t
IDocsumWriter::slime2RawBuf(const Slime & slime, RawBuf & buf)
{
    const uint32_t preUsed = buf.GetUsedLen();
    const uint32_t magic = ::search::fs4transport::SLIME_MAGIC_ID;
    buf.append(&magic, sizeof(magic));
    SlimeOutputRawBufAdapter adapter(buf);
    vespalib::slime::BinaryFormat::encode(slime, adapter);
    return (buf.GetUsedLen() - preUsed);
}

uint32_t
DynamicDocsumWriter::WriteClassID(uint32_t classID, search::RawBuf *target)
{
    uint32_t written = 0;

    target->append(&classID, sizeof(classID));
    written = sizeof(classID);

    return written;
}


uint32_t
DynamicDocsumWriter::GenerateDocsum(uint32_t docid,
                                    GetDocsumsState *state,
                                    const ResultClass *outputClass,
                                    search::RawBuf *target)
{
    uint32_t written = 0;

    written += WriteClassID(outputClass->GetClassID(), target);

    for (uint32_t i = 0; i < outputClass->GetNumEntries(); i++) {

        const ResConfigEntry     *outCfg = outputClass->GetEntry(i);
        IDocsumFieldWriter *writer = _overrideTable[outCfg->_enumValue];
        LOG_ASSERT(writer != NULL);

        written += writer->WriteField(docid, NULL, state, outCfg->_type, target);
    }

    return written;
}


uint32_t
DynamicDocsumWriter::RepackDocsum(GeneralResult *gres,
                                  GetDocsumsState *state,
                                  const ResultClass *outputClass,
                                  search::RawBuf *target)
{
    uint32_t written = 0;

    written += WriteClassID(outputClass->GetClassID(), target);

    DocsumFormat::Appender appender(*target);

    for (uint32_t i = 0; i < outputClass->GetNumEntries(); i++) {

        const ResConfigEntry     *outCfg = outputClass->GetEntry(i);
        IDocsumFieldWriter *writer = _overrideTable[outCfg->_enumValue];

        if (writer != NULL) {

            written += writer->WriteField(gres->GetDocID(), gres, state,
                                          outCfg->_type, target);

        } else {

            int inIdx = gres->GetClass()->GetIndexFromEnumValue(outCfg->_enumValue);
            const ResConfigEntry *inCfg = gres->GetClass()->GetEntry(inIdx);

            if (inCfg != NULL && inCfg->_type == outCfg->_type) {

                // copy field

                const ResEntry *entry = gres->GetEntry(inIdx);
                LOG_ASSERT(entry != NULL);

                switch (outCfg->_type) {

                case RES_INT: {
                    written += appender.addInt32(entry->_intval);
                    break; }

                case RES_SHORT: {
                    written += appender.addShort(entry->_intval);
                    break; }

                case RES_BYTE: {
                    written += appender.addByte(entry->_intval);
                    break; }

                case RES_FLOAT: {
                    written += appender.addFloat(entry->_doubleval);
                    break; }

                case RES_DOUBLE: {
                    written += appender.addDouble(entry->_doubleval);
                    break; }

                case RES_INT64: {
                    written += appender.addInt64(entry->_int64val);
                    break; }

                case RES_STRING: {
                    uint32_t slen = entry->_stringlen;
                    const char *sval = entry->_stringval;
                    written += appender.addShortData(sval, slen);
                    break; }

                case RES_DATA: {
                    uint32_t dlen = entry->_datalen;
                    const char *dval = entry->_dataval;
                    written += appender.addShortData(dval, dlen);
                    break; }

                case RES_XMLSTRING:
                case RES_JSONSTRING:
                case RES_FEATUREDATA:
                case RES_LONG_STRING: {
                    uint32_t flen = entry->_len;
                    uint32_t slen = entry->_get_length();

                    // preserve compression flag
                    target->append(&flen, sizeof(flen));
                    written += sizeof(flen);
                    target->append(entry->_stringval, slen);
                    written += slen;
                    break; }

                case RES_LONG_DATA: {
                    uint32_t flen = entry->_len;
                    uint32_t dlen = entry->_get_length();

                    // preserve compression flag
                    target->append(&flen, sizeof(flen));
                    written += sizeof(flen);
                    target->append(entry->_dataval, dlen);
                    written += dlen;
                    break; }
                }

            } else {
                // insert empty field
                written += appender.addEmpty(outCfg->_type);
            }
        }
    } // END for loop

    return written;
}


DynamicDocsumWriter::ResolveClassInfo
DynamicDocsumWriter::resolveClassInfo(vespalib::stringref outputClassName, uint32_t inputClassId) const
{
    DynamicDocsumWriter::ResolveClassInfo rci = resolveOutputClass(outputClassName);
    if (!rci.mustSkip && !rci.allGenerated) {
        resolveInputClass(rci, inputClassId);
    }
    return rci;
}

DynamicDocsumWriter::ResolveClassInfo
DynamicDocsumWriter::resolveOutputClass(vespalib::stringref summaryClass) const
{
    DynamicDocsumWriter::ResolveClassInfo result;
    uint32_t id = _defaultOutputClass;
    id = _resultConfig->LookupResultClassId(summaryClass, id);

    if (id != ResultConfig::NoClassID()) {
        const ResultClass *oC = _resultConfig->LookupResultClass(id);
        if (oC == NULL) {
            LOG(warning, "Illegal docsum class requested: %d, using empty docsum for documents", id);
            result.mustSkip = true;
        } else {
            result.outputClass = oC;
            const ResultClass::DynamicInfo *rcInfo = oC->getDynamicInfo();
            if (rcInfo->_generateCnt == oC->GetNumEntries()) {
                LOG_ASSERT(rcInfo->_overrideCnt == rcInfo->_generateCnt);
                result.allGenerated = true;
            }
            result.outputClassInfo = rcInfo;
        }
    }
    result.outputClassId = id;
    return result;
}

void
DynamicDocsumWriter::resolveInputClass(ResolveClassInfo &rci, uint32_t id) const
{
    rci.inputClass = _resultConfig->LookupResultClass(id);
    if (rci.inputClass == NULL) {
        rci.mustSkip = true;
        return;
    }
    if (rci.outputClass == NULL) {
        LOG_ASSERT(rci.outputClassId == ResultConfig::NoClassID());
        rci.outputClassId = id;
        rci.outputClass = rci.inputClass;
        rci.outputClassInfo = rci.inputClass->getDynamicInfo();
    }
    if ((rci.inputClass == rci.outputClass) && (rci.outputClassInfo->_overrideCnt == 0)) {
        rci.mustRepack = false;
    }
}

void
DynamicDocsumWriter::resolveInputClass(ResolveClassInfo &rci, DocsumStoreValue blob) const
{
    uint32_t id = _resultConfig->GetClassID(blob.pt(), blob.len());
    resolveInputClass(rci, id);
}


uint32_t
DynamicDocsumWriter::oldWriteDocsum(uint32_t docid,
                                    GetDocsumsState *state,
                                    IDocsumStore *docinfos,
                                    search::RawBuf *target)
{
    ResolveClassInfo rci = resolveOutputClass(state->_args.getResultClassName());
    if (rci.mustSkip) {
        return 0;
    } else if (rci.allGenerated) {
        // generate docsum entry on-the-fly
        return GenerateDocsum(docid, state, rci.outputClass, target);
    }
    // look up docsum entry
    DocsumStoreValue value = docinfos->getMappedDocsum(docid, false);
    resolveInputClass(rci, value);
    if (rci.mustSkip) {
        return 0;
    } else if (rci.mustRepack) {
        // re-pack docsum blob
        GeneralResult gres(rci.inputClass, 0, docid, 0);
        if (gres.inplaceUnpack(value)) {
            return RepackDocsum(&gres, state, rci.outputClass, target);
        } else { // unpack failed
            LOG(error, "Unpack failed: illegal docsum entry for document %d", docid);
        }
    } else {
        // pass-through docsum blob
        target->append(value.pt(), value.len());
        return value.len();
    }
    return 0;
}


static void convertEntry(GetDocsumsState *state,
                         const ResConfigEntry *resCfg,
                         const ResEntry *entry,
                         Inserter &inserter,
                         Slime &slime)
{
    using vespalib::slime::BinaryFormat;
    const char *ptr;
    uint32_t len;

    LOG_ASSERT(resCfg != 0 && entry != 0);
    switch (resCfg->_type) {
    case RES_INT:
    case RES_SHORT:
    case RES_BYTE:
        inserter.insertLong(entry->_intval);
        break;
    case RES_FLOAT:
    case RES_DOUBLE:
        inserter.insertDouble(entry->_doubleval);
        break;
    case RES_INT64:
        inserter.insertLong(entry->_int64val);
        break;
    case RES_STRING:
    case RES_LONG_STRING:
    case RES_FEATUREDATA:
    case RES_XMLSTRING:
        entry->_resolve_field(&ptr, &len, &state->_docSumFieldSpace);
        inserter.insertString(Memory(ptr, len));
        break;
    case RES_DATA:
    case RES_LONG_DATA:
        entry->_resolve_field(&ptr, &len, &state->_docSumFieldSpace);
        inserter.insertData(Memory(ptr, len));
        break;
    case RES_JSONSTRING:
        entry->_resolve_field(&ptr, &len, &state->_docSumFieldSpace);
        if (len != 0) {
            // note: 'JSONSTRING' really means 'structured data',
            // and in this code path we depend on calling the
            // getMappedDocsum api with flag useSlimeInsideFields=true
            size_t d = BinaryFormat::decode_into(Memory(ptr, len), slime, inserter);
            if (d != len) {
                LOG(warning, "could not decode %u bytes: %zu bytes decoded", len, d);
            }
        }
        break;
    }
}


void
DynamicDocsumWriter::insertDocsum(const ResolveClassInfo & rci,
                                  uint32_t docid,
                                  GetDocsumsState *state,
                                  IDocsumStore *docinfos,
                                  vespalib::Slime & slime,
                                  vespalib::slime::Inserter & topInserter)
{
    if (rci.allGenerated) {
        // generate docsum entry on-the-fly
        vespalib::slime::Cursor & docsum = topInserter.insertObject();
        for (uint32_t i = 0; i < rci.outputClass->GetNumEntries(); ++i) {
            const ResConfigEntry *resCfg = rci.outputClass->GetEntry(i);
            IDocsumFieldWriter *writer = _overrideTable[resCfg->_enumValue];
            if (! writer->isDefaultValue(docid, state)) {
                const Memory field_name(resCfg->_bindname.data(),
                                        resCfg->_bindname.size());
                ObjectInserter inserter(docsum, field_name);
                writer->insertField(docid, NULL, state, resCfg->_type, inserter);
            }
        }
    } else {
        // look up docsum entry
        DocsumStoreValue value = docinfos->getMappedDocsum(docid, true);
        // re-pack docsum blob
        GeneralResult gres(rci.inputClass, 0, docid, 0);
        if (! gres.inplaceUnpack(value)) {
            LOG(debug, "Unpack failed: illegal docsum entry for document %d. This is expected during lidspace compaction.", docid);
            topInserter.insertNix();
            return;
        }
        vespalib::slime::Cursor & docsum = topInserter.insertObject();
        for (uint32_t i = 0; i < rci.outputClass->GetNumEntries(); ++i) {
            const ResConfigEntry *outCfg = rci.outputClass->GetEntry(i);
            IDocsumFieldWriter *writer = _overrideTable[outCfg->_enumValue];
            const Memory field_name(outCfg->_bindname.data(), outCfg->_bindname.size());
            ObjectInserter inserter(docsum, field_name);
            if (writer != NULL) {
                writer->insertField(docid, &gres, state, outCfg->_type, inserter);
            } else {
                if (rci.inputClass == rci.outputClass) {
                    convertEntry(state, outCfg, gres.GetEntry(i), inserter, slime);
                } else {
                    int inIdx = rci.inputClass->GetIndexFromEnumValue(outCfg->_enumValue);
                    const ResConfigEntry *inCfg = rci.inputClass->GetEntry(inIdx);
                    if (inCfg != NULL && inCfg->_type == outCfg->_type) {
                        // copy field
                        const ResEntry *entry = gres.GetEntry(inIdx);
                        LOG_ASSERT(entry != NULL);
                        convertEntry(state, outCfg, entry, inserter, slime);
                    }
                }
            }
        }
    }
}


DynamicDocsumWriter::DynamicDocsumWriter( ResultConfig *config, KeywordExtractor *extractor)
    : _resultConfig(config),
      _keywordExtractor(extractor),
      _defaultOutputClass(ResultConfig::NoClassID()),
      _numClasses(config->GetNumResultClasses()),
      _numEnumValues(config->GetFieldNameEnum().GetNumEntries()),
      _classInfoTable(NULL),
      _overrideTable(NULL)
{
    LOG_ASSERT(config != NULL);
    _classInfoTable = new ResultClass::DynamicInfo[_numClasses];
    _overrideTable  = new IDocsumFieldWriter*[_numEnumValues];

    uint32_t i = 0;
    for (ResultConfig::iterator it(config->begin()), mt(config->end()); it != mt; it++, i++) {
        _classInfoTable[i]._overrideCnt = 0;
        _classInfoTable[i]._generateCnt = 0;
        it->setDynamicInfo(&(_classInfoTable[i]));
    }
    LOG_ASSERT(i == _numClasses);

    for (i = 0; i < _numEnumValues; i++)
        _overrideTable[i] = NULL;
}


DynamicDocsumWriter::~DynamicDocsumWriter()
{
    delete _resultConfig;
    delete _keywordExtractor;

    delete [] _classInfoTable;

    for (uint32_t i = 0; i < _numEnumValues; i++)
        delete _overrideTable[i];
    delete [] _overrideTable;

}

bool
DynamicDocsumWriter::SetDefaultOutputClass(uint32_t classID)
{
    const ResultClass *resClass = _resultConfig->LookupResultClass(classID);

    if (resClass == NULL ||
        _defaultOutputClass != ResultConfig::NoClassID())
    {
        if (resClass == NULL) {
            LOG(warning, "cannot set default output docsum class to %d; class not defined", classID);
        } else if (_defaultOutputClass != ResultConfig::NoClassID()) {
            LOG(warning, "cannot set default output docsum class to %d; value already set", classID);
        }
        return false;
    }
    _defaultOutputClass = classID;
    return true;
}


bool
DynamicDocsumWriter::Override(const char *fieldName, IDocsumFieldWriter *writer)
{
    uint32_t fieldEnumValue = _resultConfig->GetFieldNameEnum().Lookup(fieldName);

    if (fieldEnumValue >= _numEnumValues ||
        _overrideTable[fieldEnumValue] != NULL)
    {

        if (fieldEnumValue >= _numEnumValues) {
            LOG(warning, "cannot override docsum field '%s'; undefined field name", fieldName);
        } else if (_overrideTable[fieldEnumValue] != NULL) {
            LOG(warning, "cannot override docsum field '%s'; already overridden", fieldName);
        }
        delete writer;
        return false;
    }

    writer->setIndex(fieldEnumValue);
    _overrideTable[fieldEnumValue] = writer;

    for (ResultConfig::iterator it(_resultConfig->begin()), mt(_resultConfig->end()); it != mt; it++) {

        if (it->GetIndexFromEnumValue(fieldEnumValue) >= 0) {
            ResultClass::DynamicInfo *info = it->getDynamicInfo();
            info->_overrideCnt++;
            if (writer->IsGenerated())
                info->_generateCnt++;
        }
    }

    return true;
}


void
DynamicDocsumWriter::InitState(IAttributeManager & attrMan, GetDocsumsState *state)
{
    state->_kwExtractor = _keywordExtractor;
    state->_attrCtx = attrMan.createContext();
    state->_attributes.resize(_numEnumValues);
    for (size_t i(0); i < state->_attributes.size(); i++) {
        const IDocsumFieldWriter *fw = _overrideTable[i];
        if (fw) {
            const vespalib::string & attributeName = fw->getAttributeName();
            if (!attributeName.empty()) {
                state->_attributes[i] = state->_attrCtx->getAttribute(attributeName);
            }
        }
    }
}


uint32_t
DynamicDocsumWriter::WriteDocsum(uint32_t docid,
                                 GetDocsumsState *state,
                                 IDocsumStore *docinfos,
                                 search::RawBuf *target)
{
    if ((state->_args.getFlags() & ::search::fs4transport::GDFLAG_ALLOW_SLIME) != 0) {
        vespalib::Slime slime;
        vespalib::slime::SlimeInserter inserter(slime);
        insertDocsum(resolveClassInfo(state->_args.getResultClassName(), docinfos->getSummaryClassId()), docid, state, docinfos, slime, inserter);
        return slime2RawBuf(slime, *target);
    }
    return oldWriteDocsum(docid, state, docinfos, target);
}


} // namespace search::docsummary
} // namespace search
