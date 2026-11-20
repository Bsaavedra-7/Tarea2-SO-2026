CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -pthread
SRCS = rw.cpp monitor_rw.cpp control_atomico_rw.cpp verificador.cpp
TARGET = rw

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)