file(GLOB_RECURSE PLBENCH_SRC *.h *.cpp)

add_library(plbench STATIC ${PLBENCH_SRC} ../include/differentialCry/WwMILP.h differentialCry/WwMILP.cpp ../include/differentialCry/BwMILP.h differentialCry/BwMILP.cpp)

set_target_properties(plbench PROPERTIES LINKER_LANGUAGE CXX PREFIX "lib")
