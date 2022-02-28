// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.searchdefinition.parser;


import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;

/**
 * This class holds the extracted information after parsing a
 * "document" block in a schema (.sd) file, using simple data
 * structures as far as possible.  Do not put advanced logic here!
 * @author arnej27959
 **/
public class ParsedDocument extends ParsedBlock {
    private final List<String> inherited = new ArrayList<>();
    private final Map<String, ParsedDocument> resolvedInherits = new HashMap();
    private final Map<String, ParsedField> docFields = new HashMap<>();
    private final Map<String, ParsedStruct> docStructs = new HashMap<>();
    private final Map<String, ParsedAnnotation> docAnnotations = new HashMap<>();

    public ParsedDocument(String name) {
        super(name, "document");
    }

    List<String> getInherited() { return List.copyOf(inherited); }
    List<ParsedAnnotation> getAnnotations() { return List.copyOf(docAnnotations.values()); }
    List<ParsedDocument> getResolvedInherits() { return List.copyOf(resolvedInherits.values()); }
    List<ParsedField> getFields() { return List.copyOf(docFields.values()); }
    List<ParsedStruct> getStructs() { return List.copyOf(docStructs.values()); }
    ParsedStruct getStruct(String name) { return docStructs.get(name); }
    ParsedAnnotation getAnnotation(String name) { return docAnnotations.get(name); }

    void inherit(String other) { inherited.add(other); }

    void addField(ParsedField field) {
        String fieldName = field.name();
        verifyThat(! docFields.containsKey(fieldName), "already has field", fieldName);
        docFields.put(fieldName, field);
    }

    void addStruct(ParsedStruct struct) {
        String sName = struct.name();
        verifyThat(! docStructs.containsKey(sName), "already has struct", sName);
        docStructs.put(sName, struct);
        struct.tagOwner(name());
    }

    void addAnnotation(ParsedAnnotation annotation) {
        String annName = annotation.name();
        verifyThat(! docAnnotations.containsKey(annName), "already has annotation", annName);
        docAnnotations.put(annName, annotation);
        annotation.tagOwner(name());
    }

    public String toString() { return "document " + name(); }

    void resolveInherit(String name, ParsedDocument parsed) {
        verifyThat(inherited.contains(name), "resolveInherit for non-inherited name", name);
        verifyThat(name.equals(parsed.name()), "resolveInherit name mismatch for", name);
        verifyThat(! resolvedInherits.containsKey(name), "double resolveInherit for", name);
        resolvedInherits.put(name, parsed);
    }
}

