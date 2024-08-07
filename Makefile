# Define the compiler and flags
CC = cl
CFLAGS = /EHsc /Iinclude

# Define the target executable
TARGET = jpeg_decoder.exe

# Define the source files
SRCS = main.cpp src/jpeg_parser.cpp src/errorHandler.cpp

# Define the object files
OBJS = main.obj src\jpeg_parser.obj src\errorHandler.obj

# Default target
all: $(TARGET)

# Rule to build the target executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) /Fe$(TARGET) $(OBJS)

# Rule to build object files from source files
main.obj: main.cpp
	$(CC) $(CFLAGS) /c main.cpp /Fomain.obj

src\jpeg_parser.obj: src\jpeg_parser.cpp
	$(CC) $(CFLAGS) /c src\jpeg_parser.cpp /Fosrc\jpeg_parser.obj

src\errorHandler.obj: src\errorHandler.cpp
	$(CC) $(CFLAGS) /c src\errorHandler.cpp /Fosrc\errorHandler.obj

# Clean target to remove generated files
clean:
	del main.obj src\jpeg_parser.obj src\errorHandler.obj $(TARGET)
