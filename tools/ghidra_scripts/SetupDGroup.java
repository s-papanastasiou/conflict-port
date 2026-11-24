// Pre-analysis: Zortech large-model startup sets DS=SS=DGROUP (file para 0x128e -> 0x228e in Ghidra).
// Mark DGROUP blocks as data and assume DS/SS = DGROUP in every code block.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.ProgramContext;
import ghidra.program.model.mem.MemoryBlock;
import java.math.BigInteger;

public class SetupDGroup extends GhidraScript {
    static final int DGROUP = 0x228e;
    @Override
    public void run() throws Exception {
        ProgramContext ctx = currentProgram.getProgramContext();
        Register ds = ctx.getRegister("DS"), ss = ctx.getRegister("SS");
        for (MemoryBlock b : currentProgram.getMemory().getBlocks()) {
            if (b.getName().equals("HEADER")) continue;
            long seg = b.getStart().getOffset() >> 4;  // segmented addr offset is linear
            if (b.getStart().toString().startsWith("228e:")) {
                b.setExecute(false);
                b.setName(b.getName().equals("DATA") ? "BSS_DGROUP" : "DGROUP");
                continue;
            }
            ctx.setValue(ds, b.getStart(), b.getEnd(), BigInteger.valueOf(DGROUP));
            ctx.setValue(ss, b.getStart(), b.getEnd(), BigInteger.valueOf(DGROUP));
        }
        println("DGROUP setup done");
    }
}
