
#ifndef _DATA_BUFFER_H_H
#define _DATA_BUFFER_H_H

#if defined(WIN32)
#	include "winsock2.h"
#endif

class DataBuffer
{
public:
	DataBuffer(int initial_size);
	~DataBuffer();

	char * start_of_data() { return _storage + _position; }
	char * end_of_data() { return _storage + _bytes_written; }
	int    bytes_stored() { return _bytes_written - _position; }
	void   consume_bytes(int amount) { _position += amount; }
	void   eliminate_parsed_data() { if(_position == _bytes_written) clear(); }

	void read_next_chunk(SOCKET socket);
	void reserve_capacity_from_start(int amount_needed);
	void reserve_capacity_from_end(int amount_needed);
	void clear();
	void write_to_end(const char * data, int amount);

	char * _storage;
	int    _bytes_allocated;
	int    _bytes_written;
	int    _position;

private:
	DataBuffer();
	int _CHUNK_SIZE_;
};

#endif // _DATA_BUFFER_H_H
