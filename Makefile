#
## makefile for libgdc and gdc_test
#
LIBGDC_OBJ = gdc.o
CFLAGS += -I ./include/gdc/
LIBGDC = libgdc.so

LIBDIR:= .
LIBDEWARPDIR:= ./dewarp
GDC_TEST_OBJ = gdc_test.o
GDC_CHIP_CHECK_OBJ = gdc_chip_check.o
DEWARP_TEST_OBJ = dewarp/dewarp_test.o
GDC_TEST = gdc_test
GDC_CHIP_CHECK = gdc_chip_check
DEWARP_TEST = dewarp_test
OBJS = $(GDC_TEST_OBJ) $(GDC_CHIP_CHECK_OBJ) $(DEWARP_TEST_OBJ) $(LIBGDC_OBJ)

# rules
all: $(LIBGDC) $(GDC_TEST) $(GDC_CHIP_CHECK) $(DEWARP_TEST)

$(GDC_TEST): $(LIBGDC)
$(GDC_CHIP_CHECK): $(LIBGDC)
$(DEWARP_TEST): $(LIBGDC)

%.o: %.c
	$(CC) -c -fPIC $(CFLAGS) $^ -o $@
dewarp/%.o: dewarp/%.c
	$(CC) -c $(CFLAGS) $^ -o $@

$(LIBGDC): $(LIBGDC_OBJ)
	$(CC) -shared -fPIC -lion $(CFLAGS) $^ -o $(LIBGDC)

$(GDC_TEST): $(GDC_TEST_OBJ)
	$(CC) $^ -L$(LIBDIR) -lgdc -lion -lpthread $(CFLAGS) -o $@

$(GDC_CHIP_CHECK): $(GDC_CHIP_CHECK_OBJ)
	$(CC) $^ -L$(LIBDIR) -lgdc -lion -lpthread $(CFLAGS) -o $@

$(DEWARP_TEST): $(DEWARP_TEST_OBJ)
	$(CC) $^ -L$(LIBDIR) -lgdc -lion -L$(LIBDEWARPDIR) -ldewarp -lpthread $(CFLAGS) -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(GDC_TEST) $(GDC_CHIP_CHECK) $(DEWARP_TEST) $(LIBGDC)
