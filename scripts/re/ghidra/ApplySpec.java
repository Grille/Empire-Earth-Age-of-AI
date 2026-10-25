// Puts the spec into the program: local/work/spec_<build>.json (scripts/re/spec_ghidra.py) - its classes as
// structures and its enums under /spec, its functions named into their class's namespace (Class::member) with their
// convention and types, its places as labels, the docs as comments. The spec's names win over the program's. Args:
// <spec json>
// @category EEMod
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import com.google.gson.*;
import java.io.*;
import java.util.*;

public class ApplySpec extends GhidraScript {
    private DataTypeManager dtm;
    private final CategoryPath cat = new CategoryPath("/spec");
    private final Map<String, DataType> made = new HashMap<>();

    @Override
    protected void run() throws Exception {
        JsonObject spec = JsonParser.parseReader(new FileReader(getScriptArgs()[0])).getAsJsonObject();
        dtm = currentProgram.getDataTypeManager();
        int bad = 0;
        // the enums, then the structures: empty at their size first (pointers and embedded ones find them), then
        // their fields
        for (JsonElement e : spec.getAsJsonArray("enums")) {
            JsonObject o = e.getAsJsonObject();
            EnumDataType en = new EnumDataType(cat, str(o, "name"), 4);
            for (JsonElement v : o.getAsJsonArray("values"))
                en.add(v.getAsJsonArray().get(0).getAsString(), v.getAsJsonArray().get(1).getAsLong());
            en.setDescription(str(o, "doc"));
            made.put("enum:" + str(o, "name"), dtm.addDataType(en, DataTypeConflictHandler.REPLACE_HANDLER));
        }
        JsonArray structs = spec.getAsJsonArray("structs");
        for (JsonElement e : structs) {
            JsonObject o = e.getAsJsonObject();
            StructureDataType s = new StructureDataType(cat, str(o, "name"), o.get("size").getAsInt());
            s.setDescription(str(o, "doc"));
            made.put("struct:" + str(o, "name"), dtm.addDataType(s, DataTypeConflictHandler.REPLACE_HANDLER));
        }
        for (JsonElement e : structs) {
            JsonObject o = e.getAsJsonObject();
            Structure s = (Structure) made.get("struct:" + str(o, "name"));
            for (JsonElement fe : o.getAsJsonArray("fields")) {
                JsonObject f = fe.getAsJsonObject();
                int len = f.get("length").getAsInt();
                if (len == 0) continue;   // a place inside (block, code): its address only
                try {
                    s.replaceAtOffset(f.get("offset").getAsInt(), type(str(f, "type")), len, str(f, "name"), str(f, "doc"));
                } catch (Exception ex) {
                    bad++;
                    println("field " + str(o, "name") + "." + str(f, "name") + ": " + ex.getMessage());
                }
            }
        }
        // the functions
        SymbolTable st = currentProgram.getSymbolTable();
        int fns = 0;
        for (JsonElement e : spec.getAsJsonArray("functions")) {
            JsonObject o = e.getAsJsonObject();
            Address a = toAddr(o.get("address").getAsLong());
            try {
                Function f = getFunctionAt(a);
                if (f == null) {
                    disassemble(a);
                    f = createFunction(a, null);
                }
                Namespace ns = namespace(st, str(o, "namespace"), made.containsKey("struct:" + str(o, "namespace")));
                f.setParentNamespace(ns);
                f.setName(str(o, "name"), SourceType.USER_DEFINED);
                String conv = "__" + str(o, "convention");
                List<Variable> params = new ArrayList<>();
                for (JsonElement pe : o.getAsJsonArray("params")) {
                    JsonObject p = pe.getAsJsonObject();
                    if (ns instanceof GhidraClass && conv.equals("__thiscall") && str(p, "name").equals("this")) continue;   // Ghidra's own
                    params.add(new ParameterImpl(str(p, "name"), type(str(p, "type")), currentProgram));
                }
                f.updateFunction(conv, new ReturnParameterImpl(type(str(o, "ret")), currentProgram), params,
                                 Function.FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS, true, SourceType.USER_DEFINED);
                if (!str(o, "doc").isEmpty()) f.setComment(str(o, "doc"));
                fns++;
            } catch (Exception ex) {
                bad++;
                println("function " + str(o, "namespace") + "::" + str(o, "name") + " at " + a + ": " + ex.getMessage());
            }
        }
        // the places
        int labels = 0;
        for (JsonElement e : spec.getAsJsonArray("labels")) {
            JsonObject o = e.getAsJsonObject();
            Address a = toAddr(o.get("address").getAsLong());
            try {
                Namespace ns = namespace(st, str(o, "namespace"), made.containsKey("struct:" + str(o, "namespace")));
                createLabel(a, str(o, "name"), ns, true, SourceType.USER_DEFINED);
                if (!str(o, "doc").isEmpty()) setPlateComment(a, str(o, "doc"));
                labels++;
            } catch (Exception ex) {
                bad++;
                println("label " + str(o, "namespace") + "::" + str(o, "name") + " at " + a + ": " + ex.getMessage());
            }
        }
        println(String.format("spec %s: %d structures, %d enums, %d of %d functions, %d of %d labels, %d failed",
                str(spec, "build"), structs.size(), spec.getAsJsonArray("enums").size(), fns,
                spec.getAsJsonArray("functions").size(), labels, spec.getAsJsonArray("labels").size(), bad));
    }

    private static String str(JsonObject o, String k) {
        return o.has(k) && !o.get(k).isJsonNull() ? o.get(k).getAsString() : "";
    }

    // a class of the spec: Ghidra's class of that name (RTTI's), else one made; a static class's: a namespace
    private Namespace namespace(SymbolTable st, String name, boolean isClass) throws Exception {
        Namespace global = currentProgram.getGlobalNamespace();
        Namespace ns = st.getNamespace(name, global);
        if (ns != null) return ns;
        return isClass ? st.createClass(global, name, SourceType.USER_DEFINED)
                       : st.createNameSpace(global, name, SourceType.USER_DEFINED);
    }

    // the JSON's type: int32, ptr:T, struct:T, enum:T, bytes:n, array:T:n, void
    private DataType type(String t) {
        if (t.startsWith("ptr:")) return dtm.getPointer(type(t.substring(4)), 4);
        if (t.startsWith("struct:") || t.startsWith("enum:")) {
            DataType d = made.get(t);
            return d != null ? d : Undefined4DataType.dataType;
        }
        if (t.startsWith("bytes:")) {
            int n = Integer.parseInt(t.substring(6));
            return n <= 0 ? Undefined1DataType.dataType : new ArrayDataType(Undefined1DataType.dataType, n, 1);
        }
        if (t.startsWith("array:")) {
            int k = t.lastIndexOf(':');
            DataType inner = type(t.substring(6, k));
            return new ArrayDataType(inner, Integer.parseInt(t.substring(k + 1)), inner.getLength());
        }
        switch (t) {
            case "int8": return SignedByteDataType.dataType;
            case "uint8": return ByteDataType.dataType;
            case "char8": return CharDataType.dataType;
            case "int16": return ShortDataType.dataType;
            case "uint16": return UnsignedShortDataType.dataType;
            case "int32": case "nint": return IntegerDataType.dataType;
            case "uint32": case "nuint": return UnsignedIntegerDataType.dataType;
            case "int64": return LongLongDataType.dataType;
            case "uint64": return UnsignedLongLongDataType.dataType;
            case "float32": return FloatDataType.dataType;
            case "float64": return DoubleDataType.dataType;
            case "void": return VoidDataType.dataType;
            default: return Undefined4DataType.dataType;
        }
    }
}
