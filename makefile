ifdef S_AXI_BASEADDR
DEFINES += -DS_AXI_BASEADDR=$(S_AXI_BASEADDR)
endif

CC = gcc
CROSS_COMPILE = aarch64-linux-gnu-

all: fanctrl

help:
	@printf "Targets:\n"
	@printf "    fanctrl    Build Ultra96 fan control application\n"

fanctrl: fanctrl.c
	$(CROSS_COMPILE)$(CC) $(INCLUDES) $(DEFINES) $< -o $@

clean:
	rm -Rf fanctrl

.PHONY: all help clean
