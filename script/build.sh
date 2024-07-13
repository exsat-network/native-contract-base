#!/bin/bash -i

cd wasm
cdt-cpp ../../contracts/btc.xsat/btc.xsat.cpp -I ../../contracts/ -DDEBUG 
cdt-cpp ../../contracts/exsat.xsat/exsat.xsat.cpp -I ../../contracts/ -DDEBUG 
cdt-cpp ../../contracts/poolreg.xsat/poolreg.xsat.cpp -I ../../contracts/ -DDEBUG 
cdt-cpp ../../contracts/rescmng.xsat/rescmng.xsat.cpp -I ../../contracts/ -DDEBUG 
cdt-cpp ../../contracts/rwddist.xsat/rwddist.xsat.cpp -I ../../contracts/ -DDEBUG 
cdt-cpp ../../contracts/staking.xsat/staking.xsat.cpp -I ../../contracts/ -DDEBUG 
cdt-cpp ../../contracts/blkendt.xsat/blkendt.xsat.cpp -I ../../contracts/ -DDEBUG
cdt-cpp ../../contracts/endrmng.xsat/endrmng.xsat.cpp -I ../../contracts/ -DDEBUG
cdt-cpp ../../contracts/blksync.xsat/blksync.xsat.cpp -I ../../contracts/ -I ../../external -I ../../external/intx/include -DDEBUG -DMAINNET
cdt-cpp ../../contracts/utxomng.xsat/utxomng.xsat.cpp -I ../../contracts/ -I ../../external -I ../../external/intx/include -DDEBUG -DMAINNET

cleos set contract btc.xsat wasm btc.xsat.wasm btc.xsat.abi
cleos set contract exsat.xsat wasm exsat.xsat.wasm exsat.xsat.abi
cleos set contract poolreg.xsat wasm poolreg.xsat.wasm poolreg.xsat.abi
cleos set contract rescmng.xsat wasm rescmng.xsat.wasm rescmng.xsat.abi
cleos set contract rwddist.xsat wasm rwddist.xsat.wasm rwddist.xsat.abi
cleos set contract staking.xsat wasm staking.xsat.wasm staking.xsat.abi
cleos set contract blkendt.xsat wasm blkendt.xsat.wasm blkendt.xsat.abi
cleos set contract endrmng.xsat wasm endrmng.xsat.wasm endrmng.xsat.abi
cleos set contract blksync.xsat wasm blksync.xsat.wasm blksync.xsat.abi
cleos set contract utxomng.xsat wasm utxomng.xsat.wasm utxomng.xsat.abi

cleos push action btc.xsat create '["btc.xsat", "21000000.00000000 BTC"]' -p btc.xsat
cleos push action exsat.xsat create '["rwddist.xsat", "21000000.00000000 XSAT"]' -p exsat.xsat
cleos push action utxomng.xsat config '[600, 100, 100, 10]' -p utxomng.xsat
cleos push action utxomng.xsat init '[839999, '0000000000000000000172014ba58d66455762add0512355ad651207918494ab', '0000000000000000000000000000000000000000753b8c1eaae701e1f0146360']' -p utxomng.xsat
cleos push action rescmng.xsat init '{"fee_account": "fees.xsat", "cost_per_slot": "0.00000001 BTC", "cost_per_upload": "0.00000001 BTC", "cost_per_verification": "0.00000001 BTC", "cost_per_endorsement": "0.00000001 BTC", "cost_per_parse": "0.00000001 BTC"}' -p rescmng.xsat

cleos set account permission btc.xsat active --add-code
cleos set account permission exsat.xsat active --add-code
cleos set account permission poolreg.xsat active --add-code
cleos set account permission rescmng.xsat active --add-code
cleos set account permission rwddist.xsat active --add-code
cleos set account permission staking.xsat active --add-code
cleos set account permission blkendt.xsat active --add-code
cleos set account permission endrmng.xsat active --add-code
cleos set account permission blksync.xsat active --add-code
cleos set account permission utxomng.xsat active --add-code
