ifneq ($(words $(wildcard make.local)),0)
include make.local
endif

SEARCHDIRS := -Iinclude

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

LOADLIBES := 
LDFLAGS    =  

LIBSRCS := $(wildcard src/pflib/*.cxx)
LIBTARGET_STATIC := ${LIBDIR}/libpflib.a
LIBTARGET_SHARED := ${LIBDIR}/libpflib.so

LIBOBJS := $(LIBSRCS:.cxx=.o)

all: ${LIBTARGET_STATIC}

${LIBTARGET_STATIC} : $(LIBOBJS)
	ar rcs $@ $^ 

${LIBTARGET_SHARED} : $(LIBOBJS)
	g++ -shared -Wl,-soname,pflib.so -o $@ $^ ${LDFLAGS} ${LOADLIBES}

