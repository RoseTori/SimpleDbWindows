#define _CRT_SECURE_NO_WARNINGS
#ifdef _WIN32
#include <io.h>
#include <stdint.h>
typedef intptr_t ssize_t;
#endif

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <sstream>
#include <fcntl.h> 
#include <sys/stat.h>
#include <io.h>


#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255
#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)
#define TABLE_MAX_PAGES 100
#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)


struct Row {
	uint32_t id;
	char username[COLUMN_USERNAME_SIZE + 1];
	char email[COLUMN_EMAIL_SIZE + 1];

};

const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;
const uint32_t PAGE_SIZE = 4096;
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;


using std::string;

struct Pager {
	int file_descriptor;
	uint32_t file_length;
	void* pages[TABLE_MAX_PAGES];
};

struct InputBuffer {
	char* buffer;
	size_t buffer_length;
	ssize_t input_length;
};

enum StatementType {
	STATEMENT_INSERT,
	STATEMENT_SELECT,
};

struct Statement {
	StatementType type;
	Row row_to_insert;
};

struct Table {
	uint32_t num_rows;
	Pager* pager;

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

void* get_page(Pager* pager, uint32_t page_num) {
	if (page_num > TABLE_MAX_PAGES) {
		std::cout << "Tried to fetch page number out of bounds. " << page_num << " > " << TABLE_MAX_PAGES << std::endl;
		exit(EXIT_FAILURE);
	}

	if (pager->pages[page_num] == NULL) {
		void* page = malloc(PAGE_SIZE);
		uint32_t num_pages = pager->file_length / PAGE_SIZE;

		if (pager->file_length % PAGE_SIZE) {
			num_pages += 1;
		}

		if (page_num <= num_pages) {
			_lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
			ssize_t bytes_read = _read(pager->file_descriptor, page, PAGE_SIZE);

			if (bytes_read == -1) {
				std::cout << "Error reading file: " << errno << std::endl;
				exit(EXIT_FAILURE);
			}
		}

		pager->pages[page_num] = page;
	}
	return pager->pages[page_num];
}

void* row_slot(Table* table, uint32_t row_num) {
	uint32_t page_num = row_num / ROWS_PER_PAGE;

	void* page = get_page(table->pager, page_num);
	uint32_t row_offset = row_num % ROWS_PER_PAGE;
	uint32_t byte_offset = row_offset * ROW_SIZE;

	char* p = (char*)page;
	return p + byte_offset;
}

enum MetaCommanResult {
	META_COMMAND_SUCCESS,
	META_COMMAND_UNRECOGNIZED_COMMAND
};

enum ExecuteResult {
	EXECUTE_TABLE_FULL,
	EXECUTE_SUCCESS
};

void serialize_row(Row* source, void* destination) {
	char* dest = (char*)destination;

	memcpy(dest + ID_OFFSET, &(source->id), ID_SIZE);
	memcpy(dest + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
	memcpy(dest + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}

void deserialize_row(void* source, Row* destination) {
	char* sour = (char*)source;

	memcpy(&(destination->id), sour + ID_OFFSET, ID_SIZE);
	memcpy(&(destination->username), sour + USERNAME_OFFSET, USERNAME_SIZE);
	memcpy(&(destination->email), sour + EMAIL_OFFSET, EMAIL_SIZE);
}

void print_row(Row* row) {
	std::cout << "(" << row->id << ", " << row->username << ", " << row->email << ")" << std::endl;
}

enum PrepareResult {
	PREPARE_SUCCESS,
	PREPARE_NEGATIVE_ID,
	PREPARE_UNRECOGNIZED_STATEMENT,
	PREPARE_STRING_TOO_LONG,
	PREPARE_SYNTAX_ERROR
};


void pager_flush(Pager* pager, uint32_t page_num, uint32_t size) {
	if (pager->pages[page_num] == NULL) {
		std::cout << "Tried to flush null page" << std::endl;
		exit(EXIT_FAILURE);
	}

	off_t offset = _lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);

	if (offset == -1) {
		std::cout << "Error seeking: " << errno << std::endl;
		exit(EXIT_FAILURE);
	}

	ssize_t bytes_written = _write(pager->file_descriptor, pager->pages[page_num], size);

	if (bytes_written == -1) {
		std::cout << "Error writing: " << errno << std::endl;
		exit(EXIT_FAILURE);
	}
}


void db_close(Table* table) {
	Pager* pager = table->pager;
	uint32_t num_full_pages = table->num_rows / ROWS_PER_PAGE;

	for (uint32_t i = 0; i < num_full_pages; i++) {
		if (pager->pages[i] == NULL) {
			continue;
		}
		pager_flush(pager, i, PAGE_SIZE);
		free(pager->pages[i]);
		pager->pages[i] = NULL;
	}

	uint32_t num_additional_rows = table->num_rows % ROWS_PER_PAGE;
	if (num_additional_rows > 0) {
		uint32_t page_num = num_full_pages;
		if (pager->pages[page_num] != NULL) {
			pager_flush(pager, page_num, num_additional_rows * ROW_SIZE);
			free(pager->pages[page_num]);
			pager->pages[page_num] = NULL;

		}
	}

	int result = _close(pager->file_descriptor);
	if (result == -1) {
		std::cout << "Error closing file" << std::endl;
		exit(EXIT_FAILURE);
	}
	for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
		void* page = pager->pages[i];
		if (page) {
			free(page);
			pager->pages[i] = NULL;
		}
	}
}

MetaCommanResult do_meta_command(InputBuffer* input_buffer, Table* table) {
	if (strcmp(input_buffer->buffer, ".exit") == 0) {
		db_close(table);
		exit(EXIT_SUCCESS);
	}
	else {
		return META_COMMAND_UNRECOGNIZED_COMMAND;
	}
}

PrepareResult prepare_insert(InputBuffer* input_buffer, Statement* statement) {
	statement->type = STATEMENT_INSERT;
	std::stringstream ss(input_buffer->buffer);

	std::string keyword; 
	int id;
	std::string username, email;

	if (!(ss >> keyword >> id >> username >> email)) {
		return PREPARE_SYNTAX_ERROR;
	}

	if (id < 0) {
		return PREPARE_NEGATIVE_ID;
	}

	if (username.length() > COLUMN_USERNAME_SIZE || email.length() > COLUMN_EMAIL_SIZE) {
		return PREPARE_STRING_TOO_LONG;
	}

	statement->row_to_insert.id = id;

	strncpy(statement->row_to_insert.username, username.c_str(), COLUMN_USERNAME_SIZE);
	strncpy(statement->row_to_insert.email, email.c_str(), COLUMN_EMAIL_SIZE);

	return PREPARE_SUCCESS;
}

PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement) {
	std::string input(input_buffer->buffer);

	if (input.substr(0, 6) == "insert") {
		return prepare_insert(input_buffer, statement);
	}

	if (input == "select") {
		statement->type = STATEMENT_SELECT;
		return PREPARE_SUCCESS;
	}

	return PREPARE_UNRECOGNIZED_STATEMENT;
}

ExecuteResult execute_insert(Statement* statement, Table* table) {
	if (table->num_rows >= TABLE_MAX_ROWS) {
		return EXECUTE_TABLE_FULL;
	}

	Row* row_to_insert = &(statement->row_to_insert);

	serialize_row(row_to_insert, row_slot(table, table->num_rows));
	table->num_rows += 1;

	return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement* statement, Table* table) {
	Row row;
	for (uint32_t i = 0; i < table->num_rows; i++) {
		deserialize_row(row_slot(table, i), &row);
		print_row(&row);
	}
	return EXECUTE_SUCCESS;

}

ExecuteResult execute_statement(Statement* statement, Table* table) {
	switch (statement->type) {
	case (STATEMENT_INSERT):
		return execute_insert(statement, table);
	case(STATEMENT_SELECT):
		return execute_select(statement, table);
	}
}

Pager* pager_open(const char* filename) {
	int fd = _open(filename, O_RDWR | O_CREAT, S_IWRITE | S_IREAD);

	if (fd == -1) {
		std::cout << "Unable to open file" << std::endl;
		exit(EXIT_FAILURE);
	}

	off_t file_length = _lseek(fd, 0, SEEK_END);

	Pager* pager = (Pager*)malloc(sizeof(Pager));
	pager->file_descriptor = fd;
	pager->file_length = file_length;

	for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
		pager->pages[i] = NULL;
	}
	return pager;
}

Table* db_open(const char* filename) {
	Pager* pager = pager_open(filename);
	uint32_t num_rows = pager->file_length / ROW_SIZE;

	Table* table = (Table*)malloc(sizeof(Table));
	table->pager = pager;
	table->num_rows = num_rows;
	return table;
}






int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cout << "Must supply a database filename" << std::endl;
		exit(EXIT_FAILURE);
	}

	char* filename = argv[1];
	Table* table = db_open(filename);

	InputBuffer* input_buffer = new_input_buffer();
	while (true) {
		print_prompt();
		read_input(input_buffer);

		if (input_buffer->buffer[0] == '.') {
			switch (do_meta_command(input_buffer, table)) {
			case (META_COMMAND_SUCCESS):
				continue;
			case (META_COMMAND_UNRECOGNIZED_COMMAND):
				std::cout << "Unrecognized Command " << input_buffer->buffer << "\n";
				continue;
			}
		}

		Statement statement;
		PrepareResult prepare_result = prepare_statement(input_buffer, &statement);

		switch (prepare_result) {
		case (PREPARE_SUCCESS):
			break;
		case (PREPARE_SYNTAX_ERROR):
			std::cout << "Syntax error. Could not parse statement" << std::endl;
			continue;
		case (PREPARE_UNRECOGNIZED_STATEMENT):
			std::cout << "Unrecognized keyword at start of: " << input_buffer->buffer << "\n";
			continue;
		case (PREPARE_NEGATIVE_ID):
			std::cout << "ID must be positive" << std::endl;
			continue;
		case (PREPARE_STRING_TOO_LONG):
			std::cout << "String too long" << std::endl;
			continue;
		}

		switch (execute_statement(&statement, table)) {
		case (EXECUTE_SUCCESS):
			std::cout << "Executed" << std::endl;
			break;
		case(EXECUTE_TABLE_FULL):
			std::cout << ("Error: Table full") << std::endl;
			break;
		}
	}

}