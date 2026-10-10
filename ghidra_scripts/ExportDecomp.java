// Names functions from RTTI vftables and assert source-file strings, then decompiles
// functions to C files grouped by address. Args: <outDir> [startHex endHex]
// @category EEMod
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.app.decompiler.parallel.*;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.data.*;
import ghidra.program.model.mem.*;
import ghidra.util.task.TaskMonitor;
import java.io.*;
import java.util.*;
import java.util.regex.*;

public class ExportDecomp extends GhidraScript {

    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        File outDir = new File(args[0]);
        outDir.mkdirs();
        long lo = args.length > 2 ? Long.parseLong(args[1], 16) : 0;
        long hi = args.length > 2 ? Long.parseLong(args[2], 16) : Long.MAX_VALUE;

        File namesFile = new File(outDir.getParentFile(), "work/names.tsv");
        if (namesFile.exists()) nameFromTsv(namesFile);
        nameFromVftables();
        nameFromSourceStrings();
        nameFromDataStrings();

        FunctionManager fm = currentProgram.getFunctionManager();
        List<Function> funcs = new ArrayList<>();
        for (Function f : fm.getFunctions(true)) {
            long a = f.getEntryPoint().getOffset();
            if (a >= lo && a < hi && !f.isThunk() && !f.isExternal()) funcs.add(f);
        }
        println("Decompiling " + funcs.size() + " functions");

        // index of all functions
        try (PrintWriter idx = new PrintWriter(new FileWriter(new File(outDir, "functions.tsv"), true))) {
            for (Function f : funcs)
                idx.println(f.getEntryPoint() + "\t" + f.getName(true) + "\t" + f.getBody().getNumAddresses());
        }

        final Map<Long, String> results = Collections.synchronizedMap(new TreeMap<>());
        DecompilerCallback<Void> cb = new DecompilerCallback<Void>(currentProgram, new DecompileConfigurer() {
            public void configure(DecompInterface d) {
                DecompileOptions o = new DecompileOptions();
                d.setOptions(o);
                d.toggleCCode(true);
                d.toggleSyntaxTree(false);
                d.setSimplificationStyle("decompile");
            }
        }) {
            public Void process(DecompileResults r, TaskMonitor m) {
                Function f = r.getFunction();
                String c = r.decompileCompleted() ? r.getDecompiledFunction().getC() : "/* decompile failed: " + r.getErrorMessage() + " */\n";
                results.put(f.getEntryPoint().getOffset(), "// ==== " + f.getEntryPoint() + " " + f.getName(true) + "\n" + c + "\n");
                return null;
            }
        };
        cb.setTimeout(60);
        ParallelDecompiler.decompileFunctions(cb, funcs, monitor);
        cb.dispose();

        // write in 64KB-address buckets
        Map<Long, StringBuilder> buckets = new TreeMap<>();
        for (Map.Entry<Long, String> e : results.entrySet())
            buckets.computeIfAbsent(e.getKey() & ~0xFFFFL, k -> new StringBuilder()).append(e.getValue());
        for (Map.Entry<Long, StringBuilder> e : buckets.entrySet())
            try (Writer w = new OutputStreamWriter(new FileOutputStream(new File(outDir, String.format("%08x.c", e.getKey()))), "UTF-8")) {
                w.write(e.getValue().toString());
            }
        println("Done");
    }

    private boolean isDefault(Function f) {
        return f.getSymbol().getSource() == SourceType.DEFAULT || f.getName().startsWith("FUN_");
    }

    private void nameFromVftables() throws Exception {
        SymbolTable st = currentProgram.getSymbolTable();
        Listing listing = currentProgram.getListing();
        Memory mem = currentProgram.getMemory();
        int named = 0;
        SymbolIterator it = st.getSymbols("vftable");
        List<Symbol> syms = new ArrayList<>();
        while (it.hasNext()) syms.add(it.next());
        // shorter class names first is not meaningful; process in address order
        for (Symbol s : syms) {
            Namespace cls = s.getParentNamespace();
            Address a = s.getAddress();
            for (int i = 0; i < 400; i++) {
                Address slot = a.add(i * 4L);
                if (i > 0 && st.getPrimarySymbol(slot) != null && st.getPrimarySymbol(slot).getName().contains("meta")) break;
                long p;
                try { p = mem.getInt(slot) & 0xFFFFFFFFL; } catch (Exception e) { break; }
                Address fa = toAddr(p);
                if (!mem.getExecuteSet().contains(fa)) break;
                Function f = getFunctionAt(fa);
                if (f == null) {
                    try { disassemble(fa); f = createFunction(fa, null); } catch (Exception e) { }
                }
                if (f == null) break;
                if (f.isThunk()) f = f.getThunkedFunction(true);
                if (isDefault(f)) {
                    try {
                        f.setParentNamespace(cls);
                        f.setName("vf" + i, SourceType.ANALYSIS);
                        named++;
                    } catch (Exception e) { }
                }
            }
        }
        println("vftable-named functions: " + named);
    }

    // names.tsv: hexAddr \t ClassName \t method
    private void nameFromTsv(File f) throws Exception {
        SymbolTable st = currentProgram.getSymbolTable();
        int named = 0;
        try (BufferedReader br = new BufferedReader(new FileReader(f))) {
            String line;
            while ((line = br.readLine()) != null) {
                String[] p = line.split("\t");
                if (p.length < 3) continue;
                Address a = toAddr(Long.parseLong(p[0], 16));
                Function fn = getFunctionAt(a);
                if (fn == null) {
                    try { disassemble(a); fn = createFunction(a, null); } catch (Exception e) { }
                }
                if (fn == null || !isDefault(fn)) continue;
                try {
                    Namespace ns = st.getNamespace(p[1], null);
                    if (ns == null) ns = st.createClass(null, p[1], SourceType.ANALYSIS);
                    fn.setParentNamespace(ns);
                    fn.setName(p[2], SourceType.ANALYSIS);
                    named++;
                } catch (Exception e) { }
            }
        }
        println("tsv-named functions: " + named);
    }

    // "EECPResourceManager-Data" style save/load tags
    private void nameFromDataStrings() throws Exception {
        Pattern p = Pattern.compile("^(EE\\w+)-(\\w[\\w ]*)$");
        ReferenceManager rm = currentProgram.getReferenceManager();
        int named = 0;
        for (Data d : currentProgram.getListing().getDefinedData(true)) {
            if (!(d.getValue() instanceof String)) continue;
            Matcher m = p.matcher((String) d.getValue());
            if (!m.find()) continue;
            for (Reference r : rm.getReferencesTo(d.getAddress())) {
                Function f = getFunctionContaining(r.getFromAddress());
                if (f != null && isDefault(f)) {
                    try {
                        f.setName(m.group(1) + "_persist_" + f.getEntryPoint().toString(), SourceType.ANALYSIS);
                        named++;
                    } catch (Exception e) { }
                }
            }
        }
        println("data-string-named functions: " + named);
    }

    private void nameFromSourceStrings() throws Exception {
        Pattern p = Pattern.compile("([A-Za-z0-9_ ]+)\\.cpp$");
        ReferenceManager rm = currentProgram.getReferenceManager();
        int named = 0;
        for (Data d : currentProgram.getListing().getDefinedData(true)) {
            if (!(d.getValue() instanceof String)) continue;
            String s = (String) d.getValue();
            if (!s.startsWith("C:\\sssi")) continue;
            Matcher m = p.matcher(s);
            if (!m.find()) continue;
            String file = m.group(1).replace(' ', '_');
            for (Reference r : rm.getReferencesTo(d.getAddress())) {
                Function f = getFunctionContaining(r.getFromAddress());
                if (f != null && isDefault(f)) {
                    try {
                        f.setName(file + "_" + f.getEntryPoint().toString(), SourceType.ANALYSIS);
                        named++;
                    } catch (Exception e) { }
                }
            }
        }
        println("source-file-named functions: " + named);
    }
}
