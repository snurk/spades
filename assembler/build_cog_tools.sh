#!/bin/bash
set -e

./prepare_cfg
./spades_compile.sh
make -C build/release/projects/unitig_coverage/ -j8
make -C build/release/projects/gsimplifier/ -j8
make -C build/release/projects/cds_subgraphs/ -j8
