// Apply names from re/symbols.tsv. Usage: -postScript ApplySymbols.java <symbols.tsv>
// Columns: address, name, func|data, [comment], [C signature for funcs].
// Addresses "SSSS:OOOO" are Ghidra segmented addresses; bare "OOOO" is a DGROUP (228e) offset.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;
import java.io.*;
import java.nio.file.*;
import ghidra.app.cmd.function.ApplyFunctionSignatureCmd;
import ghidra.app.util.cparser.C.CParser;
import ghidra.program.model.data.FunctionDefinitionDataType;

public class ApplySymbols extends GhidraScript {
    // "short[8][8][10]", "char[2500]", "float", "ushort[25]" -> data at addr
    void applyType(Address addr, String spec) throws Exception {
        String base = spec.contains("[") ? spec.substring(0, spec.indexOf('[')) : spec;
        ghidra.program.model.data.DataType dt = switch (base) {
            case "short" -> ghidra.program.model.data.ShortDataType.dataType;
            case "ushort" -> ghidra.program.model.data.UnsignedShortDataType.dataType;
            case "char" -> ghidra.program.model.data.CharDataType.dataType;
            case "byte" -> ghidra.program.model.data.ByteDataType.dataType;
            case "float" -> ghidra.program.model.data.FloatDataType.dataType;
            case "long" -> ghidra.program.model.data.LongDataType.dataType;
            default -> throw new IllegalArgumentException("unknown base type " + base);
        };
        java.util.List<Integer> dims = new java.util.ArrayList<>();
        java.util.regex.Matcher m = java.util.regex.Pattern.compile("\\[(\\d+)\\]").matcher(spec);
        while (m.find()) dims.add(Integer.parseInt(m.group(1)));
        for (int i = dims.size() - 1; i >= 0; --i)
            dt = new ghidra.program.model.data.ArrayDataType(dt, dims.get(i), dt.getLength());
        clearListing(addr, addr.add(dt.getLength() - 1));
        createData(addr, dt);
    }

    @Override
    public void run() throws Exception {
        int n = 0;
        for (String line : Files.readAllLines(Paths.get(getScriptArgs()[0]))) {
            if (line.isBlank() || line.startsWith("#")) continue;
            String[] f = line.split("\t");
            String a = f[0].contains(":") ? f[0] : "228e:" + f[0];
            Address addr = toAddr(a);
            if (f[2].equals("func")) {
                Function fn = getFunctionAt(addr);
                if (fn == null) fn = createFunction(addr, f[1]);
                if (fn == null) { printerr("no function at " + a); continue; }
                fn.setName(f[1], SourceType.USER_DEFINED);
                if (f.length > 4 && !f[4].isBlank()) {
                    ghidra.program.model.data.DataType dt = new CParser(currentProgram.getDataTypeManager()).parse(f[4] + ";");
                    FunctionDefinitionDataType sig = dt instanceof FunctionDefinitionDataType ? (FunctionDefinitionDataType) dt : null;
                    if (sig == null) printerr("bad signature: " + f[4]);
                    else new ApplyFunctionSignatureCmd(addr, sig, SourceType.USER_DEFINED).applyTo(currentProgram, monitor);
                }
            } else {
                createLabel(addr, f[1], true, SourceType.USER_DEFINED);
                if (f.length > 4 && !f[4].isBlank()) applyType(addr, f[4]);
            }
            if (f.length > 3 && !f[3].isBlank()) setPlateComment(addr, f[3]);
            n++;
        }
        println("Applied " + n + " symbols");
    }
}
