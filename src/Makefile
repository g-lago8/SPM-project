ifndef FF_ROOT 
# Print message if FF_ROOT is not defined
FF_ROOT		= ${HOME}/fastflow
#$(warning FF_ROOT is not defined. Setting it to ${FF_ROOT})
endif

CXX                = g++ -std=c++20
MPICXX             = mpicxx -std=c++20
OPTFLAGS	   = -O3 -DNDEBUG -ffast-math
CXXFLAGS          += -Wall #-DNO_DEFAULT_MAPPING
                           #-DBLOCKING_MODE -DFF_BOUNDED_BUFFER

INCLUDES	= -I. -I./include -I  $(FF_ROOT)
LIBS               = -pthread
SOURCES            = $(wildcard *.cpp)
TARGET             = $(SOURCES:.cpp=)

.PHONY: all clean cleanall 

BIN_DIR = ../out

# Make out directory if it does not exist
$(shell mkdir -p $(BIN_DIR))

# Compile rule for standard C++ programs
%: %.cpp 
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) $(INCLUDES) $< -o ${BIN_DIR}/$@ $(LIBS)

# Compile parallel_mpi specifically with mpicxx
parallel_mpi: parallel_mpi.cpp
ifdef USE_OPENMP
	$(MPICXX) $(CXXFLAGS) $(OPTFLAGS) $(INCLUDES) -fopenmp $< -o ${BIN_DIR}/$@ $(LIBS)
else
	$(MPICXX) $(CXXFLAGS) $(OPTFLAGS) $(INCLUDES) $< -o ${BIN_DIR}/$@ $(LIBS)
endif

# Compile rule for OpenMP program
%_omp: %_omp.cpp
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) $(INCLUDES) -fopenmp $< -o ${BIN_DIR}/$@ $(LIBS)
# Compile all targets
all : $(TARGET) parallel_mpi

# Clean up standard targets
clean: 
	rm -f $(TARGET)

# Clean up all generated binaries
cleanall: clean
	rm -f $(BIN_DIR)/*
