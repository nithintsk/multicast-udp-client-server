# the compiler: gcc for C program, define as g++ for C++
  CC = gcc

  # compiler flags:
  #  -g    adds debugging information to the executable file
  #  -Wall turns on most, but not all, compiler warnings
  CFLAGS  = -g -Wall


  # the build target executable:
  TARGET = tictactoeClient
  # TARGETO = ttts

  all: $(TARGET)

  $(TARGET): $(TARGET).c utility.c utility.h
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c utility.c

  clean:
	$(RM) $(TARGET)
