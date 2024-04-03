#
## makefile for libgdc and gdc_test
#
LIBDIR ?= .

LIBGDC_OBJ = $(LIBDIR)/gdc.o
CFLAGS += -I ./include/gdc/
CFLAGS += -I ./include/kernel-headers/
LDFLAGS += -L $(LIBDIR)/
LIBGDC = $(LIBDIR)/libgdc.so

ifeq ($(AML_DEW_BIT), 64)
LIBDEWARPDIR:= ./dewarp/lib/64
else
LIBDEWARPDIR:= ./dewarp/lib/32
endif
GDC_TEST_OBJ = $(LIBDIR)/gdc_test.o
GDC_CHIP_CHECK_OBJ = $(LIBDIR)/gdc_chip_check.o
DEWARP_TEST_OBJ = $(LIBDIR)/dewarp/dewarp_test.o
GDC_TEST = gdc_test
GDC_CHIP_CHECK = gdc_chip_check
DEWARP_TEST = dewarp_test
OBJS = $(GDC_TEST_OBJ) $(GDC_CHIP_CHECK_OBJ) $(DEWARP_TEST_OBJ) $(LIBGDC_OBJ)

$(info "LIBDIR : $(LIBDIR)")

# rules
all: $(LIBGDC) $(GDC_TEST) $(GDC_CHIP_CHECK)

$(GDC_TEST): $(LIBGDC)
$(GDC_CHIP_CHECK): $(LIBGDC)
$(DEWARP_TEST): $(LIBGDC)

$(LIBDIR)/%.o: %.c
	@echo " $(CC) gdc"
	$(CC) -fPIC $(CFLAGS) -c $^ -o $@

$(LIBDIR)/dewarp/%.o: dewarp/%.c
	$(info "$(CC) dewarp")
	$(CC) $(CFLAGS) -c $^ -o $@

$(LIBGDC): $(LIBGDC_OBJ)
	$(CC) -shared -fPIC -lion $(CFLAGS) $(LDFLAGS) $^ -o $(LIBGDC)

$(GDC_TEST): $(GDC_TEST_OBJ)
	$(CC) $^ -lgdc -lion -lpthread $(CFLAGS) $(LDFLAGS) -o $(LIBDIR)/$@

$(GDC_CHIP_CHECK): $(GDC_CHIP_CHECK_OBJ)
	$(CC) $^ -lgdc -lion -lpthread $(CFLAGS) $(LDFLAGS) -o $(LIBDIR)/$@

$(DEWARP_TEST): $(DEWARP_TEST_OBJ)
	$(CC) $^ -lgdc -lion -L$(LIBDEWARPDIR) -ldewarp -lpthread $(CFLAGS) $(LDFLAGS) -o $(LIBDIR)/$@

.PHONY: clean
clean:
	rm -f  $(OBJS) $(LIBGDC) $(LIBDIR)/$(GDC_TEST) $(LIBDIR)/$(GDC_CHIP_CHECK) $(LIBDIR)/$(DEWARP_TEST)

$(shell mkdir -p $(LIBDIR)/dewarp)