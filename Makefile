MBED = $(firstword $(wildcard mbed mbed-os core .))
TARGET = LPC546XX
TOOLCHAIN = GCC_ARM

#SRC ?= $(firstword $(wildcard TESTS/*/*))
BUILD ?= BUILD
BOARD ?= BOARD

VENDOR ?= MBED ARM
BAUD ?= 9600
DEV ?= /dev/$(notdir $(firstword $(foreach v,$(VENDOR), \
		$(shell udevadm trigger -v -n -s block -p ID_VENDOR=$(v)))))
TTY ?= /dev/$(notdir $(firstword $(foreach v,$(VENDOR), \
		$(shell udevadm trigger -v -n -s tty -p ID_VENDOR=$(v)))))

MFLAGS += -j0
#MFLAGS += --stats-depth=100
ifdef VERBOSE
MFLAGS += -v
endif
ifdef DEBUG
#MFLAGS += -o debug-info
MFLAGS += --profile debug
endif
ifdef SMALL
#MFLAGS += --profile small
MFLAGS += --profile release
endif
MFLAGS += -DMBED_TEST_BLOCKDEVICE=SPIFBlockDevice
#MFLAGS += -DMBED_TEST_BLOCKDEVICE_DECL="SPIFBlockDevice bd(PTE2, PTE4, PTE1, PTE5)"
MFLAGS += -DMBED_TEST_BLOCKDEVICE_DECL="SPIFBlockDevice bd(NC, NC, NC, NC)"


all build:
	mkdir -p $(BUILD)/$(TARGET)/$(TOOLCHAIN)
	echo '*' > $(BUILD)/$(TARGET)/$(TOOLCHAIN)/.mbedignore
	python $(MBED)/tools/make.py -t $(TOOLCHAIN) -m $(TARGET)   \
		$(addprefix --source=, . $(SRC))                        \
		$(addprefix --build=, $(BUILD)/$(TARGET)/$(TOOLCHAIN))  \
		$(MFLAGS)

board:
	mkdir -p $(BOARD)
	sudo umount $(BOARD) || true
	sudo mount $(DEV) $(BOARD)

flash:
	mkdir -p $(BOARD)
	sudo umount $(DEV) || true
	sudo mount $(DEV) $(BOARD)
	sudo cp $(BUILD)/$(TARGET)/$(TOOLCHAIN)/*.bin $(BOARD)
	sudo umount $(BOARD)
	@sleep 0.5

test:
	yes 0 | sudo python -c '__import__("pyOCD")\
		.board.MbedBoard.chooseBoard().target.reset()'
	sudo chmod a+rw $(TTY)
	stty -F $(TTY) sane nl $(BAUD)
	echo "\r\n{{__sync;12341234}}\r\n" >> $(TTY)
	echo "\r\n{{__sync;12341234}}\r\n" >> $(TTY)
	cat $(TTY) | sed '/^{{.*}}/{/^{{end/{/success/Q;Q1};d}'

debug-test:
	echo "\r\n{{__sync;12341234}}\r\n" >> $(TTY)
	echo "\r\n{{__sync;12341234}}\r\n" >> $(TTY)
	cat $(TTY) | sed '/^{{.*}}/{/^{{end/{/success/Q;Q1};d}'

tty:
	sudo screen $(TTY) $(BAUD)

cat:
	sudo stty -F $(TTY) sane nl $(BAUD)
	sudo cat $(TTY)

reset:
#   sudo python -c '__import__("serial").Serial("$(TTY)").send_break()'
	yes 0 | sudo python -c '__import__("pyOCD")\
		.board.MbedBoard.chooseBoard().target.reset()'

debug:
	sudo killall pyocd-gdbserver || true
	@sleep 0.5
	yes 0 | sudo pyocd-gdbserver &
	arm-none-eabi-gdb $(BUILD)/$(TARGET)/$(TOOLCHAIN)/*.elf \
		-ex "target remote :3333"
	@sleep 0.5

ctags:
	ctags $$(find -L -regex '.*\.\(h\|c\|hpp\|cpp\)')

clean:
	rm -rf $(BUILD) $(BOARD)
