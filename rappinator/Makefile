# makefile

CIRCLEHOME = ../circle
PQCPATH    = $(CURDIR)/PQClean/crypto_kem/ml-kem-768/clean
COMMON     = $(CURDIR)/PQClean/common

# === Pi Zero target ===
KERNEL = kernel

VPATH = $(PQCPATH):$(COMMON):..

# Global Include Injection
CFLAGS   += -I$(CURDIR) -I$(COMMON) -I$(PQCPATH)
CPPFLAGS += -I$(CURDIR) -I$(COMMON) -I$(PQCPATH)
DEFINE   += -DPQCLEAN_NAMESPACE=PQCLEAN_MLKEM768_CLEAN

# Identify source files
PQC_SRCS    = $(wildcard $(PQCPATH)/*.c)
COMMON_SRCS = $(wildcard $(COMMON)/*.c)

# Convert to objects
PQC_OBJS    = $(notdir $(PQC_SRCS:.c=.o))
COMMON_OBJS = $(notdir $(COMMON_SRCS:.c=.o))

# Main objects
RAW_OBJS = main.o kernel.o benchy_heapi.o randombytes.o compat.o\
           $(COMMON_OBJS) $(PQC_OBJS)

OBJS = $(sort $(RAW_OBJS))

# Compile rules
%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(DEFINE) -c $< -o $@

%.o: %.cpp
	$(CPP) $(CFLAGS) $(CPPFLAGS) $(DEFINE) -c $< -o $@

# === Circle libraries (Pi Zero includes FAT support if needed) ===
LIBS += $(CIRCLEHOME)/lib/fs/fat/libfatfs.a
LIBS += $(CIRCLEHOME)/lib/fs/libfs.a
LIBS += $(CIRCLEHOME)/lib/libcircle.a
LIBS += $(CIRCLEHOME)/addon/SDCard/libsdcard.a

# === IMPORTANT: include Circle build system last ===
include $(CIRCLEHOME)/Rules.mk

-include $(DEPS)
