#!/bin/bash

cd wasm
cdt-cpp ../../contracts/btc.xsat/btc.xsat.cpp -I ../../contracts/  -DDEBUG
cdt-cpp ../../contracts/exsat.xsat/exsat.xsat.cpp -I ../../contracts/ -DDEBUG
cdt-cpp ../../contracts/poolreg.xsat/poolreg.xsat.cpp -I ../../contracts/ -I ../../external -I ../../external/intx/include -DDEBUG
cdt-cpp ../../contracts/rescmng.xsat/rescmng.xsat.cpp -I ../../contracts/ -DDEBUG
cdt-cpp ../../contracts/rwddist.xsat/rwddist.xsat.cpp -I ../../contracts/ -DUNITTEST -DDEBUG
cdt-cpp ../../contracts/staking.xsat/staking.xsat.cpp -I ../../contracts/ -DDEBUG
cdt-cpp ../../contracts/blkendt.xsat/blkendt.xsat.cpp -I ../../contracts/ -DDEBUG
cdt-cpp ../../contracts/endrmng.xsat/endrmng.xsat.cpp -I ../../contracts/ -DDEBUG
cdt-cpp ../../contracts/blksync.xsat/blksync.xsat.cpp -I ../../contracts/ -I ../../external -I ../../external/intx/include -DMAINNET -DDEBUG
cdt-cpp ../../contracts/utxomng.xsat/utxomng.xsat.cpp -I ../../contracts/ -I ../../external -I ../../external/intx/include -DMAINNET -DDEBUG
cdt-cpp ../../contracts/brdgmng.xsat/brdgmng.xsat.cpp -I ../../contracts/ -I ../../external -I ../../external/intx/include -DMAINNET -DDEBUG

# Define the ABI file
BLKSYNC_ABI_FILE="blksync.xsat.abi"
BLKSYNC_ABI_TEMP_FILE="blksync.xsat.tmp.abi"

# Use jq to modify the specific field type
jq '(.structs[] | select(.name == "block_bucket_row").fields[] | select(.name == "chunk_ids").type) = "uint8[]"' "$BLKSYNC_ABI_FILE" > "$BLKSYNC_ABI_TEMP_FILE" && mv "$BLKSYNC_ABI_TEMP_FILE" "$BLKSYNC_ABI_FILE"


wasm2wat btc.xsat.wasm | sed -e 's|(memory |(memory (export \"memory\") |' | wat2wasm -o btc.xsat.wasm -
wasm2wat exsat.xsat.wasm | sed -e 's|(memory |(memory (export \"memory\") |' | wat2wasm -o exsat.xsat.wasm -
wasm2wat poolreg.xsat.wasm | sed -e 's|(memory |(memory (export \"memory\") |' | wat2wasm -o poolreg.xsat.wasm -
wasm2wat rescmng.xsat.wasm | sed -e 's|(memory |(memory (export \"memory\") |' | wat2wasm -o rescmng.xsat.wasm -
wasm2wat rwddist.xsat.wasm | sed -e 's|(memory |(memory (export \"memory\") |' | wat2wasm -o rwddist.xsat.wasm -
wasm2wat staking.xsat.wasm | sed -e 's|(memory |(memory (export \"memory\") |' | wat2wasm -o staking.xsat.wasm -
wasm2wat blkendt.xsat.wasm | sed -e 's|(memory |(memory (export \"memory\") |' | wat2wasm -o blkendt.xsat.wasm -
wasm2wat endrmng.xsat.wasm | sed -e 's|(memory |(memory (export \"memory\") |' | wat2wasm -o endrmng.xsat.wasm -
wasm2wat blksync.xsat.wasm | sed -e 's|(memory |(memory (export \"memory\") |' | wat2wasm -o blksync.xsat.wasm -
wasm2wat utxomng.xsat.wasm | sed -e 's|(memory |(memory (export \"memory\") |' | wat2wasm -o utxomng.xsat.wasm -
wasm2wat brdgmng.xsat.wasm | sed -e 's|(memory |(memory (export \"memory\") |' | wat2wasm -o brdgmng.xsat.wasm -