package ai.vespa.schemals.schemadocument;

import java.io.PrintStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

import org.eclipse.lsp4j.Diagnostic;
import org.eclipse.lsp4j.DiagnosticSeverity;
import org.eclipse.lsp4j.Position;
import org.eclipse.lsp4j.Range;
import org.eclipse.lsp4j.VersionedTextDocumentIdentifier;

import ai.vespa.schemals.SchemaDiagnosticsHandler;

import ai.vespa.schemals.context.ParseContext;
import ai.vespa.schemals.common.FileUtils;
import ai.vespa.schemals.common.SchemaDiagnostic;
import ai.vespa.schemals.common.SchemaDiagnostic.DiagnosticCode;
import ai.vespa.schemals.common.StringUtils;
import ai.vespa.schemals.index.SchemaIndex;
import ai.vespa.schemals.index.Symbol;
import ai.vespa.schemals.index.Symbol.SymbolType;
import ai.vespa.schemals.parser.SchemaParser;
import ai.vespa.schemals.parser.SubLanguageData;
import ai.vespa.schemals.parser.ast.indexingElm;
import ai.vespa.schemals.parser.ParseException;
import ai.vespa.schemals.parser.Node;


import ai.vespa.schemals.parser.indexinglanguage.IndexingParser;
import ai.vespa.schemals.schemadocument.parser.Identifier;
import ai.vespa.schemals.schemadocument.resolvers.AnnotationReferenceResolver;
import ai.vespa.schemals.schemadocument.resolvers.DocumentReferenceResolver;
import ai.vespa.schemals.schemadocument.resolvers.InheritanceResolver;
import ai.vespa.schemals.schemadocument.resolvers.ResolverTraversal;
import ai.vespa.schemals.schemadocument.resolvers.StructFieldDefinitionResolver;
import ai.vespa.schemals.schemadocument.resolvers.TypeNodeResolver;
import ai.vespa.schemals.tree.CSTUtils;
import ai.vespa.schemals.tree.SchemaNode;
import ai.vespa.schemals.tree.SchemaNode.LanguageType;
import ai.vespa.schemals.tree.indexinglanguage.ILUtils;

public class SchemaDocument implements DocumentManager {
    public record ParseResult(ArrayList<Diagnostic> diagnostics, Optional<SchemaNode> CST) {
        public static ParseResult parsingFailed(ArrayList<Diagnostic> diagnostics) {
            return new ParseResult(diagnostics, Optional.empty());
        }
    }

    private PrintStream logger;
    private SchemaDiagnosticsHandler diagnosticsHandler;
    private SchemaIndex schemaIndex;
    private SchemaDocumentScheduler scheduler;

    private String fileURI = "";
    private Integer version;
    private boolean isOpen = false;
    private String content = "";
    private String schemaDocumentIdentifier = null;
    
    private SchemaNode CST;

    private SchemaDocumentLexer lexer = new SchemaDocumentLexer();

    public SchemaDocument(PrintStream logger, SchemaDiagnosticsHandler diagnosticsHandler, SchemaIndex schemaIndex, SchemaDocumentScheduler scheduler, String fileURI) {
        this.logger = logger;
        this.diagnosticsHandler = diagnosticsHandler;
        this.schemaIndex = schemaIndex;
        this.fileURI = fileURI;
        this.scheduler = scheduler;
    }
    
    public ParseContext getParseContext(String content) {
        ParseContext context = new ParseContext(content, this.logger, this.fileURI, this.schemaIndex, this.scheduler);
        context.useDocumentIdentifiers();
        return context;
    }

    @Override
    public void reparseContent() {
        if (this.content != null) {
            updateFileContent(this.content, this.version);
        }
    }

    @Override
    public void updateFileContent(String content, Integer version) {
        this.version = version;
        updateFileContent(content);
    }

    @Override
    public void updateFileContent(String content) {
        this.content = content;
        schemaIndex.clearDocument(fileURI);

        logger.println("Parsing: " + fileURI);
        ParseContext context = getParseContext(content);
        var parsingResult = parseContent(context);

        parsingResult.diagnostics().addAll(verifyFileName());

        diagnosticsHandler.publishDiagnostics(fileURI, parsingResult.diagnostics());

        if (parsingResult.CST().isPresent()) {
            this.CST = parsingResult.CST().get();
            lexer.setCST(CST);

            logger.println("======== CST for file: " + fileURI + " ========");
     
            //CSTUtils.printTree(logger, CST);
        }



        //schemaIndex.dumpIndex();

    }

    private List<Diagnostic> verifyFileName() {
        List<Diagnostic> ret = new ArrayList<>();

        List<Symbol> schemaSymbols = schemaIndex.getSymbolsByType(SymbolType.SCHEMA);

        Symbol schemaIdentifier = null;

        for (Symbol symbol : schemaSymbols) {
            if (symbol.getFileURI().equals(fileURI)) {
                schemaIdentifier = symbol;
                break;
            }
        }

        if (schemaIdentifier != null) {
            schemaDocumentIdentifier = schemaIdentifier.getShortIdentifier();

            if (!getFileName().equals(schemaDocumentIdentifier + ".sd")) {
                ret.add(new SchemaDiagnostic.Builder()
                    .setRange(schemaIdentifier.getNode().getRange())
                    .setMessage("Schema " + schemaDocumentIdentifier + " should be defined in a file with the name: " + schemaDocumentIdentifier + ".sd. File name is: " + getFileName())
                    .setCode(DiagnosticCode.SCHEMA_NAME_SAME_AS_FILE)
                    .setSeverity(DiagnosticSeverity.Error)
                    .build()
                );
            }
        }

        return ret;
    }

    @Override
    public boolean getIsOpen() { return isOpen; }

    @Override
    public boolean setIsOpen(boolean value) {
        isOpen = value;
        return isOpen;
    }

    public String getSchemaIdentifier() {
        return schemaDocumentIdentifier;
    }

    public String getFileURI() {
        return fileURI;
    }

    public Integer getVersion() {
        return version;
    }

    public VersionedTextDocumentIdentifier getVersionedTextDocumentIdentifier() {
        return new VersionedTextDocumentIdentifier(fileURI, null);
    }

    public String getFilePath() {
        int splitPos = fileURI.lastIndexOf('/');
        return fileURI.substring(0, splitPos + 1);
    }

    public String getFileName() {
        return FileUtils.fileNameFromPath(fileURI);
    }

    public SchemaNode getRootNode() {
        return CST;
    }


    public static ParseResult parseContent(ParseContext context) {
        CharSequence sequence = context.content();

        SchemaParser parserStrict = new SchemaParser(context.logger(), context.fileURI(), sequence);
        parserStrict.setParserTolerant(false);

        ArrayList<Diagnostic> diagnostics = new ArrayList<Diagnostic>();

        try {
            parserStrict.Root();
        } catch (ParseException e) {

            Node.TerminalNode node = e.getToken();

            Range range = CSTUtils.getNodeRange(node);
            String message = e.getMessage();

            diagnostics.add(new SchemaDiagnostic.Builder()
                .setRange(range)
                .setMessage(message)
                .setSeverity(DiagnosticSeverity.Error)
                .build());


        } catch (IllegalArgumentException e) {
            // Complex error, invalidate the whole document

            diagnostics.add(new SchemaDiagnostic.Builder()
                .setRange(
                    new Range(
                        new Position(0, 0),
                        new Position((int)context.content().lines().count() - 1, 0)
                    )
                )
                .setMessage(e.getMessage())
                .setSeverity(DiagnosticSeverity.Error)
                .build()
            );
            return ParseResult.parsingFailed(diagnostics);
        }

        SchemaParser parserFaultTolerant = new SchemaParser(context.fileURI(), sequence);
        try {
            parserFaultTolerant.Root();
        } catch (ParseException e) {
            // Ignore
        } catch (IllegalArgumentException e) {
            // Ignore
        }

        Node node = parserFaultTolerant.rootNode();

        var tolerantResult = parseCST(node, context);

        diagnostics.addAll(InheritanceResolver.resolveInheritances(context));

        for (SchemaNode typeNode : context.unresolvedTypeNodes()) {
            TypeNodeResolver.resolveType(context, typeNode.getSymbol());
        }
        
        context.clearUnresolvedTypeNodes();

        for (SchemaNode annotationReferenceNode : context.unresolvedAnnotationReferenceNodes()) {
            AnnotationReferenceResolver.resolveAnnotationReference(context, annotationReferenceNode.getSymbol());
        }

        context.clearUnresolvedAnnotationReferenceNodes();

        
        if (tolerantResult.CST().isPresent()) {
            //diagnostics.addAll(RankExpressionSymbolResolver.resolveRankExpressionReferences(tolerantResult.CST().get(), context));

            diagnostics.addAll(StructFieldDefinitionResolver.resolve(context, tolerantResult.CST().get()));

            diagnostics.addAll(ResolverTraversal.traverse(context, tolerantResult.CST().get()));

            diagnostics.addAll(DocumentReferenceResolver.resolveDocumentReferences(context));
        }

        diagnostics.addAll(tolerantResult.diagnostics());

        return new ParseResult(diagnostics, tolerantResult.CST());
    }

    private static ArrayList<Diagnostic> traverseCST(SchemaNode node, ParseContext context) {


        ArrayList<Diagnostic> ret = new ArrayList<>();

        if (node.containsOtherLanguageData(LanguageType.INDEXING)) {
            SchemaNode indexingNode = parseIndexingScript(node, context, ret);
            if (indexingNode != null) {
                node.addChild(indexingNode);
            }
        }

        if (node.containsOtherLanguageData(LanguageType.RANK_EXPRESSION)) {
            // Range nodeRange = node.getRange();
            // String nodeString = node.get(0).get(0).getText();
            // Position rankExpressionStart = CSTUtils.addPositions(nodeRange.getStart(), new Position(0, nodeString.length()));

            SchemaRankExpressionParser.embedCST(context, node, ret);
        }

        for (Identifier identifier : context.identifiers()) {
            ret.addAll(identifier.identify(node));
        }

        for (int i = 0; i < node.size(); ++i) {
            ret.addAll(traverseCST(node.get(i), context));
        }

        return ret;
    }

    public static ParseResult parseCST(Node node, ParseContext context) {
        if (node == null) {
            return ParseResult.parsingFailed(new ArrayList<>());
        }
        SchemaNode CST = new SchemaNode(node);
        var errors = traverseCST(CST, context);
        return new ParseResult(errors, Optional.of(CST));
    }

    private static SchemaNode parseIndexingScript(SchemaNode indexingElmNode, ParseContext context, ArrayList<Diagnostic> diagnostics) {
        SubLanguageData script = indexingElmNode.getILScript();
        if (script == null) return null;

        Position indexingStart = indexingElmNode.getRange().getStart();
        CharSequence sequence = script.content();
        IndexingParser parser = new IndexingParser(context.logger(), context.fileURI(), sequence);
        parser.setParserTolerant(false);

        Position scriptStart = StringUtils.positionAddOffset(context.content(), indexingStart, script.leadingStripped());

        try {
            var expression = parser.root();

            // store the expression to be checked later
            ((indexingElm)indexingElmNode.getOriginalSchemaNode()).expression = expression;

            return new SchemaNode(parser.rootNode(), scriptStart);
        } catch(ai.vespa.schemals.parser.indexinglanguage.ParseException pe) {
            try {
                Range range = ILUtils.getNodeRange(pe.getToken());
                range.setStart(CSTUtils.addPositions(scriptStart, range.getStart()));
                range.setEnd(CSTUtils.addPositions(scriptStart, range.getEnd()));

                diagnostics.add(new SchemaDiagnostic.Builder()
                    .setRange(range)
                    .setMessage(pe.getMessage())
                    .setSeverity(DiagnosticSeverity.Error)
                    .build());
            } catch(Exception e) {
                // ignore
            }
        } catch(IllegalArgumentException ex) {
            diagnostics.add(new SchemaDiagnostic.Builder()
                .setRange(indexingElmNode.getRange())
                .setMessage(ex.getMessage())
                .setSeverity(DiagnosticSeverity.Error)
                .build());
        }

        return null;
    }


    public String toString() {
        String openString = getIsOpen() ? " [OPEN]" : "";
        return getFileURI() + openString;
    }

	@Override
	public String getCurrentContent() {
        return this.content;
	}

	@Override
	public SchemaDocumentLexer lexer() {
        return this.lexer;
	}
}
