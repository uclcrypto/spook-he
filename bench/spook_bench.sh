#! /bin/sh

CC=gcc
CFLAGS="-std=gnu99 -g -Ofast -mtune=skylake-avx512"

BENCH_DIR=../spook_bench
BENCH_RUNS=spook_bench_set

export PROC_FREQ=2.0 # Processor frequency (GHz)

N_ITER=100000000

rm -rf $BENCH_DIR
mkdir -p $BENCH_DIR

grep -v ^# < $BENCH_RUNS | while read -r line
do
    set -- x $line;
    CLYDE=clyde_$3bit
    SHADOW=shadow_$5bit
    CLYDE_DEF=CLYDE_TYPE_$3_BIT
    SHADOW_DEF=SHADOW_TYPE_$5_BIT
    DEFS=-D CLYDE_DEF -D SHADOW_DEF
    ARCH=$6
    TYPE=$2
    echo bench $CLYDE $SHADOW $ARCH ...;
    # True bench
    FULLNAME=./$BENCH_DIR/$CLYDE-$SHADOW-$ARCH-bench
    BENCH_HARNESS=bench_spook
    CLYDE_O=$BENCH_DIR/$CLYDE-$ARCH.o
    SHADOW_O=$BENCH_DIR/$SHADOW-$ARCH.o
    $CC $CFLAGS -march=$ARCH $DEFS -c ../src/$CLYDE.c -o $CLYDE_O
    $CC $CFLAGS -march=$ARCH $DEFS -c ../src/$SHADOW.c -o $SHADOW_O
    $CC $CFLAGS -march=$ARCH $DEFS -c ../src/s1p.c -o $BENCH_DIR/s1p-$ARCH.o
    $CC $CFLAGS -march=$ARCH $DEFS -c ../src/encrypt.c -o $BENCH_DIR/encrypt-$ARCH.o
    $CC $CFLAGS -march=$ARCH $DEFS -I ../src -D N_ITER=$N_ITER -c src/$BENCH_HARNESS.c -o $BENCH_DIR/$BENCH_HARNESS-$ARCH.o
    $CC $CFLAGS -march=$ARCH -flto $CLYDE_O $SHADOW_O $BENCH_DIR/s1p-$ARCH.o $BENCH_DIR/encrypt-$ARCH.o $BENCH_DIR/$BENCH_HARNESS-$ARCH.o -o $FULLNAME
    $FULLNAME > $FULLNAME.txt
done

for f in `ls $BENCH_DIR/*.txt`
do
    INSTANCE=`echo $f | awk -F/ '{print $NF}' | cut -d '.' -f 1`
    while read -r line
    do
        echo "$INSTANCE | $line" >> $BENCH_DIR/results.txt
    done < $f
done

# Results analysis

export RES_FILE=$BENCH_DIR/results.txt
python3 spook_max_throughput.py
