#!/bin/bash
# build-owasp.sh - Build owasp-llm-tool for Azure deployment:

set -euo pipefail

LLAMA_REPO="/mnt/Data/Working_Directory/MyCode/myGithub/PublicRepositories/llama.cpp"
DEMO_REPO="/mnt/Data/Working_Directory/MyCode/myGithub/PublicRepositories/llmSecurityDemo"

echo "=== Building owasp-llm-tool (generic for Azure) ==="

cd "$LLAMA_REPO"

# Remove old build directory and recreate
echo "[1/3] Configuring CMake..."
rm -R build
mkdir build

cd build
cmake .. -DCMAKE_BUILD_TYPE=Release


# Build
echo "[2/3] Building..."
cmake --build . --target owasp-llm-tool -j$(nproc)
if [ ! -f "bin/owasp-llm-tool" ]; then
    echo "Error: Build failed"
    exit 1
fi

# Copy to demo
echo "[3/3] Copying to demo repo..."
cp bin/owasp-llm-tool "$DEMO_REPO/llama.cpp/build/bin/"
# Sync config from demo (production source of truth) back to the dev clone,
# so local testing always uses the same thresholds/entities as the deployed build
cp "$DEMO_REPO"/config/*.json "$LLAMA_REPO"/examples/owasp-llm-tool/config/

echo ""
echo "✓ Build complete"
echo "✓ Binary: $(ls -lh bin/owasp-llm-tool | awk '{print $9, $5}')"
echo "✓ Copied to demo repo"
echo "✓ Config synced from demo → dev clone"
echo ""
echo "Next steps:"
echo "  cd $DEMO_REPO"
echo "  ./tests/test_owasp.sh              # Test locally"
echo "  git add llama.cpp/build/bin/owasp-llm-tool"
echo "  git commit -m 'build: update binary'"
echo "  git push                            # Deploy to Azure"
