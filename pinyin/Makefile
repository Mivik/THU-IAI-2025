CXX = g++
CXXFLAGS = -Wall -std=c++11 -O2

ifdef KN_SMOOTHING
CXXFLAGS += -DKN_SMOOTHING
endif

SRCS = $(shell find src -type f)
OBJS = $(SRCS:.cpp=.o)

COMMON_OBJS = src/corpus.o src/tables.o src/encoding.o src/utils.o

all: main main_word

run: main
	./main < data/input.txt > data/output.txt

main: src/main.o src/bigram_ime.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

main_word: src/main_word.o src/word_ime.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

main_word_tri: src/main_word_tri.o src/word_tri_ime.o $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

test_aho_corasick: src/test_aho_corasick.o
	$(CXX) $(CXXFLAGS) -o $@ $^
	./$@

%.o: %.cpp $(shell find include -type f)
	$(CXX) $(CXXFLAGS) -Iinclude -c $< -o $@

clean:
	rm -f $(OBJS) main main_word

.PHONY: all clean run
