#!/bin/bash

cd wasm
cdt-cpp ../../contracts/btc.xsat/btc.xsat.cpp -I ../../contracts/  -DDEBUG
cdt-cpp ../../contracts/exsat.xsat/exsat.xsat.cpp -I ../../contracts/ -DDEBUG
cdt-cpp ../../contracts/poolreg.xsat/poolreg.xsat.cpp -I ../../contracts/ -I ../../external -I ../../external/intx/include -DDEBUG
cdt-cpp ../../contracts/rescmng.xsat/rescmng.xsat.cpp -I ../../contracts/ -I ../../external -DDEBUG 
cdt-cpp ../../contracts/rwddist.xsat/rwddist.xsat.cpp -I ../../contracts/ -I ../../external -DUNITTEST -DDEBUG 
cdt-cpp ../../contracts/staking.xsat/staking.xsat.cpp -I ../../contracts/ -I ../../external -DDEBUG 
cdt-cpp ../../contracts/xsatstk.xsat/xsatstk.xsat.cpp -I ../../contracts/ -I ../../external -DDEBUG 
cdt-cpp ../../contracts/blkendt.xsat/blkendt.xsat.cpp -I ../../contracts/ -I ../../external -DDEBUG
cdt-cpp ../../contracts/endrmng.xsat/endrmng.xsat.cpp -I ../../contracts/ -I ../../external -DDEBUG
cdt-cpp ../../contracts/blksync.xsat/blksync.xsat.cpp -I ../../contracts/ -I ../../external -I ../../external/intx/include -DDEBUG
cdt-cpp ../../contracts/utxomng.xsat/utxomng.xsat.cpp -I ../../contracts/ -I ../../external -I ../../external/intx/include -DDEBUG
