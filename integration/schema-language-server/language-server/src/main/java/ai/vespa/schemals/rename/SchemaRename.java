package ai.vespa.schemals.rename;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Optional;

import org.eclipse.lsp4j.MessageType;
import org.eclipse.lsp4j.RenameFile;
import org.eclipse.lsp4j.TextEdit;
import org.eclipse.lsp4j.WorkspaceEdit;

import ai.vespa.schemals.context.EventContext;
import ai.vespa.schemals.context.EventPositionContext;
import ai.vespa.schemals.index.Symbol;
import ai.vespa.schemals.index.Symbol.SymbolType;
import ai.vespa.schemals.schemadocument.DocumentManager;
import ai.vespa.schemals.schemadocument.SchemaDocument;
import ai.vespa.schemals.tree.CSTUtils;
import ai.vespa.schemals.tree.SchemaNode;
import ai.vespa.schemals.workspaceEdit.SchemaTextDocumentEdit;
import ai.vespa.schemals.workspaceEdit.SchemaWorkspaceEdit;

public class SchemaRename {

    private static ArrayList<SchemaTextDocumentEdit> createTextDocumentEditsFromSymbols(EventContext context, List<Symbol> symbols, String newName) {
        HashMap<String, SchemaTextDocumentEdit> documentEdits = new HashMap<>();

        for (Symbol symbol : symbols) {

            if (!documentEdits.containsKey(symbol.getFileURI())) {
                SchemaDocument document = context.scheduler.getSchemaDocument(symbol.getFileURI());
                if (document == null) return new ArrayList<>();
                SchemaTextDocumentEdit documentEdit = new SchemaTextDocumentEdit(document.getVersionedTextDocumentIdentifier());
                documentEdits.put(documentEdit.getFileURI(), documentEdit);
            }

            SchemaTextDocumentEdit documentEdit = documentEdits.get(symbol.getFileURI());
            documentEdit.add(new TextEdit(symbol.getNode().getRange(), newName));
        }

        return new ArrayList<SchemaTextDocumentEdit>(documentEdits.values());
    }

    public static WorkspaceEdit rename(EventPositionContext context, String newName) {

        SchemaNode node = CSTUtils.getSymbolAtPosition(context.document.getRootNode(), context.position);
        if (node == null || !node.hasSymbol()) {
            return null;
        }

        // CASES: rename schema / document, field, struct, funciton

        SymbolType type = node.getSymbol().getType();
        if (type == SymbolType.DOCUMENT) {
            type = SymbolType.SCHEMA;
        }

        Symbol symbol = context.schemaIndex.findSymbol(context.document.getFileURI(), type, node.getText());
        if (symbol == null) {
            // TODO: later maybe search for schema symbol in the document. And use that insted, if the symbol type is DOCUMENT
            Optional<Symbol> result = context.schemaIndex.findDefinitionOfReference(node.getSymbol());

            if (result.isEmpty()) {
                context.messageHandler.sendMessage(MessageType.Error, "Could not find the symbol definition for: " + node.getText());
                return null;
            }
            symbol = result.get();
        }

        SchemaDocument document = context.scheduler.getSchemaDocument(symbol.getFileURI());
        if (document == null) {
            context.logger.println("Symbol has a fileURI to a file not in index!");
            return null;
        }

        context.schemaIndex.dumpIndex(context.logger);

        
        ArrayList<Symbol> symbolOccurances = context.schemaIndex.findSymbolReferences(symbol);
        symbolOccurances.add(symbol);
        
        if (type == SymbolType.SCHEMA) {
            return renameSchema(context, document, symbol, symbolOccurances, node.getText(), newName);
        }

        SchemaWorkspaceEdit workspaceEdits = new SchemaWorkspaceEdit();

        ArrayList<SchemaTextDocumentEdit> documentEdits = createTextDocumentEditsFromSymbols(context, symbolOccurances, newName);

        workspaceEdits.addTextDocumentEdits(documentEdits);
        
        return workspaceEdits.exportEdits();
    }

    private static WorkspaceEdit renameSchema(EventContext context, SchemaDocument document, Symbol symbol, ArrayList<Symbol> symbolOccurances, String oldName, String newName) {
        SchemaWorkspaceEdit workspaceEdits = new SchemaWorkspaceEdit();

        Symbol documentSymbol = context.schemaIndex.findSymbol(symbol.getFileURI(), SymbolType.DOCUMENT, oldName);

        if (documentSymbol != null) {
            symbolOccurances.add(documentSymbol);
            ArrayList<Symbol> documentReferences = context.schemaIndex.findSymbolReferences(documentSymbol);
            symbolOccurances.addAll(documentReferences);
        } else {
            ArrayList<Symbol> documentReferences = context.schemaIndex.findDocumentSymbolReferences(symbol);
            symbolOccurances.addAll(documentReferences);
        }


        String newFileURI = document.getFilePath() + newName + ".sd";

        // Check for name collision
        SchemaDocument nameCollitionDocument = context.schemaIndex.findSchemaDocumentWithFileURI(newFileURI);
        if (nameCollitionDocument != null) {
            context.messageHandler.sendMessage(MessageType.Error, "Cannot rename schema to '" + newName + "' since the name is already taken.");

            return null;
        }

        ArrayList<SchemaTextDocumentEdit> documentEdits = createTextDocumentEditsFromSymbols(context, symbolOccurances, newName);

        for (int i = 0; i < documentEdits.size(); i++) {
            if (documentEdits.get(i).getFileURI().equals(document.getFileURI())) {
                workspaceEdits.addTextDocumentEdit(documentEdits.get(i));

                documentEdits.remove(i);

                break;
            }
        }

        workspaceEdits.addResourceOperation(new RenameFile(document.getFileURI(), newFileURI));

        workspaceEdits.addTextDocumentEdits(documentEdits);

        return workspaceEdits.exportEdits();
    }

}
