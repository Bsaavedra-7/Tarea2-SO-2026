CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -pthread

OBJS = rw.o monitor_rw.o control_atomico_rw.o verificador.o
TARGET = rw

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
