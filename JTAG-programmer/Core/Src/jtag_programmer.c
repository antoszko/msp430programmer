/*
 * jtag_programmer.c
 *
 *  Created on: Sep 10, 2023
 *      Author: stefan
 */

#include "jtag_programmer.h"

#include "JTAGfunc430.h"
#include "LowLevelFunc430.h"
#include "memory.h" // for memcpy
#include "stdlib.h" // for malloc

uint8_t is_valid_hex_char(uint8_t c)
{
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

uint8_t ascii_string_to_byte(const uint8_t* const string)
{
	uint8_t answer = 0;

	//process LSB first
	for (int i = 0; i < 2; i++)
	{
		if (string[i] <= '9') // its a digit
		{
			answer |= (string[i] - '0');
		}
		else if (string[i] <= 'F') // its an upper case letter
		{
			answer |= (string[i] - 'A' + 10);
		}
		else // its a lower case letter
		{
			answer |= (string[i] - 'a' + 10);
		}

		answer <<= (4 * (1-i));
	}
	return answer;
}

uint16_t ascii_string_to_address(const uint8_t* const string)
{
	uint16_t answer = (ascii_string_to_byte(string) << 8) |
					ascii_string_to_byte(string+2);
	return answer;
}

uint16_t ascii_string_to_word(const uint8_t* const string)
{
	return ascii_string_to_byte(string) |
			(ascii_string_to_byte(string+2) << 8);
}


uint8_t verify_hexfile_and_return_program_linklist(const uint8_t *const file, uint16_t file_size, node_t** program)
{
	const uint8_t * readptr= file;
	*program = NULL;

	while (readptr < file + file_size)	// for each line...
	{
		while (*readptr != ':' && readptr < file + file_size)	// ignore everything before the :
		{
			++readptr;
		}
		readptr += 1;

		if (!(readptr < file + file_size)) {
			printf("Critical Error! File not ended correctly. Ensure last line of file is ':00000001FF'\n");
			goto error;
		}

		if (!(is_valid_hex_char(readptr[0]) && is_valid_hex_char(readptr[1]))) {
			printf("Critical Error! First two bytes after : must be in the range '0'-'9' or 'A'-'F' or 'a'-'f'\n");
			goto error;
		}

		uint8_t byte_count = ascii_string_to_byte(readptr);
		readptr += 2;

		if (readptr + 4 + 2 + 2 * byte_count + 2 >= file + file_size) {
			printf("Critical Error! Length of data field exceeds number of bytes in .hex file\n");
			goto error;
		}

		uint16_t address = ascii_string_to_address(readptr);
		readptr += 4;

		uint8_t record_type = ascii_string_to_byte(readptr);
		readptr += 2;

		if (!(record_type == 0x00 || record_type == 0x01)) {
			printf("Critical Error! Encountered unsupported record type 0x%02x\n", record_type);
			goto error;
		}

		if (record_type == 0x00 && !(address >= 0xF800 && address <= 0xFFFF)) {
			printf("Critical Error! Address 0x%04x is outside of valid range (0xF800 - 0xFFFF)\n", address);
			goto error;
		}

		uint8_t data[255];// the maximum data per line is 255 bytes. https://en.wikipedia.org/wiki/Intel_HEX#Record_structure

		uint8_t checksum_calc = byte_count + (address & 0xFF)
				+ ((address & 0xFF00) >> 8) + record_type;

		for (int i = 0; i < byte_count; i++) {
			data[i] = ascii_string_to_byte(readptr);
			readptr += 2;

			checksum_calc += data[i];
		}

		uint8_t checksum = ascii_string_to_byte(readptr);
		readptr += 2;

		if (((checksum_calc + checksum) & 0xff) != 0x00) {
			printf( "Critical Error! Checksums dont match! Given: 0x%02x calculated: 0x%02x\n", checksum, checksum_calc);
			goto error;
		}

		if (record_type == 0x00) {
			// copy the data into the virtual flash
			node_t* next = (node_t*) malloc(sizeof(node_t));
			next->address = address;
			next->data = (uint16_t*)malloc(byte_count);
			memcpy(next->data, data, byte_count);		// copying array of uint8_t to array of uint16_t.
			next->length = byte_count / 2;
			next->next = *program;

			*program = next;

			printf("-vData: %u bytes @ 0x%04x cs=0x%02x\n", byte_count, address, checksum_calc);
		} else {
			printf("-vEof cs=0x%02x\n", checksum_calc);
			return 0;
		}
	}
	return 0;

error: // cleanup the linked list
	free_linkedlist(*program);
	return 1;
}

void free_linkedlist(node_t* head)
{
	while(head != NULL)
	{
		free(head->data);
		node_t* todelete = head;	// temp
		head = head->next;
		free(todelete);
	}
}
