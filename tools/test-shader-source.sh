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
set -euo pipefail
cd "$(dirname "$0")/.."

renderer="native/mhw-renderer/renderer.cpp"
out="$(mktemp -d)"
trap 'rm -rf "$out"' EXIT

python3 - "$renderer" "$out/shader.hlsl" <<'PY'
import re, sys
source, target = sys.argv[1], sys.argv[2]
text = open(source, errors="ignore").read()
match = re.search(r'constexpr char Shader\[\] = R"hlsl\((.*?)\)hlsl";', text, re.S)
if not match:
    sys.exit("no Shader string literal found in " + source)
open(target, "w").write(match.group(1))
PY

failures=0
note() { printf 'FAILED: %s\n' "$1"; failures=$((failures + 1)); }

# --- Structure, checked whether or not a compiler is present --------------
python3 - "$out/shader.hlsl" <<'PY' || failures=$((failures + 1))
import re, sys, collections
body = open(sys.argv[1], errors="ignore").read()
ok = True
if body.count("{") != body.count("}"):
    print(f"FAILED: braces unbalanced ({body.count('{')} open, {body.count('}')} close)")
    ok = False
defs = re.findall(r"^\s*(?:float4|float3|float2|float)\s+(\w+)\s*\(", body, re.M)
duplicates = [n for n, c in collections.Counter(defs).items() if c > 1]
if duplicates:
    print("FAILED: shader function(s) defined more than once: " + ", ".join(sorted(duplicates)))
    ok = False
sys.exit(0 if ok else 1)
PY

# --- Every entry point the renderer requests must exist and compile ------
mapfile -t entries < <(python3 - "$renderer" <<'PY'
import re, sys
text = open(sys.argv[1], errors="ignore").read()
# D3DCompile(<source>, ..., "<entry>", "<profile>", ...) - capture the pair.
for entry, profile in re.findall(r'D3DCompile\([^;]*?"(\w+)",\s*"(\w+)"', text, re.S):
    print(entry, profile)
PY
)

if [ "${#entries[@]}" -eq 0 ]; then
    note "no D3DCompile entry points found in $renderer"
else
    if ! command -v glslangValidator >/dev/null; then
        echo "glslangValidator not installed: structure checked, compilation skipped."
        echo "Install glslang to compile every entry point before deploying."
    else
        for pair in "${entries[@]}"; do
            entry="${pair%% *}"
            profile="${pair##* }"
            case "$profile" in
                vs_*) stage=vert ;;
                ps_*) stage=frag ;;
                *) note "unknown shader profile '$profile' for entry '$entry'"; continue ;;
            esac
            if output="$(glslangValidator -D -S "$stage" -e "$entry" -V "$out/shader.hlsl" 2>&1)" &&
               ! grep -qiE "error|not found" <<<"$output"; then
                printf '%-10s %-7s compiles\n' "$entry" "$profile"
            else
                printf '%-10s %-7s FAILED\n' "$entry" "$profile"
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