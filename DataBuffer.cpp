
#include "DataBuffer.h"
#include "log.h"
#include <stdlib.h>
#include "app_exceptions.h"


DataBuffer::DataBuffer(int initial_size) {
	_bytes_allocated = initial_size;
	_storage         = (char *) malloc(initial_size);
	_position        = 0;
	_bytes_written   = 0;
	_CHUNK_SIZE_     = 8192;
}

DataBuffer::~DataBuffer() {
	free(_storage);
}

void DataBuffer::read_next_chunk(SOCKET socket) {
	reserve_capacity_from_end(_CHUNK_SIZE_);
	int count = recv(socket, end_of_data(), _CHUNK_SIZE_, 0);
	if(count <= 0) {
		// socket is broken
		// // LOGE << "DataBuffer::read_next_chunk returned <= 0";
		throw EventConnectionBroken();
	}
	_bytes_written += count;
}

void DataBuffer::reserve_capacity_from_start(int amount_needed) {
	int existing_space = _bytes_allocated - _position;
	if(existing_space < amount_needed) {
		int space_to_allocate = _position + amount_needed + (_CHUNK_SIZE_ * 4); // add extra space to allocation to reduce number of realloc()'s
		_storage              = (char *) realloc(_storage, space_to_allocate);
		if(_storage == NULL) {
			LOGE << "DataBuffer::reserve_capacity_from_start realloc() error amount_needed=" << amount_needed << " space_to_allocate=" << space_to_allocate;
			throw ErrorOutOfMemory("DataBuffer::realloc()");
		}
		_bytes_allocated = space_to_allocate;
	}
}

void DataBuffer::reserve_capacity_from_end(int amount_needed) {
	if(_bytes_allocated < amount_needed + _bytes_written) {
		int space_to_allocate = amount_needed + _bytes_written + (_CHUNK_SIZE_ * 4); // add extra space to allocation to reduce number of realloc()'s
		_storage              = (char *) realloc(_storage, space_to_allocate);
		if(_storage == NULL) {
			LOGE << "DataBuffer::reserve_capacity_from_end realloc() error amount_needed=" << amount_needed << " space_to_allocate=" << space_to_allocate;
			throw ErrorOutOfMemory("DataBuffer::realloc()");
		}
		_bytes_allocated = space_to_allocate;
	}
}

void DataBuffer::clear() {
	_bytes_written = 0;
	_position      = 0;
}

void DataBuffer::write_to_end(const char * data, int amount) {
	reserve_capacity_from_end(amount);
	memcpy(end_of_data(), data, amount);
	_bytes_written += amount;
}

DataBuffer::DataBuffer() {
	_bytes_allocated = 0;
	_storage         = NULL;
	_position        = 0;
	_bytes_written   = 0;
	_CHUNK_SIZE_     = 8192;
}



