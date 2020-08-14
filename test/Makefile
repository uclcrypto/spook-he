
CC?=gcc
#CFLAGS:=-std=c99 -Wall -Wextra -g $(CFLAGS)
CFLAGS:=-std=c99 -g $(CFLAGS) -flax-vector-conversions

BDIR?=../build_test
SRCDIR?=../src

CLYDE_TYPE?=CLYDE_TYPE_32_BIT
#CLYDE_TYPE?=CLYDE_TYPE_64_BIT

SHADOW_TYPE?=SHADOW_TYPE_32_BIT
#SHADOW_TYPE?=SHADOW_TYPE_128_BIT

CFLAGS+=-D$(SHADOW_TYPE) -D$(CLYDE_TYPE)

ifeq "$(CLYDE_TYPE)" "CLYDE_TYPE_64_BIT"
    CFLAGS+=-mbmi2
endif
ifeq "$(SHADOW_TYPE)" "shadow_256bit"
    CFLAGS+=-mavx2
endif
ifeq "$(SHADOW_TYPE)" "shadow_512bit"
    CFLAGS+=-mavx512f
endif

objects := s1p.o genkat_aead.o encrypt.o clyde_32bit.o clyde_64bit.o shadow_32bit.o shadow_128bit.o

all: $(BDIR)/test

bdir:
	mkdir -p $(BDIR)

$(BDIR)/%.o: $(SRCDIR)/%.c bdir
	$(CC) -c -o $@ $< $(CFLAGS)

$(BDIR)/%.o: %.c bdir
	$(CC) -c -o $@ $< $(CFLAGS) -I $(SRCDIR)

$(BDIR)/test: $(patsubst %,$(BDIR)/%,$(objects))
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf $(BDIR)


.PHONY: clean bdir
