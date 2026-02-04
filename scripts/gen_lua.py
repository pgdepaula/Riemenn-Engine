#!/usr/bin/env python3
import os
import re
import sys
from pathlib import Path

# Configuration
PROJECT_ROOT = Path(__file__).parent.parent.resolve()
INCLUDE_DIR = PROJECT_ROOT / "include"
LUA_JIT_OUT = PROJECT_ROOT / "include" / "lua" / "jit" / "riengine.lua"
LUA_PUC_OUT = PROJECT_ROOT / "include" / "lua" / "puc" / "riengine_lua.c"
ROOT_HEADER = INCLUDE_DIR / "riengine.h"

# Regex Patterns
RE_INCLUDE = re.compile(r'#include\s*[<"]([^>"]+)[>"]')
RE_DEFINE = re.compile(r'#define\s+(\w+)\s+([^\n]+)')
RE_TYPEDEF_STRUCT_PTR = re.compile(r'typedef\s+struct\s+\w+\s*\*\s*(\w+);')
RE_ENUM = re.compile(r'enum\s+(\w+)\s*\{([^}]+)\};', re.DOTALL)
RE_STRUCT = re.compile(r'struct\s+(\w+)\s*\{([^}]+)\};', re.DOTALL)
RE_FUNCTION = re.compile(r'\b(\w+\s*\*?)\s*(\w+)\s*\(([^)]*)\);')

class ParsingContext:
    def __init__(self):
        self.defines = {}
        self.enums = {}
        self.structs = [] # List of (name, code_str) to maintain order
        self.opaque_types = []
        self.functions = []
        self.parsed_files = set()
        self.unions = []

def strip_comments(text):
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.DOTALL)
    text = re.sub(r'//.*', '', text)
    return text

def parse_header(file_path, ctx):
    file_path = file_path.resolve()
    if file_path in ctx.parsed_files:
        return
    ctx.parsed_files.add(file_path)
    print(f"Parsing: {file_path}")

    try:
        with open(file_path, 'r') as f:
            content = f.read()
    except FileNotFoundError:
        print(f"Warning: File not found: {file_path}")
        return

    # Handle recursively included files
    includes = RE_INCLUDE.findall(content)
    for inc in includes:
        possible_paths = [
            INCLUDE_DIR / inc,
            PROJECT_ROOT / inc,
            PROJECT_ROOT / 'math' / inc if (PROJECT_ROOT / 'math' / inc).exists() else None,
        ]
        
        found = False
        for p in possible_paths:
            if p and p.exists():
                parse_header(p, ctx)
                found = True
                break
        
        if not found:
            p = PROJECT_ROOT / inc
            if p.exists():
                parse_header(p, ctx)

    clean_content = strip_comments(content)

    # 1. Defines
    for name, value in RE_DEFINE.findall(clean_content):
        # Only keep simple numeric/string defines
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
                if v.isdigit():
                    val = int(v)
                elif v.startswith('0x'):
                    val = int(v, 16)
                entries.append((k, val))
            else:
                entries.append((line, val))
            val += 1
        ctx.enums[name] = entries

    # 4. Structs
    cursor = 0
    while True:
        match = re.search(r'struct\s+(\w+)\s*\{', clean_content[cursor:])
        if not match:
            break
        
        struct_name = match.group(1)
        start_idx = cursor + match.end()
        
        brace_count = 1
        i = start_idx
        while i < len(clean_content) and brace_count > 0:
            if clean_content[i] == '{':
                brace_count += 1
            elif clean_content[i] == '}':
                brace_count -= 1
            i += 1
            
        if brace_count == 0:
            body = clean_content[start_idx:i-1]
            parse_struct_body(struct_name, body, ctx)
            cursor = i
        else:
            cursor = start_idx

    # 5. Functions
    for ret_type, name, args in RE_FUNCTION.findall(clean_content):
        arg_list = []
        if args.strip() and args.strip() != 'void':
            for arg_raw in args.split(','):
                arg_raw = arg_raw.strip()
                parts = arg_raw.rsplit(' ', 1)
                if len(parts) == 2:
                    a_type = parts[0].strip()
                    a_name = parts[1].strip()
                    while a_name.startswith('*'):
                       a_type += '*'
                       a_name = a_name[1:]
                    arg_list.append((a_type, a_name))
                else:
                    arg_list.append((parts[0], ''))
        
        ctx.functions.append({
            'name': name,
            'ret_type': ret_type.strip(),
            'args': arg_list
        })

def parse_struct_body(name, body, ctx):
    fields = []
    cursor = 0
    while cursor < len(body):
        while cursor < len(body) and body[cursor].isspace():
            cursor += 1
        if cursor >= len(body): break
        
        if body.startswith('union', cursor) or body.startswith('struct', cursor):
            # Complex nested types - simplified parse
            brace_start = body.find('{', cursor)
            if brace_start != -1:
                semicolon = body.find(';', cursor, brace_start)
                if semicolon == -1:
                    brace_count = 1
                    i = brace_start + 1
                    while i < len(body) and brace_count > 0:
                        if body[i] == '{': brace_count += 1
                        elif body[i] == '}': brace_count -= 1
                        i += 1
                    
                    rest = body[i:].lstrip()
                    end_stmt = rest.find(';')
                    if end_stmt != -1:
                        field_name = rest[:end_stmt].strip()
                        cursor = i + len(body[i:]) - len(rest) + end_stmt + 1
                        fields.append((field_name, "void*")) # Treat anonymous union/struct as opaque for now
                        continue

        end = body.find(';', cursor)
        if end == -1: break
        
        stmt = body[cursor:end].strip()
        cursor = end + 1
        
        if not stmt: continue
        
        parts = stmt.rsplit(' ', 1)
        if len(parts) < 2: continue
             
        field_type = parts[0].strip()
        field_name = parts[1].strip()
        
        while field_name.startswith('*'):
            field_type += '*'
            field_name = field_name[1:]

        dims = re.findall(r'\[([^\]]+)\]', field_name)
        if dims:
            field_name = field_name.split('[')[0]
            for d in dims:
                field_type += f"[{d}]"
            
        fields.append((field_name, field_type))
    
    ctx.structs.append((name, fields))

# --- LuaJIT Generator ---

def generate_luajit(ctx):
    lines = []
    lines.append("-- Auto-generated LuaJIT bindings for Black Hole Simulator")
    lines.append("local ffi = require('ffi')")
    lines.append("local libs = { 'libriengine.so', './build/bin/libriengine.so', '../build/bin/libriengine.so' }")
    lines.append("local lib = nil")
    lines.append("for _, p in ipairs(libs) do")
    lines.append("    local status, result = pcall(ffi.load, p)")
    lines.append("    if status then lib = result; break end")
    lines.append("end")
    lines.append("if not lib then error('Could not load libriengine.so') end")
    lines.append("")
    lines.append("ffi.cdef[[")
    
    # Constants (Defines)
    for k, v in ctx.defines.items():
        if v.isdigit() or re.match(r'^-?\d+(\.\d+)?$', v):
            lines.append(f"    static const int {k} = {v};")
            
    # Opaque Handles
    for t in ctx.opaque_types:
        lines.append(f"    typedef struct {t} {t};")
        
    # Enums
    for name, entries in ctx.enums.items():
        lines.append(f"    enum {name} {{")
        for k, v in entries:
            lines.append(f"        {k} = {v},")
        lines.append(f"    }};")

    # Structs
    for name, fields in ctx.structs:
        lines.append(f"    typedef struct {name} {{")
        for fname, ftype in fields:
            # Fix types for CDef
            cdef_type = ftype.replace('ctypes.', '') # Cleanup if any
            if 'void*' in cdef_type: cdef_type = 'void*'
            lines.append(f"        {cdef_type} {fname};")
        lines.append(f"    }} {name};")

    # Functions
    for func in ctx.functions:
        args_str = ", ".join([f"{a[0]} {a[1]}" for a in func['args']])
        lines.append(f"    {func['ret_type']} {func['name']}({args_str});")

    lines.append("]]")
    lines.append("")
    lines.append("return lib")
    return "\n".join(lines)

# --- PUC Lua Generator (Wrapper C File) ---

def map_c_to_lua_push(ctype):
    ctype = ctype.strip()
    if ctype in ['int', 'int32_t', 'enum']: return 'lua_pushinteger'
    if ctype in ['double', 'float', 'real_t']: return 'lua_pushnumber'
    if ctype in ['bool']: return 'lua_pushboolean'
    if 'char*' in ctype: return 'lua_pushstring'
    return 'lua_push_struct_ptr' # Generic fallback for structs (pointer)

def map_c_to_lua_check(ctype):
    ctype = ctype.strip()
    if ctype in ['int', 'int32_t', 'enum']: return 'luaL_checkinteger'
    if ctype in ['double', 'float', 'real_t']: return 'luaL_checknumber'
    if ctype in ['bool']: return 'lua_toboolean'
    if 'char*' in ctype: return 'luaL_checkstring'
    return 'lua_check_struct_ptr'

def generate_lua_puc(ctx):
    lines = []
    lines.append("#include <lua.h>")
    lines.append("#include <lauxlib.h>")
    lines.append("#include <lualib.h>")
    lines.append("#include <riengine.h>")
    lines.append("#include <string.h>")
    lines.append("")
    lines.append("// Helper macros for struct UserData")
    lines.append("#define BHS_LUA_CHECK(L, i, type) (*(type**)luaL_checkudata(L, i, #type))")
    lines.append("")
    
    # 1. Struct Metatable & Methods
    # For simplicity, we just expose a generic constructor and field accessors might be too much for C auto-gen
    # So we will wrap FUNCTIONS primarily, and assume Structs are passed as LightUserdata or specific generic Userdata?
    # Correct PUC approach: Create metatable for each struct type.
    
    # We will generate a minimal wrapper:
    # - Functions exposed globally in 'riengine' table.
    # - Structs: Wrapped as alloc'd userdata holding the struct (by value)
    
    for name, fields in ctx.structs:
        lines.append(f"// --- {name} binding ---")
        
        # __index
        lines.append(f"static int {name}_index(lua_State *L) {{")
        lines.append(f"    {name} *obj = luaL_checkudata(L, 1, \"{name}\");")
        lines.append(f"    const char *key = luaL_checkstring(L, 2);")
        for fname, ftype in fields:
            push_fn = map_c_to_lua_push(ftype)
            lines.append(f"    if (strcmp(key, \"{fname}\") == 0) {{")
            if push_fn == 'lua_push_struct_ptr':
                # For nested structs, we might need new userdata or just fail for now
                lines.append(f"        // Nested struct {fname} not fully supported in auto-gen")
                lines.append(f"        lua_pushnil(L);") 
            else:
                 lines.append(f"        {push_fn}(L, obj->{fname});")
            lines.append(f"        return 1;")
            lines.append(f"    }}")
        lines.append("    return 0;")
        lines.append("}")
        
        # __newindex
        lines.append(f"static int {name}_newindex(lua_State *L) {{")
        lines.append(f"    {name} *obj = luaL_checkudata(L, 1, \"{name}\");")
        lines.append(f"    const char *key = luaL_checkstring(L, 2);")
        for fname, ftype in fields:
            check_fn = map_c_to_lua_check(ftype)
            lines.append(f"    if (strcmp(key, \"{fname}\") == 0) {{")
            if check_fn == 'lua_check_struct_ptr':
                 lines.append(f"        // Nested struct set not supported")
            else:
                 lines.append(f"        obj->{fname} = ({ftype}){check_fn}(L, 3);")
            lines.append(f"        return 0;")
            lines.append(f"    }}")
        lines.append("    return 0;")
        lines.append("}")

        # Metatable registration helper
        lines.append(f"static const struct luaL_Reg {name}_methods[] = {{")
        lines.append(f"    {{\"__index\", {name}_index}},")
        lines.append(f"    {{\"__newindex\", {name}_newindex}},")
        lines.append(f"    {{NULL, NULL}}")
        lines.append(f"}};")
        lines.append("")

    # 2. Function Wrappers
    lua_funcs = []
    for func in ctx.functions:
        fname = func['name']
        lua_func_name = f"l_{fname}"
        lua_funcs.append((fname, lua_func_name))
        
        lines.append(f"static int {lua_func_name}(lua_State *L) {{")
        
        # Parse Args
        call_args = []
        for i, (type, argname) in enumerate(func['args']):
            idx = i + 1
            check_fn = map_c_to_lua_check(type)
            if check_fn == 'lua_check_struct_ptr':
                 # Assume struct pointer arg implies we passed a userdata of that struct type
                 clean_type = type.replace('*', '').strip()
                 lines.append(f"    {type} arg{i} = luaL_checkudata(L, {idx}, \"{clean_type}\");")
            else:
                 lines.append(f"    {type} arg{i} = ({type}){check_fn}(L, {idx});")
            call_args.append(f"arg{i}")

        # Call
        call_str = ", ".join(call_args)
        if func['ret_type'] == 'void':
            lines.append(f"    {fname}({call_str});")
            lines.append("    return 0;")
        else:
            lines.append(f"    {func['ret_type']} ret = {fname}({call_str});")
            push_fn = map_c_to_lua_push(func['ret_type'])
            if push_fn == 'lua_push_struct_ptr':
                 # Helper to return struct by value -> new userdata
                 clean_ret = func['ret_type'].replace('*', '').strip()
                 lines.append(f"    {clean_ret} *ret_obj = lua_newuserdata(L, sizeof({clean_ret}));")
                 lines.append(f"    *ret_obj = ret;")
                 lines.append(f"    luaL_setmetatable(L, \"{clean_ret}\");")
            else:
                 lines.append(f"    {push_fn}(L, ret);")
            lines.append("    return 1;")
            
        lines.append("}")
        lines.append("")

    # 3. Main Init
    lines.append("static const struct luaL_Reg riengine_funcs[] = {")
    for fname, wrapname in lua_funcs:
        lines.append(f"    {{\"{fname}\", {wrapname}}},")
    lines.append("    {NULL, NULL}")
    lines.append("};")
    lines.append("")
    lines.append("int luaopen_riengine(lua_State *L) {")
    lines.append("    luaL_newlib(L, riengine_funcs);")
    lines.append("")
    lines.append("    // Register Metatables")
    for name, _ in ctx.structs:
        lines.append(f"    luaL_newmetatable(L, \"{name}\");")
        lines.append(f"    luaL_setfuncs(L, {name}_methods, 0);")
        lines.append(f"    lua_pop(L, 1);")
    lines.append("")
    lines.append("    // Constants")
    for k, v in ctx.defines.items():
        if v.isdigit() or re.match(r'^-?\d+(\.\d+)?$', v):
            lines.append(f"    lua_pushnumber(L, {v});")
            lines.append(f"    lua_setfield(L, -2, \"{k}\");")
            
    lines.append("    return 1;")
    lines.append("}")

    return "\n".join(lines)

def main():
    ctx = ParsingContext()
    print(f"Parsing root: {ROOT_HEADER}")
    parse_header(ROOT_HEADER, ctx)
    print(f"Found {len(ctx.structs)} structs, {len(ctx.functions)} functions.")

    # LuaJIT
    print(f"Generating LuaJIT bindings to {LUA_JIT_OUT}...")
    jit_code = generate_luajit(ctx)
    os.makedirs(LUA_JIT_OUT.parent, exist_ok=True)
    with open(LUA_JIT_OUT, 'w') as f:
        f.write(jit_code)

    # PUC Lua
    print(f"Generating PUC Lua bindings to {LUA_PUC_OUT}...")
    puc_code = generate_lua_puc(ctx)
    os.makedirs(LUA_PUC_OUT.parent, exist_ok=True)
    with open(LUA_PUC_OUT, 'w') as f:
        f.write(puc_code)
    
    print("Done.")

if __name__ == "__main__":
    main()
