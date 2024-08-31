// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

package com.yahoo.vespasignificance;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.yahoo.language.significance.impl.DocumentFrequencyFile;
import com.yahoo.language.significance.impl.SignificanceModelFile;
import io.airlift.compress.zstd.ZstdInputStream;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Path;
import java.util.HashMap;

import static org.junit.jupiter.api.Assertions.*;

/**
 * @author MariusArhaug
 */
public class SignificanceModelGeneratorTest {

    private ObjectMapper objectMapper = new ObjectMapper();

    @TempDir
    private Path tempDir;

    private ClientParameters.Builder createParameters(String inputPath, String outputPath, String field, String language, String docType, String zstCompression) {
        tempDir.toFile().mkdirs();

        return new ClientParameters.Builder()
                .setInputFile("src/test/files/" + inputPath)
                .setOutputFile(tempDir.resolve(outputPath).toString())
                .setField(field)
                .setLanguage(language)
                .setDocType(docType)
                .setZstCompression(zstCompression);
    }

    private SignificanceModelGenerator createSignificanceModelGenerator(ClientParameters params) {
        return new SignificanceModelGenerator(params);
    }

    @Test
    void testGenerateSimpleFile() throws IOException {
        String inputPath = "no.jsonl";
        String outputPath = "output.json";
        ClientParameters params = createParameters(inputPath, outputPath,  "text", "nb", "nb", "false").build();
        SignificanceModelGenerator generator = createSignificanceModelGenerator(params);
        generator.generate();

        File outputFile = new File(tempDir.resolve(outputPath).toString());
        assertTrue(outputFile.exists());

        SignificanceModelFile modelFile = objectMapper.readValue(outputFile, SignificanceModelFile.class);

        HashMap<String, DocumentFrequencyFile> languages = modelFile.languages();
        assertEquals(1, languages.size());

        assertTrue(languages.containsKey("nb"));

        DocumentFrequencyFile documentFrequencyFile = languages.get("nb");

        assertEquals(3, documentFrequencyFile.frequencies().get("fra"));
        assertEquals(3, documentFrequencyFile.frequencies().get("skriveform"));
        assertEquals(3, documentFrequencyFile.frequencies().get("kategori"));
        assertEquals(3, documentFrequencyFile.frequencies().get("eldre"));

    }

    @Test
    void testGenerateSimpleFileWithZST() throws IOException {
        String inputPath = "no.jsonl";
        ClientParameters params1 = createParameters(inputPath, "output.json",  "text", "nb", "nb", "true").build();

        // Throws exception when outputfile does not have .zst extension when using zst compression
        assertThrows(IllegalArgumentException.class, () -> createSignificanceModelGenerator(params1));

        String outputPath = "output.json.zst";
        ClientParameters params = createParameters(inputPath, outputPath,  "text", "nb", "nb", "true").build();

        SignificanceModelGenerator generator = createSignificanceModelGenerator(params);
        generator.generate();

        File outputFile = new File(tempDir.resolve(outputPath ).toString());
        assertTrue(outputFile.exists());

        InputStream in = new ZstdInputStream(new FileInputStream(outputFile));

        SignificanceModelFile modelFile = objectMapper.readValue(in, SignificanceModelFile.class);

        HashMap<String, DocumentFrequencyFile> languages = modelFile.languages();
        assertEquals(1, languages.size());

        assertTrue(languages.containsKey("nb"));

        DocumentFrequencyFile documentFrequencyFile = languages.get("nb");

        assertEquals(3, documentFrequencyFile.frequencies().get("fra"));
        assertEquals(3, documentFrequencyFile.frequencies().get("skriveform"));
        assertEquals(3, documentFrequencyFile.frequencies().get("kategori"));
        assertEquals(3, documentFrequencyFile.frequencies().get("eldre"));

    }

    @Test
    void testGenerateFileWithMultipleLanguages() throws IOException {
        String inputPath = "no.jsonl";
        String outputPath = "output.json";
        ClientParameters params1 = createParameters(inputPath, outputPath, "text", "nb", "nb", "false").build();
        SignificanceModelGenerator generator = createSignificanceModelGenerator(params1);
        generator.generate();

        File outputFile = new File(tempDir.resolve(outputPath).toString());
        assertTrue(outputFile.exists());

        String inputPath2 = "en.jsonl";
        ClientParameters params2 = createParameters(inputPath2,  outputPath, "text", "en", "en", "false").build();
        generator = createSignificanceModelGenerator(params2);
        generator.generate();

        outputFile = new File(tempDir.resolve(outputPath).toString());
        assertTrue(outputFile.exists());

        SignificanceModelFile modelFile = objectMapper.readValue(outputFile, SignificanceModelFile.class);

        HashMap<String, DocumentFrequencyFile> languages = modelFile.languages();

        assertEquals(2, languages.size());

        assertTrue(languages.containsKey("nb"));
        assertTrue(languages.containsKey("en"));

        DocumentFrequencyFile nb = languages.get("nb");
        DocumentFrequencyFile en = languages.get("en");

        assertEquals(3, nb.documentCount());
        assertEquals(3, en.documentCount());

        assertEquals(3, nb.frequencies().get("fra"));
        assertEquals(3, nb.frequencies().get("skriveform"));

        assertEquals(3, en.frequencies().get("some"));
        assertEquals(3, en.frequencies().get("wiki"));

    }

    @Test
    void testOverwriteExistingDocumentFrequencyLanguage() throws IOException {
        String inputPath = "no.jsonl";
        String outputPath = "output.json";
        ClientParameters params1 = createParameters(inputPath, outputPath,  "text", "nb", "nb", "false").build();
        SignificanceModelGenerator generator = createSignificanceModelGenerator(params1);
        generator.generate();

        File outputFile = new File(tempDir.resolve(outputPath).toString());
        assertTrue(outputFile.exists());

        SignificanceModelFile preUpdatedFile = objectMapper.readValue(outputFile, SignificanceModelFile.class)
                ;
        HashMap<String, DocumentFrequencyFile> oldLanguages = preUpdatedFile.languages();
        assertEquals(1, oldLanguages.size());

        assertTrue(oldLanguages.containsKey("nb"));

        DocumentFrequencyFile oldDf = oldLanguages.get("nb");

        assertEquals(3, oldDf.frequencies().get("fra"));
        assertEquals(3, oldDf.frequencies().get("skriveform"));
        assertFalse(oldDf.frequencies().containsKey("nytt"));

        String inputPath2 = "no_2.jsonl";
        ClientParameters params2 = createParameters(inputPath2, outputPath, "text", "nb", "nb", "false").build();
        SignificanceModelGenerator generator2 = createSignificanceModelGenerator(params2);
        generator2.generate();

        outputFile = new File(tempDir.resolve(outputPath).toString());
        assertTrue(outputFile.exists());

        SignificanceModelFile modelFile = objectMapper.readValue(outputFile, SignificanceModelFile.class);

        HashMap<String, DocumentFrequencyFile> languages = modelFile.languages();

        assertEquals(1, languages.size());

        assertTrue(languages.containsKey("nb"));

        DocumentFrequencyFile df = languages.get("nb");

        assertEquals(2, df.frequencies().get("fra"));
        assertEquals(3, df.frequencies().get("skriveform"));
        assertTrue(df.frequencies().containsKey("nytt"));
        assertEquals(2, df.frequencies().get("nytt"));
    }

    @Test
    void testGenerateFileWithMultipleLanguagesForSingleDocumentFrequency() throws IOException {
        String inputPath = "no.jsonl";
        String outputPath = "output.json";
        ClientParameters params = createParameters(inputPath, outputPath,  "text", "nb,un", "nb", "false").build();
        SignificanceModelGenerator generator = createSignificanceModelGenerator(params);
        generator.generate();

        File outputFile = new File(tempDir.resolve(outputPath).toString());
        assertTrue(outputFile.exists());

        SignificanceModelFile modelFile = objectMapper.readValue(outputFile, SignificanceModelFile.class);

        HashMap<String, DocumentFrequencyFile> languages = modelFile.languages();
        assertEquals(1, languages.size());

        assertTrue(languages.containsKey("nb,un"));

        DocumentFrequencyFile documentFrequencyFile = languages.get("nb,un");

        assertEquals(3, documentFrequencyFile.frequencies().get("fra"));
        assertEquals(3, documentFrequencyFile.frequencies().get("skriveform"));
        assertEquals(3, documentFrequencyFile.frequencies().get("kategori"));
        assertEquals(3, documentFrequencyFile.frequencies().get("eldre"));
    }
}
