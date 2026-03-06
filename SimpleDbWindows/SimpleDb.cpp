#ifdef _WIN32
#include <io.h>
#include <stdint.h>
typedef intptr_t ssize_t;
#endif

#include <iostream>
#include <string>
#include <cstdlib>
#include <string>
#include <cstdio>
#include <vector>

using std::string;


struct InputBuffer {
	char* buffer;
	size_t buffer_length;
	ssize_t input_length;
};

InputBuffer* new_input_buffer()
{
	InputBuffer* input_buffer = (InputBuffer*)malloc(sizeof(InputBuffer));
	input_buffer->buffer = NULL;
	input_buffer->buffer_length = 0;
	input_buffer->input_length = 0;

	return input_buffer;
}

void print_prompt() { std::cout << "db > "; }

void read_input(InputBuffer* input_buffer)
{
	if (input_buffer->buffer == NULL) {
		input_buffer->buffer_length = 1024;
		input_buffer->buffer = (char*)malloc(input_buffer->buffer_length);
	}

	if (fgets(input_buffer->buffer, input_buffer->buffer_length, stdin) == NULL) {
		std::cout << "Error reading input" << std::endl;
		exit(EXIT_FAILURE);
	}

	input_buffer->input_length = strlen(input_buffer->buffer) - 1;
	if (input_buffer->buffer[input_buffer->input_length] == '\n') {
		input_buffer->buffer[input_buffer->input_length] = 0;
	}
}

void close_input_buffer(InputBuffer* input_buffer) {
	free(input_buffer->buffer);
	free(input_buffer);
}




int main(int argc, char* argv[]) {
	InputBuffer* input_buffer = new_input_buffer();
	while (true) {
		print_prompt();
		read_input(input_buffer);


		if (strcmp(input_buffer->buffer, ".exit") == 0) {
			close_input_buffer(input_buffer);
			exit(EXIT_SUCCESS);
		}
		else {
			std::cout << "Unrecognized command: " << input_buffer->buffer << std::endl;
		}
	}

}