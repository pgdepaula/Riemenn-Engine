#!/usr/bin/env python3
import os
import re
import sys
from pathlib import Path

# Configuration
PROJECT_ROOT = Path(__file__).parent.parent.resolve()
INCLUDE_DIR = PROJECT_ROOT / "include"
OUTPUT_FILE = PROJECT_ROOT / "python" / "riengine.py"
ROOT_HEADER = INCLUDE_DIR / "riengine.h"

# Regex Patterns
RE_INCLUDE = re.compile(r'#include\s*[<"]([^>"]+)[>"]')
RE_DEFINE = re.compile(r'#define\s+(\w+)\s+([^\n]+)')
RE_TYPEDEF_STRUCT_PTR = re.compile(r'typedef\s+struct\s+\w+\s*\*\s*(\w+);')
RE_ENUM = re.compile(r'enum\s+(\w+)\s*\{([^}]+)\};', re.DOTALL)
RE_STRUCT = re.compile(r'struct\s+(\w+)\s*\{([^}]+)\};', re.DOTALL)
RE_FUNCTION = re.compile(r'\b(\w+\s*\*?)\s*(\w+)\s*\(([^)]*)\);')

# Type Mapping
TYPE_MAP = {
    'int': 'ctypes.c_int',
    'double': 'ctypes.c_double',
    'float': 'ctypes.c_float',
    'char': 'ctypes.c_char',
    'bool': 'ctypes.c_bool',
    'void': 'None',
    'uint32_t': 'ctypes.c_uint32',
    'int32_t': 'ctypes.c_int32',
    'uint64_t': 'ctypes.c_uint64',
    'int64_t': 'ctypes.c_int64',
    'size_t': 'ctypes.c_size_t',
    'real_t': 'ctypes.c_double',  # Assuming double
}

class ParsingContext:
    def __init__(self):
        self.defines = {}
        self.enums = {}
        self.structs = [] # List of (name, code_str) to maintain order
        self.opaque_types = []
        self.functions = []
        self.parsed_files = set()

    def map_type(self, c_type):
        c_type = c_type.strip()
        is_ptr = False
        if c_type.endswith('*'):
            c_type = c_type[:-1].strip()
            is_ptr = True
        
        # Array handling (simple cases)
        array_match = re.search(r'\[([^\]]+)\]', c_type)
        array_size = None
        if array_match:
            size_str = array_match.group(1)
            c_type = c_type[:array_match.start()].strip()
            # Try to resolve size from defines
            if size_str.isdigit():
                array_size = int(size_str)
            elif size_str in self.defines:
                 try:
                     array_size = int(self.defines[size_str])
                 except:
                     pass # complex define

        py_type = TYPE_MAP.get(c_type)
        if not py_type:
            if c_type.startswith('struct '):
                struct_name = c_type.split()[1]
                py_type = struct_name # Assume class name matches struct name
            elif c_type.startswith('enum '):
                py_type = 'ctypes.c_int'
            elif c_type in [t for t in self.opaque_types]:
                py_type = 'ctypes.c_void_p'
            else:
                # Fallback
                py_type = f"'{c_type}'" # String forward ref or unknown

        if array_size is not None:
             py_type = f"({py_type} * {array_size})"

        if is_ptr:
            if py_type == 'None':
                return 'ctypes.c_void_p'
            if py_type == 'ctypes.c_char':
                return 'ctypes.c_char_p'
            return f'ctypes.POINTER({py_type})'
        
        return py_type

def strip_comments(text):
    # Strip C-style comments
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.DOTALL)
    # Strip C++-style comments
    text = re.sub(r'//.*', '', text)
    return text

def parse_header(file_path, ctx):
    file_path = file_path.resolve()
    if file_path in ctx.parsed_files:
        return
    ctx.parsed_files.add(file_path)

    try:
        with open(file_path, 'r') as f:
            content = f.read()
    except FileNotFoundError:
        print(f"Warning: File not found: {file_path}")
        return

    # Handle recursively included files first provided they are in the project
    includes = RE_INCLUDE.findall(content)
    for inc in includes:
        # Check in project directories
        possible_paths = [
            INCLUDE_DIR / inc,
            PROJECT_ROOT / inc,
            PROJECT_ROOT / 'math' / inc if (PROJECT_ROOT / 'math' / inc).exists() else None, # Special case for math/vec4.h pattern if needed
        ]
        
        found = False
        for p in possible_paths:
            if p and p.exists():
                parse_header(p, ctx)
                found = True
                break
        
        # If not found, assume it matches the relative path pattern found in include/riengine.h ("math/vec4.h") which is relative to root
        if not found:
             p = PROJECT_ROOT / inc
             if p.exists():
                 parse_header(p, ctx)


    clean_content = strip_comments(content)

    # 1. Defines
    for name, value in RE_DEFINE.findall(clean_content):
        ctx.defines[name] = value.strip()

    # 2. Opaque Typedefs (handles)
    for name in RE_TYPEDEF_STRUCT_PTR.findall(clean_content):
        ctx.opaque_types.append(name)

    # 3. Enums
    for name, body in RE_ENUM.findall(clean_content):
        entries = []
        val = 0
        for line in body.split(','):
            line = line.strip()
            if not line: continue
            if '=' in line:
                k, v = line.split('=')
                k = k.strip()
                v = v.strip()
                # Try simple int parse
                if v.isdigit():
                    val = int(v)
                elif v.startswith('0x'):
                     val = int(v, 16)
                # else: assume previous val logic or complex expression not supported yet
                entries.append((k, val))
            else:
                entries.append((line, val))
            val += 1
        ctx.enums[name] = entries

    # 4. Structs (Definitions)
    # Finding structs is tricky with regex because of nested braces, but standard C style in this project is regular.
    # We use a custom parser for braces.
    
    # Simple regex approach for well-formatted K&R / Linux Kernel style structs
    # We iterate manually to handle nested braces for field arrays or inner structs if any
    
    tokens = re.split(r'(struct\s+\w+\s*\{|\};)', clean_content)
    
    current_struct_name = None
    struct_body = ""
    in_struct = False
    
    # Warning: The simple regex split above might be insufficient for complex nesting.
    # Better approach: Scan for "struct NAME {" and find matching "};"
    
    cursor = 0
    while True:
        match = re.search(r'struct\s+(\w+)\s*\{', clean_content[cursor:])
        if not match:
            break
        
        struct_name = match.group(1)
        start_idx = cursor + match.end()
        
        # Find closing brace
        brace_count = 1
        i = start_idx
        while i < len(clean_content) and brace_count > 0:
            if clean_content[i] == '{':
                brace_count += 1
            elif clean_content[i] == '}':
                brace_count -= 1
            i += 1
            
        if brace_count == 0:
            # Found struct body
            body = clean_content[start_idx:i-1]
            parse_struct_body(struct_name, body, ctx)
            cursor = i
        else:
            cursor = start_idx # Failed to match, move on (should error)

    # 5. Functions
    # Similar scanning for functions
    # We use the regex which is decent for standard declarations
    for ret_type, name, args in RE_FUNCTION.findall(clean_content):
        # clean args
        arg_list = []
        if args.strip() and args.strip() != 'void':
            for arg_raw in args.split(','):
                arg_raw = arg_raw.strip()
                # Split type and name. Name is the last identifier.
                parts = arg_raw.rsplit(' ', 1)
                if len(parts) == 2:
                    a_type = parts[0].strip()
                    a_name = parts[1].strip()
                    # Handle pointer stuck to name: "type *name" -> type *, name
                    while a_name.startswith('*'):
                       a_type += '*'
                       a_name = a_name[1:]
                    arg_list.append((a_type, a_name))
                else:
                    # just type?
                    arg_list.append((parts[0], ''))
        
        ctx.functions.append({
            'name': name,
            'ret_type': ret_type.strip(),
            'args': arg_list
        })


def parse_struct_body(name, body, ctx):
    fields = []
    
    # We need to scan the body to handle nested unions/structs
    cursor = 0
    while cursor < len(body):
        # Skip whitespace
        while cursor < len(body) and body[cursor].isspace():
            cursor += 1
        if cursor >= len(body): break
        
        # Check for Union or Struct block
        if body.startswith('union', cursor) or body.startswith('struct', cursor):
            # Find start of brace
            brace_start = body.find('{', cursor)
            if brace_start != -1:
                # Check directly, ensure no semicolon before brace (which would mean forward decl or member)
                semicolon = body.find(';', cursor, brace_start)
                if semicolon == -1:
                    # It is a nested definition
                    # Find matching brace
                    brace_count = 1
                    i = brace_start + 1
                    while i < len(body) and brace_count > 0:
                        if body[i] == '{': brace_count += 1
                        elif body[i] == '}': brace_count -= 1
                        i += 1
                    
                    inner_body = body[brace_start+1:i-1]
                    # Parse the name after the block: "union { ... } name;"
                    rest = body[i:].lstrip()
                    end_stmt = rest.find(';')
                    if end_stmt != -1:
                        field_name = rest[:end_stmt].strip()
                        cursor = i + len(body[i:]) - len(rest) + end_stmt + 1
                        
                        # Create a synthetic type for this union
                        union_name = f"{name}_u_{field_name}" if field_name else f"{name}_u_anonymous"
                        
                        # Generate union fields
                        union_fields = []
                        # Recursive simple parsing for union body
                        # Note: This limits nesting depth handling if we don't recurse full parsing
                        # But for this project it should be enough (flat unions)
                        
                        # Basic split for union internals (assuming no nested stuff inside union for now)
                        u_stmts = inner_body.split(';')
                        for u_stmt in u_stmts:
                            u_stmt = u_stmt.strip()
                            if not u_stmt: continue
                            parts = u_stmt.rsplit(' ', 1)
                            if len(parts) == 2:
                                u_type = parts[0].strip()
                                u_name = parts[1].strip()
                                # Cleaning *
                                while u_name.startswith('*'):
                                    u_type += '*'
                                    u_name = u_name[1:]
                                union_fields.append((u_name, u_type))

                        # Register this union as a struct (ctypes.Union logic needed in generation)
                        # We tag it specially
                        ctx.structs.append((union_name, union_fields)) 
                        # We need to tell generator it's a Union. 
                        # Hack: Add a marker or handle separate list.
                        # For now, let's prefix name so we can identify later or just treat as struct (layout is diff but for python bindings often ok if opaque, but bad for access).
                        # Let's add to a separate list or tuple in structs: (name, fields, is_union)
                        # Updating ctx.structs format requires changing all calls.
                        # Let's simple create a definition and map field type to it.
                        
                        # Actually, better: separate union registry
                        ctx.unions = getattr(ctx, 'unions', [])
                        ctx.unions.append((union_name, union_fields))
                        
                        fields.append((field_name, union_name))
                        continue

        # Normal field "type name;"
        end = body.find(';', cursor)
        if end == -1: break
        
        stmt = body[cursor:end].strip()
        cursor = end + 1
        
        if not stmt: continue
        
        parts = stmt.rsplit(' ', 1)
        if len(parts) < 2: continue
             
        field_type = parts[0].strip()
        field_name = parts[1].strip()
        
        # Handle "type *var"
        while field_name.startswith('*'):
            field_type += '*'
            field_name = field_name[1:]

        # Handle arrays "var[N]" or "var[N][M]"
        dims = re.findall(r'\[([^\]]+)\]', field_name)
        if dims:
            field_name = field_name.split('[')[0]
            full_type_str = field_type
            for d in dims:
                full_type_str += f"[{d}]"
            field_type = full_type_str
            
        fields.append((field_name, field_type))
        
    ctx.structs.append((name, fields))


def generate_python(ctx):
    lines = []
    lines.append("#!/usr/bin/env python3")
    lines.append("# Auto-generated by generate_bindings.py")
    lines.append("import ctypes")
    lines.append("import os")
    lines.append("")
    lines.append("# Load Library")
    lines.append("_lib_path = os.path.join(os.path.dirname(__file__), '../build/bin/libriengine.so')")
    lines.append("try:")
    lines.append("    _lib = ctypes.CDLL(_lib_path)")
    lines.append("except OSError:")
    lines.append("    # Fallback to local fallback or error")
    lines.append("    try:")
    lines.append("        _lib = ctypes.CDLL('libriengine.so')")
    lines.append("    except OSError:")
    lines.append("        print(f'Using mock library for {_lib_path}')")
    lines.append("        _lib = None")
    lines.append("")
    
    lines.append("# Constants")
    for k, v in ctx.defines.items():
        if v.isdigit() or re.match(r'^-?\d+(\.\d+)?$', v):
            lines.append(f"{k} = {v}")
        else:
            lines.append(f"# {k} = {v} # Complex definition")
    lines.append("")

    lines.append("# Enums")
    for name, entries in ctx.enums.items():
        lines.append(f"# enum {name}")
        for k, v in entries:
            lines.append(f"{k} = {v}")
    lines.append("")
    
    lines.append("# Opaque Handles")
    for t in ctx.opaque_types:
        lines.append(f"class {t}(ctypes.Structure):")
        lines.append(f"    pass")
    lines.append("")
    
    lines.append("# Forward Declarations")
    for name, _ in ctx.structs:
        lines.append(f"class {name}(ctypes.Structure):")
        lines.append(f"    pass")
    
    # Forward declarations for unions
    if hasattr(ctx, 'unions'):
        for name, _ in ctx.unions:
            lines.append(f"class {name}(ctypes.Union):")
            lines.append(f"    pass")

    lines.append("")
    
    # Union Definitions
    if hasattr(ctx, 'unions'):
        for name, fields in ctx.unions:
            lines.append(f"# {name} definition")
            field_list_str = "[\n"
            for fname, ftype in fields:
                py_type = ctx.map_type(ftype)
                field_list_str += f"        ('{fname}', {py_type}),\n"
            field_list_str += "    ]"
            lines.append(f"{name}._fields_ = {field_list_str}")
            lines.append("")

    lines.append("# Struct Definitions")
    for name, fields in ctx.structs:
        lines.append(f"# {name} definition")
        field_list_str = "[\n"
        for fname, ftype in fields:
            py_type = ctx.map_type(ftype)
            field_list_str += f"        ('{fname}', {py_type}),\n"
        field_list_str += "    ]"
        lines.append(f"{name}._fields_ = {field_list_str}")
        lines.append("")

    lines.append("# Functions")
    lines.append("if _lib:")
    for func in ctx.functions:
        name = func['name']
        ret = ctx.map_type(func['ret_type'])
        
        argtypes = [ctx.map_type(a[0]) for a in func['args']]
        arg_str = ", ".join(argtypes)
        
        lines.append(f"    # {func['ret_type']} {name}")
        lines.append(f"    if hasattr(_lib, '{name}'):")
        lines.append(f"        {name} = _lib.{name}")
        lines.append(f"        {name}.argtypes = [{arg_str}]")
        lines.append(f"        {name}.restype = {ret}")
        lines.append(f"    else:")
        lines.append(f"        print(f'Warning: Function {name} not found in library')")
    lines.append("")

    return "\n".join(lines)


def main():
    ctx = ParsingContext()
    print(f"Parsing root: {ROOT_HEADER}")
    parse_header(ROOT_HEADER, ctx)
    
    print(f"Found {len(ctx.structs)} structs, {len(ctx.functions)} functions.")
    
    code = generate_python(ctx)
    
    os.makedirs(OUTPUT_FILE.parent, exist_ok=True)
    with open(OUTPUT_FILE, 'w') as f:
        f.write(code)
    print(f"Generated bindings at {OUTPUT_FILE}")

if __name__ == "__main__":
    main()
