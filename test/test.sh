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
test_build_spook 0 0 c9f76c914bbd916c7479493a632cbe1518355bec1564ec99ec6a9e85778f1335
test_build_spook 0 1 b2b7f17c393abfb78a99dc99301667479d82abb7c5f4a57b3060bcc502a772d6
test_build_spook 1 0 3b72ee641cc670ac6e9b1c7abbc707783f006b5d8df272c658f953d62e2d1066
test_build_spook 1 1 d94e35dfee178c385a92eaf5847a81f9c962963cfe3a9bfa849f094c579cdafe
}

test_big_spook_versions()
{
test_build_spook 0 0 c9f76c914bbd916c7479493a632cbe1518355bec1564ec99ec6a9e85778f1335
test_build_spook 1 0 3b72ee641cc670ac6e9b1c7abbc707783f006b5d8df272c658f953d62e2d1066
}

for ctype in 32bit 64bit
do
    export CLYDE_TYPE=clyde_$ctype;
    for stype in 32bit 128bit 
    do
        export SHADOW_TYPE=shadow_$stype;
        test_all_spook_versions;
    done
    #for stype in 256bit 512bit
    #do
    #    export SHADOW_TYPE=shadow_$stype;
    #    test_big_spook_versions;
    #done
done
