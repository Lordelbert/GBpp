SRC= ${wildcard  src/*.cpp}
HDR= ${wildcard  src/*.hpp}
EXE=emulator

CXX=g++
CXXFLAGS+=-Wall -Wextra -W -g -O0 -std=c++20 -march=native -fcoroutines
INCLUDE+=
LDFLAGS= -lpthread

OBJDIR=build
OBJ= $(patsubst %.cpp, $(OBJDIR)/%.o,$(notdir $(SRC)))

all: $(OBJ)
	$(CXX) -o $(EXE) $^ $(LDFLAGS)

build/%.o: src/%.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDE)  -o $@ -c $<
check:
	@clang-check $(SRC)
Format:
	@clang-format -i -style=file $(SRC) $(HDR)
clean:
	rm -rf build $(EXE)
