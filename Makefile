SRC= ${wildcard  src/*.cpp}
HDR= ${wildcard  include/*.hpp}
SRC_TEST= ${wildcard test/*.cpp}
SRC_TEST+= ${filter-out $(wildcard src/main.cpp), $(SRC)}

EXE=emulator
EXE_TEST=emulator_test

CXX=g++
CXXFLAGS+=-Wall -Wextra -W -O2 -std=c++20 -march=native -fcoroutines
INCLUDE+=-I./include
INCLUDE_TEST+=-I/usr/include/catch2
LDFLAGS=

OBJDIR=build
OBJ= $(patsubst %.cpp, $(OBJDIR)/%.o,$(notdir $(SRC)))
OBJ_TEST= $(patsubst %.cpp, $(OBJDIR)/%.o,$(notdir $(SRC_TEST)))

all: build run

build: $(OBJ)
	$(CXX) -o $(EXE) $^ $(LDFLAGS)
build_test: $(OBJ_TEST)
	$(CXX) -o $(EXE_TEST) $^ $(LDFLAGS)
run: build
	./$(EXE)
testodoggo: build_test
	./$(EXE_TEST)

build/%.o: src/%.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDE)  -o $@ -c $<
build/%.o: $(SRC_TEST)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(INCLUDE_TEST) -o $@ -c $<

check:
	@clang-check $(SRC)
format:
	@clang-format -i -style=file $(SRC) $(HDR) $(SRC_TEST)
clean:
	rm -rf build $(EXE) $(EXE_TEST)



