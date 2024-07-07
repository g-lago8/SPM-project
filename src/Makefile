# 
# FF_ROOT     points to the FastFlow root directory (i.e.
#             the one containing the ff directory).
ifndef FF_ROOT 
# print message if FF_ROOT is not defined
FF_ROOT		= ${HOME}/fastflow
#$(warning FF_ROOT is not defined. Setting it to ${FF_ROOT})

endif

CXX                = g++ -std=c++20
OPTFLAGS	   = -O3 -DNDEBUG -ffast-math
CXXFLAGS          += -Wall #-DNO_DEFAULT_MAPPING
                           #-DBLOCKING_MODE -DFF_BOUNDED_BUFFER

INCLUDES	= -I. -I./include -I  $(FF_ROOT)
LIBS               = -pthread
SOURCES            = $(wildcard *.cpp)
TARGET             = $(SOURCES:.cpp=)

.PHONY: all clean cleanall 

BIN_DIR = ../out

# make out directory if it does not exist
$(shell mkdir -p $(BIN_DIR))

# Compile rule
%: %.cpp 
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) $(INCLUDES) $< -o ${BIN_DIR}/$@ $(LIBS)



all : $(TARGET)

clean: 
	-rm -f $(BIN_DIR)/*
cleanall: clean
	-rm -fr $(TARGET)
