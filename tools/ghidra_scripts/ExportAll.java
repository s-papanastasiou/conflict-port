// Export function inventory and decompiled C for every function.
// Usage (headless): -postScript ExportAll.java <outdir>
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.mem.*;
import java.io.*;

public class ExportAll extends GhidraScript {
    @Override
    public void run() throws Exception {
        String out = getScriptArgs().length > 0 ? getScriptArgs()[0] : "/tmp/export";
        new File(out).mkdirs();
        try (PrintWriter blocks = new PrintWriter(new File(out, "blocks.tsv"))) {
            for (MemoryBlock b : currentProgram.getMemory().getBlocks())
                blocks.printf("%s\t%s\t%s\t%d\t%s%s%s%n", b.getName(), b.getStart(), b.getEnd(), b.getSize(),
                        b.isRead() ? "r" : "-", b.isWrite() ? "w" : "-", b.isExecute() ? "x" : "-");
        }
        DecompInterface di = new DecompInterface();
        DecompileOptions opts = new DecompileOptions();
        opts.grabFromProgram(currentProgram);
        // Near-data model: every small integer is also a valid DS address, so pointer
        // inference turns key codes and counters into bogus string references.
        opts.setInferConstantPointers(false);
        di.setOptions(opts);
        di.openProgram(currentProgram);
        FunctionManager fm = currentProgram.getFunctionManager();
        try (PrintWriter inv = new PrintWriter(new File(out, "functions.tsv"));
             PrintWriter c = new PrintWriter(new File(out, "decompiled.c"));
             PrintWriter edges = new PrintWriter(new File(out, "callgraph.tsv"))) {
            inv.println("entry\tname\tsize\tcallers\tcallees\tsignature");
            for (Function f : fm.getFunctions(true)) {
                if (monitor.isCancelled()) break;
                inv.printf("%s\t%s\t%d\t%d\t%d\t%s%n", f.getEntryPoint(), f.getName(), f.getBody().getNumAddresses(),
                        f.getCallingFunctions(monitor).size(), f.getCalledFunctions(monitor).size(), f.getPrototypeString(false, false));
                for (Function callee : f.getCalledFunctions(monitor))
                    edges.printf("%s\t%s\t%s\t%s%n", f.getEntryPoint(), f.getName(), callee.getEntryPoint(), callee.getName());
                DecompileResults r = di.decompileFunction(f, 60, monitor);
                c.printf("// ===== %s @ %s (%d bytes)%n", f.getName(), f.getEntryPoint(), f.getBody().getNumAddresses());
                if (r != null && r.decompileCompleted()) c.println(r.getDecompiledFunction().getC());
                else c.println("// decompile failed: " + (r == null ? "null" : r.getErrorMessage()));
            }
        }
        di.dispose();
        println("Exported " + fm.getFunctionCount() + " functions to " + out);
    }
}
