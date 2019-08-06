#! /bin/bash

CC=gcc

test_build_spook()
{

MULTI_USER=$1
SMALL_PERM=$2
export CFLAGS="-DMULTI_USER=$MULTI_USER -DSMALL_PERM=$SMALL_PERM"
make clean > /dev/null && make > /dev/null || {
    echo "Build failed: CLYDE_TYPE=$CLYDE_TYPE, SHADOW_TYPE=$SHADOW_TYPE, MULTI_USER=$MULTI_USER, SMALL_PERM=$SMALL_PERM";
    return 1;
}
pushd ../build_test/ > /dev/null
./test
if [ "$MULTI_USER" == "0" ]; then
    LWCNAME=LWC_AEAD_KAT_128_128.txt
else
    LWCNAME=LWC_AEAD_KAT_256_128.txt
fi
if [ "$(sha256sum $LWCNAME  | cut -f 1 -d ' ')" == "$3" ]; then
    echo "Ok, CLYDE_TYPE=$CLYDE_TYPE, SHADOW_TYPE=$SHADOW_TYPE, MULTI_USER=$MULTI_USER, SMALL_PERM=$SMALL_PERM";
else
    echo "Fail, CLYDE_TYPE=$CLYDE_TYPE, SHADOW_TYPE=$SHADOW_TYPE, MULTI_USER=$MULTI_USER, SMALL_PERM=$SMALL_PERM";
fi
popd > /dev/null
}

test_all_spook_versions()
{
test_build_spook 0 0 c744322005f6d6df1846bc4baa4033047856d8183f502a65604711dd25e87b8c
test_build_spook 0 1 5bd32e37cd41cfd48b6e9fc740c56c32a3154e8acdf7e05310bae9d8f213f41c
test_build_spook 1 0 410c79bf206274bf6145103d1e87c20e17d258cd77c550fd0d33ef30a971c460
test_build_spook 1 1 53d431a078490a709767c0089614fcda87218f11e97ab565d2e84f25bbfcd9cc
}

for ctype in 32bit 64bit
do
    for stype in 32bit 128bit 256bit 512bit
    do
        export CLYDE_TYPE=clyde_$ctype;
        export SHADOW_TYPE=shadow_$stype;
        test_all_spook_versions;
    done
done
