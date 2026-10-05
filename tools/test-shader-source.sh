#!/usr/bin/env bash
# Compiles every HLSL entry point the native renderer asks D3DCompile for.
#
# Why this exists: two entry points were lost in a single edit during development
# - one duplicated, one renamed away - and both reached the game before anything
# noticed, because nothing on the host ever looked at the shader. The renderer
# logs its own compile errors clearly, but only once someone is standing in an
# expedition waiting for Steve to appear.
#
# The entry-point names are read out of renderer.cpp rather than listed here, so a
# newly added entry point is validated without touching this script and cannot be
# forgotten. glslangValidator is not a D3D compiler and does not accept every
# construct D3DCompile does, so this is a floor rather than proof: it catches the
# structural damage a text edit causes, which is what it is aimed at.
set -uo pipefail
cd "$(dirname "$0")/.."

failures=0
note() { printf 'FAILED: %s\n' "$1"; failures=$((failures + 1)); }

renderer="native/mhw-renderer/renderer.cpp"
out="$(mktemp -d)"
trap 'rm -rf "$out"' EXIT

python3 - "$renderer" "$out" <<'PY'
import re, sys
source, out = sys.argv[1], sys.argv[2]
text = open(source, errors="ignore").read()
literals = dict(re.findall(r'constexpr char (\w+)\[\] = R"hlsl\((.*?)\)hlsl";', text, re.S))
if not literals:
    sys.exit("no HLSL string literals found in " + source)
for name, body in literals.items():
    open(f"{out}/{name}.hlsl", "w").write(body)

# Structure per literal: braces balance and no function defined twice.
failed = False
for name, body in literals.items():
    if body.count("{") != body.count("}"):
        print(f"FAILED: {name} braces unbalanced "
              f"({body.count('{')} open, {body.count('}')} close)")
        failed = True
    defs = re.findall(r"^\s*(?:float4|float3|float2|float)\s+(\w+)\s*\(", body, re.M)
    dupes = sorted({n for n in defs if defs.count(n) > 1})
    if dupes:
        print(f"FAILED: {name} defines a function twice: {', '.join(dupes)}")
        failed = True

# Register bindings must not be declared twice inside one compilation unit.
# D3DCompile rejects this outright (X4500 "overlapping register semantics"), and
# no per-entry-point compiler catches it, so it is checked structurally.
for name, body in literals.items():
    for register in ("t0", "t1", "t2", "t3", "b0", "b1", "b2", "b3", "s0"):
        count = len(re.findall(rf"register\({register}\)", body))
        if count > 1:
            print(f"FAILED: {name} binds {register} {count} times in one unit")
            failed = True
sys.exit(1 if failed else 0)
PY
structure=$?
[[ $structure -eq 0 ]] || failures=$((failures + 1))

# --- Every entry point must compile against the string it is given -------
# The source string is read from the D3DCompile call rather than assumed, because
# compiling an entry point against the wrong literal is exactly the mistake that
# let two overlapping t0/t1 declarations reach the game.
mapfile -t entries < <(python3 - "$renderer" <<'PY'
import re, sys
text = open(sys.argv[1], errors="ignore").read()
for source, entry, profile in re.findall(
        r'D3DCompile\(\s*(\w+)\s*,[^;]*?"(\w+)",\s*"(\w+)"', text, re.S):
    print(source, entry, profile)
PY
)

if [ "${#entries[@]}" -eq 0 ]; then
    note "no D3DCompile entry points found in $renderer"
else
    if ! command -v glslangValidator >/dev/null; then
        echo "glslangValidator not installed: structure checked, compilation skipped."
        echo "Install glslang to compile every entry point before deploying."
    else
        for triple in "${entries[@]}"; do
            read -r literal entry profile <<<"$triple"
            case "$profile" in
                vs_*) stage=vert ;;
                ps_*) stage=frag ;;
                *) note "unknown shader profile '$profile' for entry '$entry'"; continue ;;
            esac
            # Run from the temporary directory. glslangValidator writes its SPIR-V
            # output as vert.spv/frag.spv into the *current* directory, so running
            # it from the repository root littered the working tree with two files
            # per entry point - which is exactly what happened.
            if output="$(cd "$out" && glslangValidator -D -S "$stage" -e "$entry" -V "$literal.hlsl" 2>&1)" &&
               ! grep -qiE "error|not found" <<<"$output"; then
                printf '%-14s %-9s %-7s compiles\n' "$literal" "$entry" "$profile"
            else
                printf '%-14s %-9s %-7s FAILED\n' "$literal" "$entry" "$profile"
                grep -iE "error|not found" <<<"$output" | head -4 | sed 's/^/    /'
                failures=$((failures + 1))
            fi
        done
    fi
fi

if [ "$failures" -eq 0 ]; then
    echo "Shader source checks passed: structure sound, every requested entry point compiles."
else
    echo "$failures shader source check(s) failed" >&2
    exit 1
fi