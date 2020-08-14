#! /bin/bash


# mu su
# 384 512
# c32 c64

rm -r crypto_aead

for MU in mu su
do
    for SHADOW in 384 512
    do
        BASE_DIR=crypto_aead/spook128${MU}${SHADOW}v2
        mkdir -p $BASE_DIR
        cp -r ~/phd/spook/nist_implem/v2/$BASE_DIR/* submission/$BASE_DIR/
        for CLYDE in 32 64
        do
            DEST_DIR=submission/$BASE_DIR/hes128c${CLYDE}
            mkdir -p $DEST_DIR
            cp src/*.{c,h} $DEST_DIR/
            cp LICENSE $DEST_DIR/COPYING
            rm $DEST_DIR/{crypto_aead,iacaMarks}.h
            if [[ "$MU" == "mu" ]]
            then
                sed -i -e "s/#define MULTI_USER 0/#define MULTI_USER 1/" $DEST_DIR/parameters.h
            fi
            if [[ "$SHADOW" == "384" ]]
            then
                sed -i -e "s/#define SMALL_PERM 0/#define SMALL_PERM 1/" $DEST_DIR/parameters.h
            fi
            sed -i "\$ i\\#define CLYTE_TYPE_${CLYDE}_BIT" $DEST_DIR/parameters.h
            sed -i "\$ i\\#define SHADOW_TYPE_128_BIT" $DEST_DIR/parameters.h
        done
    done
done

cp -r crypto_aead/* supercop-20200702/crypto_aead/
