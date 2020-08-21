SRC= ${wildcard  src/*.cpp}
HDR= ${wildcard  include/*.hpp}
SRC_TEST= ${filter-out $(wildcard src/main.cpp), $(SRC)}
SRC_TEST+= ${wildcard test/*.cpp}

EXE=emulator
EXE_TEST=emulator_test

CXX=g++
CXXFLAGS=-Wall -Wextra -W -std=c++20 -ffunction-sections -fdata-sections -flto -fcoroutines

ifndef DEBUG
CXXFLAGS+=-O2 -march=native
else
LD_DEBUG=-fsanitize=address
CXXFLAGS+=-O0 -g3 -fno-omit-frame-pointer $(LD_DEBUG)
endif

INCLUDE+=-I./include
INCLUDE_TEST+=-I/usr/include/catch2
LDFLAGS=-flto -Wl,--gc-sections
LDFLAGS+=$(LD_DEBUG)

OBJDIR=build
OBJ= $(patsubst %.cpp, $(OBJDIR)/%.o,$(notdir $(SRC)))
OBJ_TEST= $(patsubst %.cpp, $(OBJDIR)/%.o,$(notdir $(SRC_TEST)))

.PHONY: build build_test run testodoggo check format clean

all: build run

build: $(OBJ)
	$(CXX) -o $(EXE) $(LDFLAGS) $(OPTI) $^
build_test: $(OBJ_TEST)
	$(CXX) -o $(EXE_TEST) $(OPTI) $(LDFLAGS)  $^
run: build
	./$(EXE)
testodoggo: build_test
	./$(EXE_TEST)

build/%.o: src/%.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(OPTI) $(INCLUDE)  -o $@ -c $<
build/%.o: test/%.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(OPTI) $(INCLUDE) $(INCLUDE_TEST) -o $@ -c $<

check:
	@clang-check $(SRC)
format:
	@clang-format -i -style=file $(SRC) $(HDR) $(SRC_TEST)
clean:
	rm -rf build $(EXE) $(EXE_TEST)



