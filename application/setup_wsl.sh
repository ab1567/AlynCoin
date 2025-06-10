#!/bin/bash
set -e

echo "📦 Setting up AlynCoin build environment..."

# --- Sanity Check: Warn if installed in Program Files (which breaks compilation) ---
if [[ "$PWD" =~ [Pp]rogram\ Files ]]; then
  echo "❌ AlynCoin is currently inside a Windows directory path containing spaces:"
  echo "   $PWD"
  echo "🛑 This breaks build tools like Make and Protoc on WSL."
  echo "💡 Please move the AlynCoin folder to a safe path like: /mnt/c/AlynCoin/"
  exit 1
fi

# --- Install Core Build Dependencies ---
sudo apt update
sudo apt install -y dnsutils
sudo apt install -y \
  build-essential cmake pkg-config \
  libssl-dev libprotobuf-dev protobuf-compiler \
  libjsoncpp-dev libboost-all-dev \
  librocksdb-dev zlib1g-dev libbz2-dev \
  libsnappy-dev liblz4-dev libzstd-dev \
  libasio-dev nlohmann-json3-dev \
  zip unzip git curl python3 python3-pip

# --- Install Rust (if not already present) ---
if ! command -v cargo >/dev/null 2>&1; then
  echo "🦀 Installing Rust toolchain..."
  curl https://sh.rustup.rs -sSf | sh -s -- -y
  source "$HOME/.cargo/env"
fi

echo "🛠️ Rust version: $(rustc --version)"

# --- Setup Paths ---
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ALYN_DIR="$SCRIPT_DIR/AlynCoin"
SRC_DIR="$ALYN_DIR/src"
BUILD_DIR="$ALYN_DIR/build"

export ALYNCOIN_CLI_PATH="$BUILD_DIR/alyncoin-cli"
export ALYNCOIN_BLOCKCHAIN_DB="$HOME/.alyncoin/blockchain_db"
export ALYNCOIN_TX_DB="$HOME/.alyncoin/transactions_db"
export ALYNCOIN_GOV_DB="$HOME/.alyncoin/governance_db"
export ALYNCOIN_BLACKLIST_DB="$HOME/.alyncoin/blacklist"
export ALYNCOIN_KEY_DIR="$HOME/.alyncoin/keys"
export ALYNCOIN_IDENTITY_DB="$HOME/.alyncoin/identity_db"

mkdir -p "$ALYNCOIN_BLOCKCHAIN_DB" "$ALYNCOIN_TX_DB" "$ALYNCOIN_GOV_DB" \
         "$ALYNCOIN_BLACKLIST_DB" "$ALYNCOIN_KEY_DIR" "$ALYNCOIN_IDENTITY_DB"

# --- Install Crow Headers if Missing ---
if [ ! -d /usr/include/crow ]; then
  echo "📚 Installing Crow headers from GitHub..."
  git clone --depth=1 https://github.com/CrowCpp/Crow.git /tmp/crow
  sudo mkdir -p /usr/include/crow
  sudo cp /tmp/crow/include/crow/*.h /usr/include/crow/
  rm -rf /tmp/crow
else
  echo "✅ Crow headers already present."
fi

# --- Generate Protobuf Files Manually ---
if [ -d "$SRC_DIR/proto" ]; then
  echo "📄 Manually generating Protobuf files via protoc..."
  echo "📁 Cleaning old generated Protobufs if any (with root fix)..."
  sudo rm -rf "$SRC_DIR/generated"
  mkdir -p "$SRC_DIR/generated"

  find "$SRC_DIR/proto" -name "*.proto" | while read -r file; do
    echo "🛠️ Generating: $(basename "$file")"
    protoc --proto_path="$SRC_DIR/proto" --cpp_out="$SRC_DIR/generated" "$file"
  done
else
  echo "⚠️ No proto/ directory found at $SRC_DIR/proto"
fi

# --- Compile Winterfell Rust zk-STARK FFI ---
echo "🌀 Compiling Winterfell Rust zk-STARK FFI..."
cd "$SRC_DIR/rust"
cargo build --release

# --- Configure and Build with CMake ---
echo "🔧 Running CMake..."
cd "$ALYN_DIR"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake "$ALYN_DIR" -DCMAKE_BUILD_TYPE=Release

echo "🚀 Building AlynCoin CLI tools..."
make -j$(nproc)

# --- Final Output & Permissions ---
chmod -R 755 "$ALYN_DIR"

echo "✅ Build complete!"
echo "📁 Binaries: $BUILD_DIR/bin/"
ls -lh "$BUILD_DIR/bin/"
