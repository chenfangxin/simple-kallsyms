TARGETS := demo 
sym_tmp = .tmp2.o

target_OBJECTS = root.o \
				 rte_backtrace.o

INCLUDES =
CFLAGS = -g -Wall
LIBS = -ldl
CC = gcc
# CC = clang
NM = nm
COMPILE = $(CC) $(CFLAGS) $(INCLUDES)
LINK = $(COMPILE) -o $@

all: $(TARGETS)
	
syms: syms.c
	$(CC) -o syms syms.c

.tmp1: $(target_OBJECTS) 
	$(LINK) $(target_OBJECTS) $(LIBS)
.tmp1.S: .tmp1
	$(NM) -n .tmp1 > .tmp1_symbol
	./syms .tmp1_symbol .tmp1.S
.tmp1.o: .tmp1.S
	$(CC) -c -o $@ $<
.tmp2: $(target_OBJECTS) .tmp1.o
	$(LINK) $(target_OBJECTS) $(LIBS) .tmp1.o
.tmp2.S: .tmp2
	$(NM) -n .tmp2 > .tmp2_symbol
	./syms .tmp2_symbol .tmp2.S
.tmp2.o: .tmp2.S
	$(CC) -c -o $@ $<

$(TARGETS): syms $(target_OBJECTS) $(sym_tmp)
	$(LINK) $(target_OBJECTS) $(LIBS) $(sym_tmp)

root.o: root.c
	$(COMPILE) -c -o $@ $<

rte_backtrace.o: rte_backtrace.c
	$(COMPILE) -c -o $@ $<

clean:
	rm -rf syms
	rm -rf *.o .tmp*
	rm -rf $(TARGETS)
