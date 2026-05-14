#!/usr/bin/env bash
# NFR verification script — run from robot-cdr/
# Scans App/ recursively. Each check reports PASS/FAIL with matching lines.

set -euo pipefail

PASS=0
FAIL=0

# check_absent DIR GLOB PATTERN LABEL [EXCLUDE]
#   All files in DIR matching GLOB must NOT contain PATTERN (0 matches = PASS).
#   Optional EXCLUDE filters out known false positives via grep -v.
check_absent() {
    local label="$1"
    local dir="$2"
    local glob="$3"
    local pattern="$4"
    local exclude="${5:-}"

    local matches
    if [ -n "$exclude" ]; then
        matches=$(grep -rn --include="$glob" --color=never -E "$pattern" "$dir" 2>/dev/null \
                  | grep -v -E "$exclude" || true)
    else
        matches=$(grep -rn --include="$glob" --color=never -E "$pattern" "$dir" 2>/dev/null || true)
    fi

    if [ -z "$matches" ]; then
        echo "  PASS  $label"
        PASS=$((PASS+1))
    else
        echo "  FAIL  $label"
        echo "$matches" | sed 's/^/          /'
        FAIL=$((FAIL+1))
    fi
}

# check_all_have DIR GLOB PATTERN LABEL
#   Every file in DIR matching GLOB must contain PATTERN at least once.
#   Reports each violating file individually.
check_all_have() {
    local label="$1"
    local dir="$2"
    local glob="$3"
    local pattern="$4"

    local violations=()
    while IFS= read -r -d '' file; do
        if ! grep -qE "$pattern" "$file" 2>/dev/null; then
            violations+=("$file")
        fi
    done < <(find "$dir" -name "$glob" -print0 2>/dev/null)

    if [ ${#violations[@]} -eq 0 ]; then
        echo "  PASS  $label"
        PASS=$((PASS+1))
    else
        echo "  FAIL  $label"
        for f in "${violations[@]}"; do
            echo "          missing in: $f"
        done
        FAIL=$((FAIL+1))
    fi
}

echo "=== NFR Checks (scanning App/) ==="
echo ""

# NFR-09: No snprintf anywhere in App/ except BusFormat.cpp
#   BusFormat.cpp is the sole authorized source of formatted output strings.
#   vsnprintf in ExternalComm (variadic log helper) is excluded.
echo "── NFR-09  No raw snprintf in App/ (except BusFormat.cpp)"
check_absent \
    "NFR-09  App/**/*.cpp  (excl. BusFormat.cpp and vsnprintf)" \
    "App" "*.cpp" \
    "[^v]snprintf" \
    "(BusFormat\.cpp|vsnprintf)"

# NFR-02: No dynamic allocation anywhere in App/
echo "── NFR-02  No new/delete/malloc/free in App/"
check_absent \
    "NFR-02  App/**/*.h" \
    "App" "*.h" \
    "\bnew\b|\bdelete\b|\bmalloc\b|\bfree\b"

check_absent \
    "NFR-02  App/**/*.cpp" \
    "App" "*.cpp" \
    "\bnew\b|\bdelete\b|\bmalloc\b|\bfree\b"

# NFR-08: No accented characters anywhere in App/
echo "── NFR-08  No accented chars in App/"
check_absent \
    "NFR-08  App/**/*.h" \
    "App" "*.h" \
    "[àâçèéêëîïôùûüÿœæÀÂÇÈÉÊËÎÏÔÙÛÜŸŒÆ]"

check_absent \
    "NFR-08  App/**/*.cpp" \
    "App" "*.cpp" \
    "[àâçèéêëîïôùûüÿœæÀÂÇÈÉÊËÎÏÔÙÛÜŸŒÆ]"

# NFR-06: Every .h in App/ must have an #ifndef APP_ include guard
echo "── NFR-06  Include guard #ifndef APP_ in every App/**/*.h"
check_all_have \
    "NFR-06  All App headers have #ifndef APP_ guard" \
    "App" "*.h" \
    "#ifndef APP_"

echo ""
echo "=== Result: ${PASS} passed, ${FAIL} failed ==="
[ "$FAIL" -eq 0 ] && exit 0 || exit 1
