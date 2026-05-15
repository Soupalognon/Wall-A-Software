#!/usr/bin/env bash
set -euo pipefail

# ---------------------------------------------------------------------------
# run_tests.sh — Configure, compile et lance les tests Google Test de robot-cdr
#
# Usage:
#   bash run_tests.sh [--clean] [--configure] [--build-only] [--test-only]
# ---------------------------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

MINGW_BIN="/d/msys64/mingw64/bin"
MSYS_BIN="/d/msys64/usr/bin"

DO_CLEAN=0
FORCE_CONFIGURE=0
BUILD_ONLY=0
TEST_ONLY=0

for arg in "$@"; do
    case "$arg" in
        --clean)     DO_CLEAN=1 ;;
        --configure) FORCE_CONFIGURE=1 ;;
        --build-only) BUILD_ONLY=1 ;;
        --test-only)  TEST_ONLY=1 ;;
        *)
            echo "Usage: bash run_tests.sh [--clean] [--configure] [--build-only] [--test-only]"
            exit 1
            ;;
    esac
done

# Ajouter MinGW64 au PATH si pas déjà présent
if [[ ":$PATH:" != *":$MINGW_BIN:"* ]]; then
    export PATH="$MINGW_BIN:$MSYS_BIN:$PATH"
fi

# Vérifier que cmake et ninja sont accessibles
if ! command -v cmake &>/dev/null; then
    echo "ERREUR : cmake introuvable. Vérifiez que MSYS2 MinGW64 est installé dans D:\\msys64\\"
    echo "  Installer avec : \"D:\\msys64\\mingw64.exe\" pacman -S --noconfirm mingw-w64-x86_64-cmake"
    exit 1
fi
if ! command -v ninja &>/dev/null; then
    echo "ERREUR : ninja introuvable. Vérifiez que MSYS2 MinGW64 est installé dans D:\\msys64\\"
    exit 1
fi

# --clean : supprimer le répertoire de build
if [[ $DO_CLEAN -eq 1 ]]; then
    echo ">>> Nettoyage de $BUILD_DIR ..."
    rm -rf "$BUILD_DIR"
fi

# Configuration CMake
if [[ ! -d "$BUILD_DIR" ]] || [[ $FORCE_CONFIGURE -eq 1 ]]; then
    echo ">>> Configuration CMake ..."
    mkdir -p "$BUILD_DIR"
    cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Debug
fi

# Compilation
if [[ $TEST_ONLY -eq 0 ]]; then
    echo ">>> Compilation ..."
    cmake --build "$BUILD_DIR"
fi

# Lancer les tests
if [[ $BUILD_ONLY -eq 1 ]]; then
    echo ">>> Build terminé (--build-only, tests non lancés)."
    exit 0
fi

TESTS=(
    BusFormatTest
    ExternalCommTest
    ConcreteOdomHALTest
    OdoControlTest
    MotionPlannerTest
    SensorManagerTest
    ActuatorManagerTest
    MonitoringTest
    SensorDriversTest
    ActuatorDriversTest
    PidTest
)

declare -A RESULTS
PASS=0
FAIL=0

for TEST in "${TESTS[@]}"; do
    EXE="$BUILD_DIR/$TEST.exe"
    if [[ ! -f "$EXE" ]]; then
        RESULTS[$TEST]="SKIP (binaire absent)"
        ((FAIL++)) || true
        continue
    fi
    echo ""
    echo ">>> $TEST"
    if "$EXE"; then
        RESULTS[$TEST]="PASS"
        ((PASS++)) || true
    else
        RESULTS[$TEST]="FAIL"
        ((FAIL++)) || true
    fi
done

TOTAL=$((PASS + FAIL))

echo ""
echo "==================== RÉSULTATS ===================="
for TEST in "${TESTS[@]}"; do
    STATUS="${RESULTS[$TEST]:-SKIP}"
    if [[ "$STATUS" == "PASS" ]]; then
        echo "  [PASS] $TEST"
    else
        echo "  [FAIL] $TEST"
    fi
done
echo "==================================================="

if [[ $FAIL -eq 0 ]]; then
    echo "$TOTAL/$TOTAL tests passés — OK"
else
    echo "$PASS/$TOTAL tests passés — ÉCHEC"
fi

read -rp "--- Appuyez sur Entrée pour quitter... "

[[ $FAIL -eq 0 ]] && exit 0 || exit 1
