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
#include <string.h>

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255
#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)
#define TABLE_MAX_PAGES 100
#define INVALID_PAGE_NUM UINT32_MAX

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

const uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
const uint32_t NODE_TYPE_OFFSET = 0;
const uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
const uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
const uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
const uint8_t COMMON_NODE_HEADER_SIZE = NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;

const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_NEXT_LEAF_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NEXT_LEAF_OFFSET = LEAF_NODE_NUM_CELLS_OFFSET + LEAF_NODE_NUM_CELLS_SIZE;
const uint32_t LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE + LEAF_NODE_NEXT_LEAF_SIZE;

const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_KEY_OFFSET = 0;
const uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE;
const uint32_t LEAF_NODE_VALUE_OFFSET = LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;
const uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
const uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_MAX_CELLS = LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;

const uint32_t LEAF_NODE_RIGHT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) / 2;
const uint32_t LEAF_NODE_LEFT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) - LEAF_NODE_RIGHT_SPLIT_COUNT;

const uint32_t INTERNAL_NODE_NUM_KEYS_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t INTERNAL_NODE_RIGHT_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET = INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE;
const uint32_t INTERNAL_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + INTERNAL_NODE_NUM_KEYS_SIZE + INTERNAL_NODE_RIGHT_CHILD_SIZE;

const uint32_t INTERNAL_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CELL_SIZE = INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE;
const uint32_t INTERNAL_NODE_MAX_CELLS = (PAGE_SIZE - INTERNAL_NODE_HEADER_SIZE) / INTERNAL_NODE_CELL_SIZE;

enum NodeType { NODE_INTERNAL, NODE_LEAF, NODE_TERMINAL };
enum StatementType { STATEMENT_INSERT, STATEMENT_SELECT };
enum MetaCommanResult { META_COMMAND_SUCCESS, META_COMMAND_UNRECOGNIZED_COMMAND };
enum ExecuteResult { EXECUTE_TABLE_FULL, EXECUTE_SUCCESS, EXECUTE_DUPLICATE_KEY };
enum PrepareResult { PREPARE_SUCCESS, PREPARE_NEGATIVE_ID, PREPARE_UNRECOGNIZED_STATEMENT, PREPARE_STRING_TOO_LONG, PREPARE_SYNTAX_ERROR };

struct Pager {
	int file_descriptor;
	uint32_t file_length;
	uint32_t num_pages;
	void* pages[TABLE_MAX_PAGES];
};

struct Table {
	uint32_t root_page_num;
	Pager* pager;
};

struct Cursor {
	Table* table;
	uint32_t page_num;
	uint32_t cell_num;
	bool end_of_table;
};

struct InputBuffer {
	char* buffer;
	size_t buffer_length;
	ssize_t input_length;
};

struct Statement {
	StatementType type;
	Row row_to_insert;
};

void* get_page(Pager* pager, uint32_t page_num);
uint32_t get_unused_page_num(Pager* pager);
void initialize_internal_node(void* node);
void create_new_root(Table* table, uint32_t right_child_page_num);
void internal_node_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num);
void internal_node_split_and_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num);
void update_internal_node_key(void* node, uint32_t old_key, uint32_t new_key);
uint32_t internal_node_find_child(void* node, uint32_t key);
uint32_t* internal_node_num_keys(void* node);
uint32_t* internal_node_right_child(void* node);
uint32_t* internal_node_child(void* node, uint32_t child_num);
uint32_t* internal_node_key(void* node, uint32_t key_num);
uint32_t* internal_node_cell(void* node, uint32_t cell_num);
uint32_t* node_parent(void* node);
NodeType get_node_type(void* node);
bool is_node_root(void* node);
void set_node_root(void* node, bool is_root);
uint32_t get_node_max_key(Pager* pager, void* node);

void indent(uint32_t level) {
	for (uint32_t i = 0; i < level; i++) {
		std::cout << "  ";
	}
}

void print_constants() {
	std::cout << "ROW_SIZE: " << ROW_SIZE << std::endl;
	std::cout << "COMMON_NODE_HEADER_SIZE: " << (int)COMMON_NODE_HEADER_SIZE << std::endl;
	std::cout << "LEAF_NODE_HEADER_SIZE: " << LEAF_NODE_HEADER_SIZE << std::endl;
	std::cout << "LEAF_NODE_CELL_SIZE: " << LEAF_NODE_CELL_SIZE << std::endl;
	std::cout << "LEAF_NODE_SPACE_FOR_CELLS: " << LEAF_NODE_SPACE_FOR_CELLS << std::endl;
	std::cout << "LEAF_NODE_MAX_CELLS: " << LEAF_NODE_MAX_CELLS << std::endl;
}

void* get_page(Pager* pager, uint32_t page_num) {
	if (page_num > TABLE_MAX_PAGES) {
		std::cout << "Tried to fetch page number out of bounds. " << page_num << " > " << TABLE_MAX_PAGES << std::endl;
		exit(EXIT_FAILURE);
	}
	if (pager->pages[page_num] == NULL) {
		void* page = malloc(PAGE_SIZE);
		memset(page, 0, PAGE_SIZE);
		uint32_t num_pages = pager->file_length / PAGE_SIZE;
		if (pager->file_length % PAGE_SIZE) {
			num_pages += 1;
		}
		if (page_num < num_pages) {
			_lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
			ssize_t bytes_read = _read(pager->file_descriptor, page, PAGE_SIZE);
			if (bytes_read == -1) {
				std::cout << "Error reading file: " << errno << std::endl;
				exit(EXIT_FAILURE);
			}
		}
		pager->pages[page_num] = page;
		if (page_num >= pager->num_pages) {
			pager->num_pages = page_num + 1;
		}
	}
	return pager->pages[page_num];
}

NodeType get_node_type(void* node) {
	uint8_t value = *((uint8_t*)node + NODE_TYPE_OFFSET);
	return (NodeType)value;
}

void set_node_type(void* node, NodeType type) {
	uint8_t value = (uint8_t)type;
	*((uint8_t*)node + NODE_TYPE_OFFSET) = value;
}

bool is_node_root(void* node) {
	uint8_t value = *((uint8_t*)node + IS_ROOT_OFFSET);
	return (bool)value;
}

void set_node_root(void* node, bool is_root) {
	uint8_t value = is_root;
	*((uint8_t*)node + IS_ROOT_OFFSET) = value;
}

uint32_t* leaf_node_num_cells(void* node) {
	return (uint32_t*)((uint8_t*)node + LEAF_NODE_NUM_CELLS_OFFSET);
}

void* leaf_node_cell(void* node, uint32_t cell_num) {
	return (uint8_t*)node + LEAF_NODE_HEADER_SIZE + (cell_num * LEAF_NODE_CELL_SIZE);
}

uint32_t* leaf_node_key(void* node, uint32_t cell_num) {
	return (uint32_t*)leaf_node_cell(node, cell_num);
}

void* leaf_node_value(void* node, uint32_t cell_num) {
	return (uint8_t*)leaf_node_cell(node, cell_num) + LEAF_NODE_KEY_SIZE;
}

uint32_t* leaf_node_next_leaf(void* node) {
	return (uint32_t*)((uint8_t*)node + LEAF_NODE_NEXT_LEAF_OFFSET);
}

uint32_t* internal_node_num_keys(void* node) {
	return (uint32_t*)((uint8_t*)node + INTERNAL_NODE_NUM_KEYS_OFFSET);
}

uint32_t* internal_node_right_child(void* node) {
	return (uint32_t*)((uint8_t*)node + INTERNAL_NODE_RIGHT_CHILD_OFFSET);
}

uint32_t* internal_node_cell(void* node, uint32_t cell_num) {
	return (uint32_t*)((uint8_t*)node + INTERNAL_NODE_HEADER_SIZE + (cell_num * INTERNAL_NODE_CELL_SIZE));
}

uint32_t* internal_node_key(void* node, uint32_t key_num) {
	return (uint32_t*)((uint8_t*)internal_node_cell(node, key_num) + INTERNAL_NODE_CHILD_SIZE);
}

uint32_t* node_parent(void* node) {
	return (uint32_t*)((uint8_t*)node + PARENT_POINTER_OFFSET);
}

uint32_t* internal_node_child(void* node, uint32_t child_num) {
	uint32_t num_keys = *internal_node_num_keys(node);
	if (child_num > num_keys) {
		std::cout << "Tried to access child_num: " << child_num << " > num_keys: " << num_keys << std::endl;
		exit(EXIT_FAILURE);
	}
	else if (child_num == num_keys) {
		uint32_t* right_child = internal_node_right_child(node);
		if (*right_child == INVALID_PAGE_NUM) {
			std::cout << "Tried to access right child of node, but was invalid page" << std::endl;
			exit(EXIT_FAILURE);
		}
		return right_child;
	}
	else {
		uint32_t* child = (uint32_t*)internal_node_cell(node, child_num);
		if (*child == INVALID_PAGE_NUM) {
			std::cout << "Tried to access child " << child_num << " of node, but was invalid page" << std::endl;
			exit(EXIT_FAILURE);
		}
		return child;
	}
}

uint32_t get_node_max_key(Pager* pager, void* node) {
	if (get_node_type(node) == NODE_LEAF) {
		return *leaf_node_key(node, *leaf_node_num_cells(node) - 1);
	}
	void* right_child = get_page(pager, *internal_node_right_child(node));
	return get_node_max_key(pager, right_child);
}

uint32_t internal_node_find_child(void* node, uint32_t key) {
	uint32_t num_keys = *internal_node_num_keys(node);
	uint32_t min_index = 0;
	uint32_t max_index = num_keys;
	while (min_index != max_index) {
		uint32_t index = (min_index + max_index) / 2;
		uint32_t key_to_right = *internal_node_key(node, index);
		if (key_to_right >= key) {
			max_index = index;
		}
		else {
			min_index = index + 1;
		}
	}
	return min_index;
}

void update_internal_node_key(void* node, uint32_t old_key, uint32_t new_key) {
	uint32_t old_child_index = internal_node_find_child(node, old_key);
	uint32_t num_keys = *internal_node_num_keys(node);
	if (old_child_index < num_keys) {
		*internal_node_key(node, old_child_index) = new_key;
	}
}

uint32_t get_unused_page_num(Pager* pager) {
	return pager->num_pages++;
}

void initialize_leaf_node(void* node) {
	set_node_type(node, NODE_LEAF);
	set_node_root(node, false);
	*leaf_node_num_cells(node) = 0;
	*leaf_node_next_leaf(node) = 0;
}

void initialize_internal_node(void* node) {
	set_node_type(node, NODE_INTERNAL);
	set_node_root(node, false);
	*internal_node_num_keys(node) = 0;
	*internal_node_right_child(node) = INVALID_PAGE_NUM;
}

void create_new_root(Table* table, uint32_t right_child_page_num) {
	void* root = get_page(table->pager, table->root_page_num);
	void* right_child = get_page(table->pager, right_child_page_num);
	uint32_t left_child_page_num = get_unused_page_num(table->pager);
	void* left_child = get_page(table->pager, left_child_page_num);

	memcpy(left_child, root, PAGE_SIZE);
	set_node_root(left_child, false);

	if (get_node_type(left_child) == NODE_INTERNAL) {
		for (uint32_t i = 0; i < *internal_node_num_keys(left_child); i++) {
			void* child = get_page(table->pager, *internal_node_child(left_child, i));
			*node_parent(child) = left_child_page_num;
		}
		void* child = get_page(table->pager, *internal_node_right_child(left_child));
		*node_parent(child) = left_child_page_num;
	}

	initialize_internal_node(root);
	set_node_root(root, true);
	*internal_node_num_keys(root) = 1;

	*internal_node_child(root, 0) = left_child_page_num;
	*internal_node_key(root, 0) = get_node_max_key(table->pager, left_child);
	*internal_node_right_child(root) = right_child_page_num;

	*node_parent(left_child) = table->root_page_num;
	*node_parent(right_child) = table->root_page_num;
}

void internal_node_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num) {
	void* parent = get_page(table->pager, parent_page_num);
	void* child = get_page(table->pager, child_page_num);

	uint32_t child_max_key = get_node_max_key(table->pager, child);
	uint32_t index = internal_node_find_child(parent, child_max_key);
	uint32_t original_num_keys = *internal_node_num_keys(parent);

	if (original_num_keys >= INTERNAL_NODE_MAX_CELLS) {
		internal_node_split_and_insert(table, parent_page_num, child_page_num);
		return;
	}

	uint32_t right_child_page_num = *internal_node_right_child(parent);

	if (right_child_page_num == INVALID_PAGE_NUM) {
		*internal_node_right_child(parent) = child_page_num;
		return;
	}

	void* right_child = get_page(table->pager, right_child_page_num);

	*internal_node_num_keys(parent) = original_num_keys + 1;

	if (child_max_key > get_node_max_key(table->pager, right_child)) {
		*internal_node_child(parent, original_num_keys) = right_child_page_num;
		*internal_node_key(parent, original_num_keys) = get_node_max_key(table->pager, right_child);
		*internal_node_right_child(parent) = child_page_num;
	}
	else {
		for (uint32_t i = original_num_keys; i > index; i--) {
			void* destination = internal_node_cell(parent, i);
			void* source = internal_node_cell(parent, i - 1);
			memcpy(destination, source, INTERNAL_NODE_CELL_SIZE);
		}
		*internal_node_child(parent, index) = child_page_num;
		*internal_node_key(parent, index) = child_max_key;
	}
}

void internal_node_split_and_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num) {
	uint32_t old_page_num = parent_page_num;
	void* old_node = get_page(table->pager, parent_page_num);
	uint32_t old_max = get_node_max_key(table->pager, old_node);

	void* child = get_page(table->pager, child_page_num);
	uint32_t child_max = get_node_max_key(table->pager, child);

	uint32_t new_page_num = get_unused_page_num(table->pager);

	bool splitting_root = is_node_root(old_node);

	void* parent;
	void* new_node = nullptr;

	if (splitting_root) {
		create_new_root(table, new_page_num);
		parent = get_page(table->pager, table->root_page_num);
		old_page_num = *internal_node_child(parent, 0);
		old_node = get_page(table->pager, old_page_num);
	}
	else {
		parent = get_page(table->pager, *node_parent(old_node));
		new_node = get_page(table->pager, new_page_num);
		initialize_internal_node(new_node);
	}

	uint32_t* old_num_keys = internal_node_num_keys(old_node);

	uint32_t cur_page_num = *internal_node_right_child(old_node);
	void* cur = get_page(table->pager, cur_page_num);
	internal_node_insert(table, new_page_num, cur_page_num);
	*node_parent(cur) = new_page_num;
	*internal_node_right_child(old_node) = INVALID_PAGE_NUM;

	for (int i = INTERNAL_NODE_MAX_CELLS - 1; i > INTERNAL_NODE_MAX_CELLS / 2; i--) {
		cur_page_num = *internal_node_child(old_node, i);
		cur = get_page(table->pager, cur_page_num);
		internal_node_insert(table, new_page_num, cur_page_num);
		*node_parent(cur) = new_page_num;
		(*old_num_keys)--;
	}

	*internal_node_right_child(old_node) = *internal_node_child(old_node, *old_num_keys - 1);
	(*old_num_keys)--;

	uint32_t max_after_split = get_node_max_key(table->pager, old_node);
	uint32_t destination_page_num = (child_max < max_after_split) ? old_page_num : new_page_num;
	internal_node_insert(table, destination_page_num, child_page_num);
	*node_parent(child) = destination_page_num;

	update_internal_node_key(parent, old_max, get_node_max_key(table->pager, old_node));

	if (!splitting_root) {
		internal_node_insert(table, *node_parent(old_node), new_page_num);
		*node_parent(new_node) = *node_parent(old_node);
	}
}

Cursor* leaf_node_find(Table* table, uint32_t page_num, uint32_t key) {
	void* node = get_page(table->pager, page_num);
	uint32_t num_cells = *leaf_node_num_cells(node);
	Cursor* cursor = (Cursor*)malloc(sizeof(Cursor));
	cursor->table = table;
	cursor->page_num = page_num;
	uint32_t min_index = 0;
	uint32_t one_past_max_index = num_cells;
	while (one_past_max_index != min_index) {
		uint32_t index = (min_index + one_past_max_index) / 2;
		uint32_t key_at_index = *leaf_node_key(node, index);
		if (key == key_at_index) {
			cursor->cell_num = index;
			return cursor;
		}
		if (key < key_at_index) {
			one_past_max_index = index;
		}
		else {
			min_index = index + 1;
		}
	}
	cursor->cell_num = min_index;
	return cursor;
}

Cursor* internal_node_find(Table* table, uint32_t page_num, uint32_t key) {
	void* node = get_page(table->pager, page_num);
	uint32_t child_index = internal_node_find_child(node, key);
	uint32_t child_num = *internal_node_child(node, child_index);
	void* child = get_page(table->pager, child_num);
	switch (get_node_type(child)) {
	case NODE_LEAF:
		return leaf_node_find(table, child_num, key);
	case NODE_INTERNAL:
		return internal_node_find(table, child_num, key);
	}
	return NULL;
}

Cursor* table_find(Table* table, uint32_t key) {
	uint32_t root_page_num = table->root_page_num;
	void* root_node = get_page(table->pager, root_page_num);
	if (get_node_type(root_node) == NODE_LEAF) {
		return leaf_node_find(table, root_page_num, key);
	}
	else {
		return internal_node_find(table, root_page_num, key);
	}
}

Cursor* table_start(Table* table) {
	Cursor* cursor = table_find(table, 0);
	void* node = get_page(table->pager, cursor->page_num);
	uint32_t num_cells = *leaf_node_num_cells(node);
	cursor->end_of_table = (num_cells == 0);
	return cursor;
}

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

void leaf_node_split_and_insert(Cursor* cursor, uint32_t key, Row* value) {
	void* old_node = get_page(cursor->table->pager, cursor->page_num);
	uint32_t old_max = get_node_max_key(cursor->table->pager, old_node);
	uint32_t new_page_num = get_unused_page_num(cursor->table->pager);
	void* new_node = get_page(cursor->table->pager, new_page_num);
	initialize_leaf_node(new_node);
	*node_parent(new_node) = *node_parent(old_node);
	*leaf_node_next_leaf(new_node) = *leaf_node_next_leaf(old_node);
	*leaf_node_next_leaf(old_node) = new_page_num;
	for (int32_t i = LEAF_NODE_MAX_CELLS; i >= 0; i--) {
		void* destination_node;
		if (i >= LEAF_NODE_LEFT_SPLIT_COUNT) {
			destination_node = new_node;
		}
		else {
			destination_node = old_node;
		}
		uint32_t index_within_node = i % LEAF_NODE_LEFT_SPLIT_COUNT;
		void* destination = leaf_node_cell(destination_node, index_within_node);
		if (i == cursor->cell_num) {
			serialize_row(value, leaf_node_value(destination_node, index_within_node));
			*leaf_node_key(destination_node, index_within_node) = key;
		}
		else if (i > cursor->cell_num) {
			memcpy(destination, leaf_node_cell(old_node, i - 1), LEAF_NODE_CELL_SIZE);
		}
		else {
			memcpy(destination, leaf_node_cell(old_node, i), LEAF_NODE_CELL_SIZE);
		}
	}
	*(leaf_node_num_cells(old_node)) = LEAF_NODE_LEFT_SPLIT_COUNT;
	*(leaf_node_num_cells(new_node)) = LEAF_NODE_RIGHT_SPLIT_COUNT;
	if (is_node_root(old_node)) {
		create_new_root(cursor->table, new_page_num);
	}
	else {
		uint32_t parent_page_num = *node_parent(old_node);
		uint32_t new_max = get_node_max_key(cursor->table->pager, old_node);
		void* parent = get_page(cursor->table->pager, parent_page_num);
		update_internal_node_key(parent, old_max, new_max);
		internal_node_insert(cursor->table, parent_page_num, new_page_num);
	}
}

void leaf_node_insert(Cursor* cursor, uint32_t key, Row* value) {
	void* node = get_page(cursor->table->pager, cursor->page_num);
	uint32_t num_cells = *leaf_node_num_cells(node);
	if (num_cells >= LEAF_NODE_MAX_CELLS) {
		leaf_node_split_and_insert(cursor, key, value);
		return;
	}
	if (cursor->cell_num < num_cells) {
		for (uint32_t i = num_cells; i > cursor->cell_num; i--) {
			memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i - 1), LEAF_NODE_CELL_SIZE);
		}
	}
	*(leaf_node_num_cells(node)) += 1;
	*(leaf_node_key(node, cursor->cell_num)) = key;
	serialize_row(value, leaf_node_value(node, cursor->cell_num));
}

void* cursor_value(Cursor* cursor) {
	uint32_t page_num = cursor->page_num;
	void* page = get_page(cursor->table->pager, page_num);
	return leaf_node_value(page, cursor->cell_num);
}

void cursor_advance(Cursor* cursor) {
	uint32_t page_num = cursor->page_num;
	void* node = get_page(cursor->table->pager, page_num);
	cursor->cell_num += 1;
	if (cursor->cell_num >= (*leaf_node_num_cells(node))) {
		uint32_t next_page_num = *leaf_node_next_leaf(node);
		if (next_page_num == 0) {
			cursor->end_of_table = true;
		}
		else {
			cursor->page_num = next_page_num;
			cursor->cell_num = 0;
		}
	}
}

Pager* pager_open(const char* filename) {
	int fd = _open(filename, _O_RDWR | _O_CREAT | _O_BINARY, _S_IWRITE | _S_IREAD);
	if (fd == -1) {
		std::cout << "Unable to open file" << std::endl;
		exit(EXIT_FAILURE);
	}
	off_t file_length = _lseek(fd, 0, SEEK_END);
	Pager* pager = (Pager*)malloc(sizeof(Pager));
	pager->file_descriptor = fd;
	pager->file_length = file_length;
	pager->num_pages = (uint32_t)(file_length / PAGE_SIZE);
	if (file_length % PAGE_SIZE != 0) {
		std::cout << "Db file is not a whole number of pages. Corrupt file" << std::endl;
		exit(EXIT_FAILURE);
	}
	for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
		pager->pages[i] = NULL;
	}
	return pager;
}

Table* db_open(const char* filename) {
	Pager* pager = pager_open(filename);
	Table* table = (Table*)malloc(sizeof(Table));
	table->pager = pager;
	table->root_page_num = 0;
	if (pager->num_pages == 0) {
		void* root_node = get_page(pager, 0);
		initialize_leaf_node(root_node);
		set_node_root(root_node, true);
	}
	return table;
}

void pager_flush(Pager* pager, uint32_t page_num) {
	if (pager->pages[page_num] == NULL) {
		std::cout << "Tried to flush null page" << std::endl;
		exit(EXIT_FAILURE);
	}
	off_t offset = _lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
	if (offset == -1) {
		std::cout << "Error seeking: " << errno << std::endl;
		exit(EXIT_FAILURE);
	}
	ssize_t bytes_written = _write(pager->file_descriptor, pager->pages[page_num], PAGE_SIZE);
	if (bytes_written == -1) {
		std::cout << "Error writing: " << errno << std::endl;
		exit(EXIT_FAILURE);
	}
}

void db_close(Table* table) {
	Pager* pager = table->pager;
	for (uint32_t i = 0; i < pager->num_pages; i++) {
		if (pager->pages[i] == NULL) {
			continue;
		}
		pager_flush(pager, i);
		free(pager->pages[i]);
		pager->pages[i] = NULL;
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
	free(pager);
	free(table);
}

void print_row(Row* row) {
	std::cout << "(" << row->id << ", " << row->username << ", " << row->email << ")" << std::endl;
}

void print_tree(Pager* pager, uint32_t page_num, uint32_t indentation_level) {
	void* node = get_page(pager, page_num);
	uint32_t num_keys, child;
	switch (get_node_type(node)) {
	case (NODE_LEAF):
		num_keys = *leaf_node_num_cells(node);
		indent(indentation_level);
		std::cout << "- leaf (size " << num_keys << ")" << std::endl;
		for (uint32_t i = 0; i < num_keys; i++) {
			indent(indentation_level + 1);
			std::cout << "- " << *leaf_node_key(node, i) << std::endl;
		}
		break;
	case (NODE_INTERNAL):
		num_keys = *internal_node_num_keys(node);
		indent(indentation_level);
		std::cout << "- internal (size " << num_keys << ")" << std::endl;
		if (num_keys > 0) {
			for (uint32_t i = 0; i < num_keys; i++) {
				child = *internal_node_child(node, i);
				print_tree(pager, child, indentation_level + 1);
				indent(indentation_level + 1);
				std::cout << "- key " << *internal_node_key(node, i) << std::endl;
			}
			child = *internal_node_right_child(node);
			print_tree(pager, child, indentation_level + 1);
		}
		break;
	}
}

MetaCommanResult do_meta_command(InputBuffer* input_buffer, Table** table) {
	if (strcmp(input_buffer->buffer, ".exit") == 0) {
		db_close(*table);
		exit(EXIT_SUCCESS);
	}
	else if (strncmp(input_buffer->buffer, ".open ", 6) == 0) {
		const char* new_filename = input_buffer->buffer + 6;
		if (strlen(new_filename) == 0) {
			std::cout << "Usage: .open <filename>" << std::endl;
			return META_COMMAND_SUCCESS;
		}
		db_close(*table);
		*table = db_open(new_filename);
		std::cout << "Opened database: " << new_filename << std::endl;
		return META_COMMAND_SUCCESS;
	}
	else if (strcmp(input_buffer->buffer, ".help") == 0) {
		std::cout << "Commands:" << std::endl;
		std::cout << "  .help                  - Show this help message" << std::endl;
		std::cout << "  .exit                  - Save and exit the database" << std::endl;
		std::cout << "  .open <filename>       - Close current db and open/create another" << std::endl;
		std::cout << "  .btree                 - Print the B-tree structure of the table" << std::endl;
		std::cout << "  .constants             - Print internal constants (page size, cell size, etc)" << std::endl;
		std::cout << std::endl;
		std::cout << "SQL Statements:" << std::endl;
		std::cout << "  insert <id> <username> <email>  - Insert a new row" << std::endl;
		std::cout << "  select                          - Print all rows" << std::endl;
		return META_COMMAND_SUCCESS;
	}
	else if (strcmp(input_buffer->buffer, ".btree") == 0) {
		std::cout << "Tree: " << std::endl;
		print_tree((*table)->pager, (*table)->root_page_num, 0);
	}
	else if (strcmp(input_buffer->buffer, ".constants") == 0) {
		std::cout << "Constants: " << std::endl;
		print_constants();
		return META_COMMAND_SUCCESS;
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
	statement->row_to_insert.username[COLUMN_USERNAME_SIZE] = '\0';
	strncpy(statement->row_to_insert.email, email.c_str(), COLUMN_EMAIL_SIZE);
	statement->row_to_insert.email[COLUMN_EMAIL_SIZE] = '\0';
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
	Row* row_to_insert = &(statement->row_to_insert);
	uint32_t key_to_insert = row_to_insert->id;
	Cursor* cursor = table_find(table, key_to_insert);
	void* node = get_page(table->pager, cursor->page_num);
	uint32_t num_cells = *leaf_node_num_cells(node);
	if (cursor->cell_num < num_cells) {
		uint32_t key_at_index = *leaf_node_key(node, cursor->cell_num);
		if (key_at_index == key_to_insert) {
			free(cursor);
			return EXECUTE_DUPLICATE_KEY;
		}
	}
	leaf_node_insert(cursor, row_to_insert->id, row_to_insert);
	free(cursor);
	return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement* statement, Table* table) {
	Cursor* cursor = table_start(table);
	Row row;
	while (!(cursor->end_of_table)) {
		deserialize_row(cursor_value(cursor), &row);
		print_row(&row);
		cursor_advance(cursor);
	}
	free(cursor);
	return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement* statement, Table* table) {
	switch (statement->type) {
	case (STATEMENT_INSERT):
		return execute_insert(statement, table);
	case (STATEMENT_SELECT):
		return execute_select(statement, table);
	}
	return EXECUTE_SUCCESS;
}

InputBuffer* new_input_buffer() {
	InputBuffer* input_buffer = (InputBuffer*)malloc(sizeof(InputBuffer));
	input_buffer->buffer = NULL;
	input_buffer->buffer_length = 0;
	input_buffer->input_length = 0;
	return input_buffer;
}

void print_prompt() { std::cout << "db > "; }

void read_input(InputBuffer* input_buffer) {
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

bool file_exists(const char* filename) {
	struct stat st;
	return stat(filename, &st) == 0;
}

int main(int argc, char* argv[]) {
	std::string filename_str;
	if (argc >= 2) {
		filename_str = argv[1];
	}
	else {
		std::cout << "No database file specified." << std::endl;
		std::cout << "Enter a filename to open or create: ";
		std::getline(std::cin, filename_str);
		if (filename_str.empty()) {
			std::cout << "No filename provided. Exiting." << std::endl;
			exit(EXIT_FAILURE);
		}
		if (!file_exists(filename_str.c_str())) {
			std::cout << "File '" << filename_str << "' not found. Create it? (y/n): ";
			std::string answer;
			std::getline(std::cin, answer);
			if (answer != "y" && answer != "Y") {
				std::cout << "Exiting." << std::endl;
				exit(EXIT_FAILURE);
			}
		}
	}
	Table* table = db_open(filename_str.c_str());
	InputBuffer* input_buffer = new_input_buffer();
	std::cout << "Opened database: " << filename_str << std::endl;
	std::cout << "Use .help for a list of commands." << std::endl;
	while (true) {
		print_prompt();
		read_input(input_buffer);
		if (input_buffer->buffer[0] == '.') {
			switch (do_meta_command(input_buffer, &table)) {
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
		case (EXECUTE_DUPLICATE_KEY):
			std::cout << "Error: Duplicate key." << std::endl;
			break;
		case (EXECUTE_TABLE_FULL):
			std::cout << ("Error: Table full") << std::endl;
			break;
		}
	}
	return 0;
}