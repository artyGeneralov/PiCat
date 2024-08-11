# Define the compiler and flags
CC = cl
CFLAGS = /EHsc /Iinclude /Ox

# Define the target executable
TARGET = jpeg_decoder.exe

# Define the source files
SRCS = main.cpp src/jpeg_parser.cpp src/error_handler.cpp src/bitmap_encoder src/jpeg_decoder \
src/utils/byte_writer_helper src/utils/bit_reader

# Define the object files
OBJS = main.obj src\jpeg_parser.obj src\error_handler.obj src\bitmap_encoder.obj src\jpeg_decoder.obj \
src\utils\byte_writer_helper.obj src\utils\bit_reader.obj

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

src\error_handler.obj: src\error_handler.cpp
	$(CC) $(CFLAGS) /c src\error_handler.cpp /Fosrc\error_handler.obj
	
src\bitmap_encoder.obj: src\bitmap_encoder.cpp
	$(CC) $(CFLAGS) /c src\bitmap_encoder.cpp /Fosrc\bitmap_encoder.obj
	
src\jpeg_decoder.obj: src\jpeg_decoder.cpp
	$(CC) $(CFLAGS) /c src\jpeg_decoder.cpp /Fosrc\jpeg_decoder.obj
	
src\utils\byte_writer_helper.obj: src\utils\byte_writer_helper.cpp
	$(CC) $(CFLAGS) /c src\utils\byte_writer_helper.cpp /Fosrc\utils\byte_writer_helper.obj
	
src\utils\bit_reader.obj: src\utils\bit_reader.cpp
	$(CC) $(CFLAGS) /c src\utils\bit_reader.cpp /Fosrc\utils\bit_reader.obj
	
# Clean target to remove generated files
clean:
	del main.obj src\jpeg_parser.obj src\error_handler.obj src\bitmap_encoder.obj \
	src\jpeg_decoder.obj src\utils\byte_writer_helper.obj src\utils\bit_reader.obj $(TARGET)
