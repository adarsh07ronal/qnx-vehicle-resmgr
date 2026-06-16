# Makefile — QNX Vehicle Resource Manager
# Requires QNX SDP 8.0 with qnxsdp-env.sh sourced
#
# Usage:
#   source ~/qnx800/qnxsdp-env.sh
#   make              # build for QNX x86_64
#   make HOST=1       # build for Linux host (testing without QNX)
#   make clean

TARGET  := vehicle_resmgrd
SRCS    := src/vehicle_resmgr.c src/can_simulator.c src/vehicle_bridge.c
INCS    := -Isrc

# ── QNX cross-compile (default) ──────────────────────────────────────────
ifndef HOST
  CC      := qcc -Vgcc_ntox86_64
  CFLAGS  := -Wall -Wextra -O2 -g $(INCS) -D_QNX_SOURCE
  LDFLAGS := -lm -lsocket
  $(info Building for QNX Neutrino x86_64)

# ── Linux host build (for logic testing without QNX) ────────────────────
else
  CC      := gcc
  CFLAGS  := -Wall -Wextra -O2 -g $(INCS) -DHOST_BUILD \
             -I./qnx_stubs            # see qnx_stubs/ for mock QNX headers
  LDFLAGS := -lm -lpthread
  $(info Building for Linux host (stub mode))
endif

OBJS := $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Built: $@"

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
