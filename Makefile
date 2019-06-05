#
## makefile for libgdc and gdc_test
#
LIBGDC_OBJ = gdc.o
CFLAGS = -I ./include/gdc/
LIBGDC = libgdc.so

LIBDIR:= .
GDC_TEST_OBJ = gdc_test.o
GDC_TEST = gdc_test
OBJS = $(GDC_TEST_OBJ) $(LIBGDC_OBJ)

# rules
all: $(LIBGDC) $(GDC_TEST)

$(GDC_TEST): $(LIBGDC)

%.o: %.c
	$(CC) -c -fPIC $(CFLAGS) $^ -o $@

$(LIBGDC): $(LIBGDC_OBJ)
	$(CC) -shared -fPIC -lion $(CFLAGS) $^ -o $(LIBGDC)

$(GDC_TEST): $(GDC_TEST_OBJ)
	$(CC) $^ -L$(LIBDIR) -lgdc $(CFLAGS) -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(GDC_TEST) $(LIBGDC)
