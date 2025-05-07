#!/bin/bash

CONFIG_FILE="include/os-cfg.h"
OUTPUT_DIR="run_output"
TESTCASES=(
    "sched_0" "sched_1"
    "os_0_mlq_paging" "os_1_mlq_paging"
    "os_1_singleCPU_mlq" "os_1_singleCPU_mlq_paging"
    "os_1_mlq_paging_small_1K" "os_1_mlq_paging_small_4K"
)

set_sched_mode() {
    mode=$1
    sed -i 's|^#define MLQ_SCHED|// #define MLQ_SCHED|' "$CONFIG_FILE"
    sed -i 's|^#define CFS_SCHED|// #define CFS_SCHED|' "$CONFIG_FILE"
    sed -i 's|^//\+ *#define MLQ_SCHED|// #define MLQ_SCHED|' "$CONFIG_FILE"
    sed -i 's|^//\+ *#define CFS_SCHED|// #define CFS_SCHED|' "$CONFIG_FILE"

    if [ "$mode" == "CFS" ]; then
        sed -i 's|// *#define CFS_SCHED|#define CFS_SCHED|' "$CONFIG_FILE"
    elif [ "$mode" == "MLQ" ]; then
        sed -i 's|// *#define MLQ_SCHED|#define MLQ_SCHED|' "$CONFIG_FILE"
    else
        echo "Unknown scheduling mode: $mode"
        exit 1
    fi
}

build() {
    echo "Building..."
    make all
    if [ $? -ne 0 ]; then
        echo "Build failed."
        exit 1
    fi
    echo "Build successful."
}

run_testcases() {
    mode=$1
    outdir="$OUTPUT_DIR/$(echo "$mode" | tr A-Z a-z)"
    mkdir -p "$outdir"
    for tc in "${TESTCASES[@]}"; do
        echo "Running $tc in $mode mode..."
        ./os "$tc" &> "$outdir/${tc}.log"
        echo "Output saved to $outdir/${tc}.log"
    done
}

run_single_testcase() {
    mode=$1
    testcase=$2
    found=false
    for tc in "${TESTCASES[@]}"; do
        if [ "$tc" == "$testcase" ]; then
            found=true
            break
        fi
    done

    if [ "$found" = true ]; then
        outdir="$OUTPUT_DIR/$(echo "$mode" | tr A-Z a-z)"
        mkdir -p "$outdir"
        echo "Running $testcase in $mode mode..."
        ./os "$testcase" &> "$outdir/${testcase}.log"
        echo "Output saved to $outdir/${testcase}.log"
    else
        echo "Testcase \"$testcase\" not found."
        exit 1
    fi
}

reset_to_cfs() {
    set_sched_mode "CFS"
}

trap reset_to_cfs EXIT

if [ "$1" == "run" ]; then
    set_sched_mode "CFS"
    build
    if [ "$2" == "all" ]; then
        run_testcases "CFS"
    else
        run_single_testcase "CFS" "$2"
    fi

elif [ "$1" == "compare" ]; then
    if [ "$2" == "all" ]; then
        for mode in "MLQ" "CFS"; do
            set_sched_mode "$mode"
            build
            run_testcases "$mode"
        done
    else
        for mode in "MLQ" "CFS"; do
            set_sched_mode "$mode"
            build
            run_single_testcase "$mode" "$2"
        done
    fi

elif [ "$1" == "clean" ]; then
    echo "Cleaning..."
    make clean
    rm -rf "$OUTPUT_DIR"
    echo "Clean complete."

else
    echo "Usage:"
    echo "  ./run.sh run <testcase>        Run specific testcase in CFS mode"
    echo "  ./run.sh run all               Run all testcases in CFS mode"
    echo "  ./run.sh compare <testcase>    Compare a testcase in both MLQ and CFS"
    echo "  ./run.sh compare all           Compare all testcases in both modes"
    echo "  ./run.sh clean                 Clean build and outputs"
    exit 1
fi
