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

enum MetaCommanResult {
	META_COMMAND_SUCCESS,
	META_COMMAND_UNRECOGNIZED_COMMAND
};

enum PrepareResult {
	PREPARE_SUCCESS,
	PREPARE_UNRECOGNIZED_STATEMENT
};

MetaCommanResult do_meta_command(InputBuffer* input_buffer) {
	if (strcmp(input_buffer->buffer, ".exit") == 0) {
		exit(EXIT_SUCCESS);
	}
	else {
		return META_COMMAND_UNRECOGNIZED_COMMAND;
	}
}

enum StatementType {
	STATEMENT_INSERT,
	STATEMENT_SELECT
};

struct Statement {
	StatementType type;
};

PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement) {
	if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
		statement->type = STATEMENT_INSERT;
		return PREPARE_SUCCESS;
	}
	if (strcmp(input_buffer->buffer, "select") == 0) {
		statement->type = STATEMENT_INSERT;
		return PREPARE_SUCCESS;
	}
	return PREPARE_UNRECOGNIZED_STATEMENT;

 }

void execute_statement(Statement* statement) {
	switch (statement->type) {
		case (STATEMENT_INSERT):
			std::cout << "This is where we could do an instert.\n";
			break;
		case (STATEMENT_SELECT):
			std::cout << "This is where we could do a select.\n";
			break;
	}
}



int main(int argc, char* argv[]) {
	InputBuffer* input_buffer = new_input_buffer();
	while (true) {
		print_prompt();
		read_input(input_buffer);

		if (input_buffer->buffer[0] == '.') {
			switch (do_meta_command(input_buffer)) {
			case (META_COMMAND_SUCCESS):
				continue;
			case (META_COMMAND_UNRECOGNIZED_COMMAND):
				std::cout << "Unrecognized Command " << input_buffer->buffer << "\n";
				continue;
			}
		}

		Statement statement;
		switch (prepare_statement(input_buffer, &statement)) {
			case (PREPARE_SUCCESS):
				break;
			case (PREPARE_UNRECOGNIZED_STATEMENT):
				std::cout << "Unrecognized keyword at start of: " << input_buffer->buffer << "\n";
				continue;
		}

		execute_statement(&statement);
		std::cout << "Executed.\n";
	}



}