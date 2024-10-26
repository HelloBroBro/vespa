package ai.vespa.schemals.lsp.common.command.commandtypes;

import java.util.List;

import ai.vespa.schemals.context.EventExecuteCommandContext;

public class RunVespaQuery implements SchemaCommand {

    public int getArity() {
        return -1;
    }

    public boolean setArguments(List<Object> arguments) {
        return true;
    }

    public Object execute(EventExecuteCommandContext context) {
        context.logger.info("Running Vespa query...");
        return null;
    }
    
}
