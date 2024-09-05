#!/bin/bash
set -e

# Copy scripts
cp /app/keygen.c /workspace/
cp /app/handler.py /workspace/
cp /app/rclone.conf /workspace/

# Install dependencies
apt-get update && apt-get install -y \
    libssl-dev \
    libgmp-dev \
    make \
    automake \
    libtool \
    git \
    python3 \
    python3-pip \
    rclone \
    libmbedtls-dev

# Clone and build secp256k1
git clone https://github.com/bitcoin-core/secp256k1.git
cd secp256k1
./autogen.sh
./configure --enable-module-recovery
make
make install
ldconfig
cd ..

# Compile the C program
gcc /workspace/keygen.c -o /workspace/keygenc -lsecp256k1 -lssl -lcrypto -lmbedcrypto -lpthread

# Install Python dependencies
pip3 install runpod --no-deps

# Ensure the binary is executable
chmod +x /workspace/keygenc

# Create the output folder
mkdir -p /workspace/output

# Run the Python handler
python3 /workspace/handler.py