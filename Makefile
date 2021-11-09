ifneq ($(words $(wildcard make.local)),0)
include make.local
endif
PYTHONINC=$(wildcard /usr/include/python3*)

SEARCHDIRS := -Iinclude -I${ROGUE_DIR}/include -I${PYTHONINC}

LIBDIR := lib

LINKER       := g++
DEPENDFLAGS  := -O3 -Wall ${SEARCHDIRS} -fPIC

# C++

CXX      := g++
CXXFLAGS  = ${DEPENDFLAGS}

%.o : %.cxx
	${CXX} ${CPPFLAGS} -c $< -o $@ ${CXXFLAGS}

# C preprocessor (C, C++, FORTRAN)

CPPFLAGS = 

# linker

LOADLIBES := -lrogue-core -lreadline
LDFLAGS    = -L${ROGUE_DIR}/lib

LIBSRCS = $(wildcard src/pflib/*.cxx)
LIBSRCS += $(wildcard src/pflib/rogue/*.cxx)

LIBTARGET_STATIC := ${LIBDIR}/libpflib.a
LIBTARGET_SHARED := ${LIBDIR}/libpflib.so

LIBOBJS := $(LIBSRCS:.cxx=.o)
TOOLBIN := tool/pftool.exe

all: ${LIBTARGET_SHARED} ${TOOLBIN}
	echo ${LIBSRCS}

${LIBTARGET_STATIC} : $(LIBOBJS)
	ar rcs $@ $^ 

${LIBTARGET_SHARED} : $(LIBOBJS)
	@mkdir -p ${LIBDIR}
	g++ -shared -Wl,-soname,libpflib.so -o $@ $^ 

${TOOLBIN} : tool/pftool.o
	g++ -o ${TOOLBIN} tool/pftool.o ${LDFLAGS} ${LOADLIBES} -L${LIBDIR} -lpflib

clean: 
	rm $(LIBOBJS)
