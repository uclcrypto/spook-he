# Performance evaluation

Performance has been evaluated for both most relevant primitives (Clyde128 and Shadow512) and for the whole AEAD mode (Spook128su512v1).
Performance for other Spook modes has not yet been evaluated, yet the Spook128mu512v1 is expected to have the same performance characteristics as Spook128su512v1.

## Build

The code was compiled with gcc 8.2 for the following targets:
* generic `x86_64`
* haswell
* skylake-avx512

## Intel IACA

Below are the extimates of cycle count for various primitive implementations given by the [IACA 3.0](https://software.intel.com/en-us/articles/intel-architecture-code-analyzer) tool.

Clyde128:

 | |x86-64|haswell|skylake-avx512|
|-|-|-|-|
|clyde_32bit|372.66|252.30|252.30|
|clyde_64bit| |210.00|210.00| 


Shadow512:

 | |x86-64|haswell|skylake-avx512|
|-|-|-|-|
|shadow_128bit|325.92|333.78|225.78|
|shadow_256bit| |295.56|242.22|
|shadow_32bit| | | |
|shadow_512bit| | |192.00| 

## Benchmark

The benchmark was run on a Intel(R) Xeon(R) Gold 6128 CPU @ 3.40GHz.

The reported performance figures are cycles per execution for the primitives, and throughput (cycles per byte) for Spook (for large messages).


Clyde128 (Cycles):

 | |x86-64|haswell|skylake-avx512|
|-|-|-|-|
|clyde_32bit|317.20|283.40|283.20|
|clyde_64bit| |271.00|271.40| 


Shadow512 (Cycles):

 | |x86-64|haswell|skylake-avx512|
|-|-|-|-|
|shadow_128bit|408.80|396.60|304.40|
|shadow_256bit| |432.20|312.40|
|shadow_32bit|904.40|456.60|342.40|
|shadow_512bit| | |454.20| 

Spook128su512v1 (Cycles per byte), max throughput:

 | |x86-64|haswell|skylake-avx512|
|-|-|-|-|
|clyde_32bit-shadow_128bit|12.71|12.88|10.05|
|clyde_32bit-shadow_256bit| |13.68|10.35|
|clyde_32bit-shadow_32bit|157.46|100.84|11.33|
|clyde_32bit-shadow_512bit| | |14.81|
|clyde_64bit-shadow_128bit| |12.88|10.04|
|clyde_64bit-shadow_256bit| |13.68|10.35|
|clyde_64bit-shadow_32bit| |14.71|11.33|
|clyde_64bit-shadow_512bit| | |14.77| 

Spook128su512v1 (Cycles per byte), throughput for m=2048 bytes:

 | |x86-64|haswell|skylake-avx512|
|-|-|-|-|
|clyde_32bit-shadow_128bit|13.32|13.34|10.05|
|clyde_32bit-shadow_256bit| |14.15|10.76|
|clyde_32bit-shadow_32bit|29.26|15.24|11.80|
|clyde_32bit-shadow_512bit| | |15.32|
|clyde_64bit-shadow_128bit| |13.34|10.04|
|clyde_64bit-shadow_256bit| |14.14|11.07|
|clyde_64bit-shadow_32bit| |15.23|11.79|
|clyde_64bit-shadow_512bit| | |15.36|

