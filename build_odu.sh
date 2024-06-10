#!/bin/sh

set -e

export RIC_HOST="172.30.105.104"
export RIC_PORT=36422
export INTERFACE_TO_RIC="col0"
export JSON_FORMAT=0
export DEBUG=0

# get build clean from cli arguments
if [ $# -ne 0 ]; then
    BUILD_CLEAN=1
fi

# build rust libraries
rust_libraries="csv_reader srs_connector"
for i in ${rust_libraries}; do
    cd ./src/du_app/${i} && cargo build --release && cp target/release/lib${i}.* /usr/lib/ && cd ../../..
done
ldconfig

# setup RIC e2term address and port
sed -i "s/^#define RIC_IP_V4_ADDR.*/#define RIC_IP_V4_ADDR \"${RIC_HOST}\"/g" ./src/du_app/du_cfg.h
sed -i "s/^#define RIC_PORT.*/#define RIC_PORT ${RIC_PORT}/g" ./src/du_app/du_cfg.h

# setup interface to communicate with RIC
sed -i "s/^#define INTERFACE_TO_RIC.*/#define INTERFACE_TO_RIC \"${INTERFACE_TO_RIC}\"/g" ./src/du_app/du_cfg.h

# setup debug field
sed -i "s/^#define DEBUG.*/#define DEBUG ${DEBUG}/g" ./src/du_app/bs_connector.h

# setup field to send metrics in json format
sed -i "s/^#define JSON_FORMAT.*/#define JSON_FORMAT ${JSON_FORMAT}/g" ./src/du_app/bs_connector.h

# build
if [ ${BUILD_CLEAN} ]; then
    cd build/odu && make clean_odu && make odu -j ${nproc} MACHINE=BIT64 MODE=FDD
else
    cd build/odu && make odu -j ${nproc} MACHINE=BIT64 MODE=FDD
fi

