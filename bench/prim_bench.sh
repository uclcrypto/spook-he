#! /bin/sh

CC=gcc
CFLAGS="-std=gnu99 -g -Ofast -mtune=skylake-avx512"

BENCH_DIR_IACA=../prim_bench_iaca
BENCH_DIR_REAL=../prim_bench_real

export PROC_FREQ=2.0 # Processor frequency (GHz)

# IACA simulation
echo "IACA-based benchmark"

BENCH_RUNS=prim_bench_set_iaca

rm -rf $BENCH_DIR_IACA
mkdir -p $BENCH_DIR_IACA

grep -v ^# < $BENCH_RUNS | while read -r line
do
    set -- x $line;
    TYPE=$2
    PRIM=${TYPE}_$3
    ARCH=$4
    echo bench $TYPE $PRIM $ARCH ...;
    FULLNAME=./$BENCH_DIR_IACA/$PRIM-$ARCH-iaca
    $CC $CFLAGS -march=$ARCH -D BENCH_IACA -c ../src/$PRIM.c -o $FULLNAME.o
    iaca -arch SKX  $FULLNAME.o > $FULLNAME.txt
done

for f in `ls $BENCH_DIR_IACA/*.txt`
do
    INSTANCE=`echo $f | awk -F/ '{print $NF}' | cut -d '.' -f 1`
    CYCLES=`grep 'Block Throughput' $f | cut -d ' ' -f 3`
    echo $INSTANCE $CYCLES >> $BENCH_DIR_IACA/results.txt
done

# Real benchmark
echo "Execution-based benchmark"

BENCH_DIR_REAL=../prim_bench_real
BENCH_RUNS=prim_bench_set_real

N_ITER=100000000

rm -rf $BENCH_DIR_REAL
mkdir -p $BENCH_DIR_REAL

grep -v ^# < $BENCH_RUNS | while read -r line
do
    set -- x $line;
    TYPE=$2
    PRIM=${TYPE}_$3
    ARCH=$4
    echo bench $TYPE $PRIM $ARCH ...;
    FULLNAME=./$BENCH_DIR_REAL/$PRIM-$ARCH-bench
    BENCH_HARNESS=bench_$TYPE
    $CC $CFLAGS -march=$ARCH  -c ../src/$PRIM.c -o $FULLNAME.o
    $CC $CFLAGS -march=$ARCH  -D N_ITER=$N_ITER -I ../src -c ./src/$BENCH_HARNESS.c -o $BENCH_DIR_REAL/$BENCH_HARNESS.o
    $CC $CFLAGS -march=$ARCH $FULLNAME.o $BENCH_DIR_REAL/$BENCH_HARNESS.o -o $FULLNAME
    $FULLNAME > $FULLNAME.txt
done

for f in `ls $BENCH_DIR_REAL/*.txt`
do
    INSTANCE=`echo $f | awk -F/ '{print $NF}' | cut -d '.' -f 1`
    NSITER=`cut -d ' ' -f 2 < $f`
    echo $INSTANCE $NSITER >> $BENCH_DIR_REAL/results.txt
done

# Results analysis

export IACA_RES_FILE=$BENCH_DIR_IACA/results.txt
export REAL_RES_FILE=$BENCH_DIR_REAL/results.txt
python3 prim_res_analysis.py

