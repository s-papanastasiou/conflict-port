// Pre-analysis: model Zortech's software-FP helpers (segment 2007) as call fixups so the
// decompiler shows plain double arithmetic.
// Convention: operand A (double) in AX:BX:CX:DX (AX = most significant word);
// operand B (double) on the stack above the far return address, popped by callee (retf 8).
// Results in AX:BX:CX:DX (double), AX (int), DX:AX (long/float). dcmp/dtst return flags.
import ghidra.app.script.GhidraScript;
import ghidra.program.database.SpecExtension;
import ghidra.program.model.listing.Function;

public class ZortechFloat extends GhidraScript {
    static final String A = "local a:8 = (zext(AX) << 48) | (zext(BX) << 32) | (zext(CX) << 16) | zext(DX);\n";
    static final String B = "local sp4:2 = SP + 4; local sa:4 = segment(SS, sp4); local b:8 = *[ram]:8 sa;\n";
    static final String RD = "AX = r(6); local t1:8 = r >> 32; BX = t1:2; local t2:8 = r >> 16; CX = t2:2; DX = r:2;\n";
    static final String RET = "SP = SP + 4;\n";       // pop far return address
    static final String RET8 = "SP = SP + 12;\n";     // ... plus 8 bytes of stack operand

    String fixup(String name, String body) {
        return "<callfixup name=\"" + name + "\"><pcode><body><![CDATA[\n" + body + "]]></body></pcode></callfixup>";
    }
    void bind(String addr, String name, String body) throws Exception {
        try { bind0(addr, name, body); } catch (Exception e) { printerr("FIXUP " + name + ": " + e.getMessage()); }
    }
    void bind0(String addr, String name, String body) throws Exception {
        new SpecExtension(currentProgram).addReplaceCompilerSpecExtension(fixup(name, body), monitor);
        Function f = getFunctionAt(toAddr(addr));
        if (f == null) f = createFunction(toAddr(addr), name);
        f.setName(name, ghidra.program.model.symbol.SourceType.USER_DEFINED);
        f.setCallFixup(name);
    }
    @Override
    public void run() throws Exception {
        bind("2007:01b9", "_dadd", A + B + "local r:8 = b f+ a;\n" + RD + RET8);
        bind("2007:0167", "_dsub", A + B + "local r:8 = b f- a;\n" + RD + RET8);
        bind("2007:04d6", "_dmul", A + B + "local r:8 = b f* a;\n" + RD + RET8);
        bind("2007:0390", "_ddiv", A + B + "local r:8 = b f/ a;\n" + RD + RET8);
        // flags reflect sign(B - A); OF cleared, so jg/jl/jng/jnl test B vs A
        bind("2007:032f", "_dcmp", A + B + "ZF = b f== a; SF = b f< a; OF = 0; CF = 0;\n" + RET8);
        bind("2007:015c", "_dtst", A + "ZF = a f== int2float(0:4);\n" + RET);
        bind("2007:0686", "_itod", "local r:8 = int2float(AX);\n" + RD + RET);
        bind("2007:06b6", "_ltod", "local l:4 = (zext(DX) << 16) | zext(AX); local r:8 = int2float(l);\n" + RD + RET);
        bind("2007:06fa", "_ftod", "local f:4 = (zext(DX) << 16) | zext(AX); local r:8 = float2float(f);\n" + RD + RET);
        bind("2007:0747", "_dtoi", A + "AX = trunc(a);\n" + RET);
        bind("2007:079e", "_dtol", A + "local l:4 = trunc(a); AX = l:2; DX = l(2);\n" + RET);
        bind("2007:07f6", "_dtof", A + "local f:4 = float2float(a); AX = f:2; DX = f(2);\n" + RET);
        // 32-bit compare (2094:00d6): flags from comparing DX:AX with CX:BX, as a signed `cmp`.
        bind("2094:00d6", "_lcmp", "local a:4 = (zext(DX) << 16) | zext(AX); local b:4 = (zext(CX) << 16) | zext(BX);\n"
                + "ZF = a == b; SF = a s< b; OF = 0; CF = a < b;\n" + RET);
        println("Zortech FP fixups installed");
    }
}
