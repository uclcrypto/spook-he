# Spook High End Implementations

Spook software implementations for high-end microprocessors: from 32 bit generic target to AVX512 x86_64 processors.

## Implementations

All the implementations share the same code for the S1P mode of operation, but use different Clyde and Shadow primitive implementations.

Clyde:
* `clyde_32bit.c`: A straightforward, standard and portable C99 implementation. It is based on the reference implementation with changes to enable better compiler optimizations.
* `clyde_64bit.c`: This implementation puts Clyde state in two 64 bit registers, where each register interleaves bits of two consecutive rows. The 2x32 bit L-box is implemented using 64 bit rotations and XORs. This implementation uses compiler instrinsics to access instructions of the `BMI2` extension of the `x86_64` instruction set and improves performance by about 4%.

Implementations of the inverse of Clyde are available (`clyde_32bit_inv.c` and `clyde_64bit_inv.c`) but are not used since they are not useful in implementations without side-channel countermeasures, and have worse performance than their direct counterparts (due the to properties of the the S-box and L-box functions).

Shadow:
* `shadow_32bit.c`: A straightforward, standard and portable C99 implementation. It is based on the reference implementation with changes to enable better compiler optimizations.
* `shadow_128bit.c`: This implementation uses GCC vector extensions to achieve architecture portability. Shadow state is put in four 128-bit registers, where each registers contains one row of each bundle. This implementation exhibits best performance and is thus recommended on platforms that have 128-bit registers.
* `shadow_256bit.c` and `shadow_512bit.c`: These implementations use respectively AVX2 and AVX512f intrinsics. They usually have worse performance than the 128 bit implementation. These implementation do not cover the Shadow384 case (only Shadow512).


These primitives have much better performance than the reference implementation, however they are not fully optimized either: portability and code simplicity are also a concern.
The code size is not optimized at all, for reduced code size, please look at the portable embedded implementation.

## Build

Have a look at `test/Makefile`.

## Test

```sh
$ cd test
$ ./test.sh # Compiler errors are expected when trying to build shadow_256bit.c and shadow_512.c with SMALL_PERM=1
```

## Benchmarking

```sh
$ cd bench
$ ./prim_bench.sh
$ ./spook_bench.sh
```

A few results are shown in [PERFORMANCE](PERFORMANCE.md).

## Contributing

Contributions of any kind (code, bug reports, benckmarks, ...) are welcome. Please contact us at `team@spook.dev`.
