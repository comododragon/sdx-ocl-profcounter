# Default tools
CC=aarch64-linux-gnu-gcc
VIVADO=$(XILINX_VIVADO)/bin/vivado
XOCC=$(XILINX_SDX)/bin/xocc

# sanitize_dsa: create a filesystem-friendly name from DSA name
#               $(1): name of DSA
COLON=:
PERIOD=.
UNDERSCORE=_
sanitize_dsa = $(strip $(subst $(PERIOD),$(UNDERSCORE),$(subst $(COLON),$(UNDERSCORE),$(1))))
device2dsa = $(if $(filter $(suffix $(1)),.xpfm),$(shell $(COMMON_REPO)/utility/parsexpmf.py $(1) dsa 2>/dev/null),$(1))
device2sandsa = $(call sanitize_dsa,$(call device2dsa,$(1)))
device2dep = $(if $(filter $(suffix $(1)),.xpfm),$(dir $(1))/$(shell $(COMMON_REPO)/utility/parsexpmf.py $(1) hw 2>/dev/null) $(1),)

# Standard variable. Run "make TARGET=... PLATFORM=... CLKID=..." to customise
TARGET=hw
PLATFORM=zcu102
CLKID=0
DSA = $(call device2sandsa, $(PLATFORM))

# CC compile and link flags
CCFLAGS=-fPIC -DCOMMON_COLOURED_PRINTS -Iinclude -O2 -Wall -I$(XILINX_SDX)/runtime/include/1_2/ -I/$(XILINX_SDX)/Vivado_HLS/include/
CCLINKFLAGS=-lm -lxilinxopencl -lpthread -lrt -ldl -lcrypt -L$(XILINX_SDX)/runtime/lib/aarch64

# XOCC compile flags
XOCCFLAGS=-t $(TARGET) --platform $(PLATFORM) -Iinclude --save-temps --clkid $(CLKID)

# checkForVivado: check if vivado binary is reachable
define checkForVivado
    $(if $(wildcard $(VIVADO)), , $(error vivado binary not found, please check your Xilinx installation and/or the init script, e.g. /path/to/xilinx/SDx/20xx.x/settings64.sh))
endef

# checkForXilinxVivado: check if XILINX_VIVADO is set
define checkForXilinxVivado
	$(if $(XILINX_VIVADO), , $(error XILINX_VIVADO is not set or empty, perhaps forgot to source the init script, e.g. /path/to/xilinx/SDx/20xx.x/settings64.sh?))
endef

# checkForPlatform: check if PLATFORM is set
define checkForPlatform
	$(if $(PLATFORM), , $(error PLATFORM is empty))
endef

# checkForTarget: check if TARGET is set
define checkForTarget
	$(if $(TARGET), , $(error TARGET is empty. Please set to a compatible target: hw, hw_emu or sw_emu))
endef

# checkForXOCC: check if xocc binary is reachable
define checkForXOCC
    $(if $(wildcard $(XOCC)), , $(error xocc binary not found, please check your Xilinx installation and/or the init script, e.g. /path/to/xilinx/SDx/20xx.x/settings64.sh))
endef

# checkForXilinxSDx: check if XILINX_SDX is set
define checkForXilinxSDx
    $(if $(XILINX_SDX), , $(error XILINX_SDX is not set or empty, perhaps forgot to source the init script, e.g. /path/to/xilinx/SDx/20xx.x/settings64.sh?))
endef

# checkForCC: check if CC binary is reachable
define checkForCC
	$(if $(findstring 0, $(shell which $(CC) &> /dev/null; echo $$?)), , $(error CC binary not found, please check your Xilinx installation and/or the init script, e.g. /path/to/xilinx/SDx/20xx.x/settings64.sh))
endef

# checkForHostBinary: check if stuff for compiling host binary is properly set
define checkForHostBinary
	$(call checkForTarget)
	$(call checkForCC)
	$(call checkForXilinxSDx)
endef

# checkForXclbin: check if stuff for synthesising OpenCL kernels is properly set
define checkForXclbin
	$(call checkForXilinxSDx)
	$(call checkForXOCC)
	$(call checkForTarget)
	$(call checkForPlatform)
endef

# checkForXo: check if stuff for creating OpenCL objects is properly set
define checkForXo
	$(call checkForXilinxVivado)
	$(call checkForVivado)
	$(call checkForTarget)
	$(call checkForPlatform)
endef

# Top-level make command for hw generation
.PHONY: hw
hw: fpga/hw/$(DSA)/sd_card/execute

# Make command for host binary
.PHONY: host
host: fpga/$(TARGET)/$(DSA)/execute

# Make command for OpenCL synthesis
.PHONY: xclbin
xclbin: fpga/$(TARGET)/$(DSA)/program.xclbin

# Make command for OpenCL objects
.PHONY: xo
xo: fpga/$(TARGET)/$(DSA)/profCounter.xo fpga/$(TARGET)/$(DSA)/probe.xo

# Copies host executable to SD folder
fpga/$(TARGET)/$(DSA)/sd_card/execute: fpga/$(TARGET)/$(DSA)/execute fpga/$(TARGET)/$(DSA)/program.xclbin
	$(call checkForTarget)
	cp fpga/$(TARGET)/$(DSA)/execute fpga/$(TARGET)/$(DSA)/sd_card/execute

# Compiles host executable
fpga/$(TARGET)/$(DSA)/execute: src/host.fpga.c include/common.h
	$(call checkForHostBinary)
	mkdir -p fpga/$(TARGET)/$(DSA)
	$(CC) src/host.fpga.c -o fpga/$(TARGET)/$(DSA)/execute $(CCFLAGS) $(CCLINKFLAGS)

# Synthesises OpenCL kernels
fpga/$(TARGET)/$(DSA)/program.xclbin: fpga/$(TARGET)/$(DSA)/profCounter.xo fpga/$(TARGET)/$(DSA)/probe.xo
	$(call checkForXclbin)
	$(XOCC) $(XOCCFLAGS) $(XOCCLDFLAGS) -lo fpga/$(TARGET)/$(DSA)/program.xclbin fpga/$(TARGET)/$(DSA)/profCounter.xo fpga/$(TARGET)/$(DSA)/probe.xo --sys_config ocl
	# Append two lines to init.sh, responsible for writing onto /etc/profile the XILINX_OPENCL export and cd to /mnt
	echo -e "\necho -e \"\\\\nexport XILINX_OPENCL=/mnt/embedded_root\" >> /etc/profile" >> fpga/$(TARGET)/$(DSA)/sd_card/init.sh
	echo -e "echo -e \"cd /mnt\" >> /etc/profile" >> fpga/$(TARGET)/$(DSA)/sd_card/init.sh

# Compiles OpenCL object for probe kernel
fpga/$(TARGET)/$(DSA)/probe.xo: src/probe.cl
	$(call checkForXo)
	mkdir -p fpga/$(TARGET)/$(DSA)
	$(XOCC) $(XOCCFLAGS) -c --messageDb fpga/$(TARGET)/$(DSA)/probe.mdb -Iinclude --xp misc:solution_name=_xocc_compile --xp param:compiler.version=31 src/probe.cl -o fpga/$(TARGET)/$(DSA)/probe.xo

# Compiles OpenCL object for profile counter RTL kernel
fpga/$(TARGET)/$(DSA)/profCounter.xo: $(wildcard src/profCounter/*.v src/profCounter/*.v) src/profCounter.xml
	$(call checkForXo)
	mkdir -p fpga/$(TARGET)/$(DSA)
	$(VIVADO) -mode batch -source src/profCounter/generateXO.tcl -tclargs fpga/$(TARGET)/$(DSA)/profCounter.xo profCounter $(TARGET) $(DSA) .

# Clean all
.PHONY: clean
clean:
	rm -rf .Xil _x vivado*.jou vivado*.log xocc*.log fpga
