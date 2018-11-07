GXX = g++ -std=c++11
FLAG = -Wall -O3

SRC = greedy_mapping.cpp
EXE = greedy_mapping

all::$(EXE)

$(EXE): $(SRC)
	$(GXX) $(FLAG) -o $@ $^

clean:
	rm -rf $(EXE)
