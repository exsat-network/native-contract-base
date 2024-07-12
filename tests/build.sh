#!/bin/bash

cd wasm
cdt-cpp ../../contracts/btc.xsat/btc.xsat.cpp -I ../../contracts/ 
cdt-cpp ../../contracts/exsat.xsat/exsat.xsat.cpp -I ../../contracts/
cdt-cpp ../../contracts/poolreg.xsat/poolreg.xsat.cpp -I ../../contracts/ 
cdt-cpp ../../contracts/rescmng.xsat/rescmng.xsat.cpp -I ../../contracts/ 
cdt-cpp ../../contracts/rwddist.xsat/rwddist.xsat.cpp -I ../../contracts/ -DDEBUG
cdt-cpp ../../contracts/staking.xsat/staking.xsat.cpp -I ../../contracts/ 
cdt-cpp ../../contracts/blkendt.xsat/blkendt.xsat.cpp -I ../../contracts/
cdt-cpp ../../contracts/endrmng.xsat/endrmng.xsat.cpp -I ../../contracts/
cdt-cpp ../../contracts/blksync.xsat/blksync.xsat.cpp -I ../../contracts/ -I ../../external -I ../../external/intx/include -DMAINNET
cdt-cpp ../../contracts/utxomng.xsat/utxomng.xsat.cpp -I ../../contracts/ -I ../../external -I ../../external/intx/include -DMAINNET

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