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
#include "Devices430.h" // for ERASE_SGMT

// for debug messages
// TODO put someplace nice
static const char* const HAL_STATUS_NAMES[4] = {
		"HAL_OK",
		"HAL_ERROR",
		"HAL_BUSY",
		"HAL_TIMEOUT"
};

extern UART_HandleTypeDef huart2; // defined in main.c

void program_sequence() {
	printf("Starting\n");
	HAL_Delay(1000);

	uint8_t ret = 0;
	node_t *program = NULL;

	printf("Ready\n");

	uint8_t *c = NULL;
	uint16_t bytes_received = 0;

	uint16_t file_len = 0;

	char buf[16] = {0};
	uint16_t buflen = 0;

	__HAL_UART_FLUSH_DRREGISTER(&huart2);
	if ((ret = HAL_UARTEx_ReceiveToIdle(&huart2, (uint8_t*)&buf[0], 16, &buflen, TIMEOUT)) != HAL_OK)	// receive 2 bytes, LSB first
	{
		printf("Critical Error! Failed to receive file length ec=%s (0x%02x)\n", HAL_STATUS_NAMES[ret], ret);
		goto error;
	}
	printf("-vReceived %d bytes %s\n", buflen, buf);
	if(buflen != 2) {
		printf("Critical Error! Failed to receive exactly 2 bytes representing file length\n");
		goto error;
	}

	file_len = buf[0] + (buf[1] << 8);

	printf("Received %d\n", file_len);

	if (file_len < 12) // the smallest possible .hex file is 12 bytes. ":00000001FF" + EOF
	{
		printf("Critical Error! File is too short (<12 bytes)\n");
		goto error;
	}

	c = (uint8_t*) malloc(file_len);

	{
		uint8_t *cptr = c;
		while ((ret = HAL_UART_Receive(&huart2, cptr, 1, 4000)) == HAL_OK)
		{
			++cptr;

			if (cptr >= c + file_len) {
				break;
			}
			continue;
		}

		bytes_received = (uint16_t) (cptr - c);
		if (ret != HAL_OK)
		{
			printf("Critical Error! Timed out while waiting for file. Received %u/%u ec=%s (0x%02x)\n", cptr - c, file_len, HAL_STATUS_NAMES[ret], ret);
			goto error;
		}
	}

	printf("-vReceived file of length %u bytes\n", bytes_received);

	if (bytes_received < 12) // the smallest possible .hex file is 12 bytes. ":00000001FF" + EOF
	{
		printf("Critical Error! File is too short (<12 bytes)\n");
		goto error;
	}

	printf("Verifying .hex file...\n");
	ret = verify_hexfile_and_return_program_linklist(c, bytes_received, &program);
	if (ret != 0)
	{
		printf("Aborting...\n");
		goto error;
	}
	printf("-vSuccessfully verified .hex file\n");

	printf("Connecting to target...\n");
	uint16_t deviceID;
	word status = GetDevice(&deviceID);
	if (status != STATUS_OK)         // Set DeviceId
	{
		printf("Get device failed. status=0x%x\n", status);
		goto error;
	}                                    // time-out. (error: red LED is ON)
	printf("-vSuccessfully connected to target 0x%04x\n", deviceID);

	printf("Erasing target flash...\n");

	uint8_t main_segments[64] = { 0 };// 64 segments of main memory 512 bytes each 0x8000 to 0xffff (in reality only 4 segments are used in 2k flash but this is easier
	uint8_t info_segments[4] = { 0 }; // 4 segments of info memory 64 bytes each 0x1000 to 0x10ff. [0] is segment A and [3] is segment D

	// iterate through program linked list to find out which segments need to be erased
	for (node_t *iter = program; iter != NULL; iter = iter->next)// traverse the linked list
	{
		if (iter->address >= 0x8000) // technically 32kb flash region starts at 0x8000
		{
			//main
			uint8_t seg_index = (0xffff - iter->address) / 0x0200;
			uint8_t seg_index_of_last_byte = (0xffff - iter->address - 2*iter->length) / 0x0200;	// the segment that the last byte lies in. (*2 because length is number of words!)
			if (main_segments[seg_index] == 0)
			{
				printf("-vPlan to flash main segment %u (0x%04x)\n", seg_index, iter->address);
				main_segments[seg_index] = 1;
			}
			if (main_segments[seg_index_of_last_byte] == 0)
			{
				printf("-vCode overlaps into next segment!\n");
				printf("-vPlan to flash main segment %u (0x%04x)\n", seg_index_of_last_byte, iter->address);
				main_segments[seg_index_of_last_byte] = 1;
			}
		}
		else if (iter->address >= 0x1000 && iter->address <= 0x10ff)
		{
			//info
			uint8_t seg_index = (0x10ff - iter->address) / 0x40;
			if (info_segments[seg_index] == 0)
			{
				info_segments[seg_index] = 1;
				printf("-vNeed to flash info segment %c (0x%04x)\n", seg_index + 'A', iter->address);
			}
		}
//		else
//		{
//			should never reach here because the adress range is checked in verifyhexFile.
//			TODO verify the range someplace else.
//		}
	}

	uint32_t now = HAL_GetTick();
	for (int i = 0; i < 64; i++)
	{
		if (main_segments[i])
		{
			uint16_t address = 0xFE00 - 0x200 * i;

			EraseFLASH(ERASE_SGMT, address);
			if (EraseCheck(address, 0x0100) != STATUS_OK) // Check main memory erasure (Fxx2..9)
			{
				printf("Critical Error! Failed to erase main flash segment %u (begins 0x%04x)\n", i, address);
				goto error;
			}
			printf("-vSuccessfully erased flash segment %u (begins 0x%04x)\n", i, address);
		}
	}
	for (int i = 0; i < 4; i++)
	{
		if (info_segments[i])
		{
			uint16_t address = 0x10C0 - 0x40 * i;

			EraseFLASH(ERASE_SGMT, address);
			if (EraseCheck(address, 0x0020) != STATUS_OK) // Check main memory erasure (Fxx2..9)
			{
				if (i == 0) // segment A
				{
					printf("Error! Failed to erase info flash segment A (begins 0x10C0). This is a special segment which contains configuration data\n");
					continue;
				}

				printf("Critical Error! Failed to erase info flash segment %c (begins 0x%04x)\n", i + 'A', address);
				goto error;
			}
			printf("-vSuccessfully erased flash segment %c (begins 0x%04x)\n", i = 'A', address);
		}
	}
	uint32_t delta = HAL_GetTick() - now;
	printf("-vSuccessfully erased target flash\n");

	printf("Writing target flash...\n");
	now = HAL_GetTick();

	for (node_t *iter = program; iter != NULL; iter = iter->next)// traverse the linked list
	{
		WriteFLASH(iter->address, iter->length, iter->data);

		uint16_t *temp = (uint16_t*) malloc(iter->length * sizeof(uint16_t));
		ReadMemQuick(iter->address, iter->length, temp);

		printf("-v0x%04x: ", iter->address);
		for (int i = 0; i < iter->length; i++)
		{
			printf("%04x", temp[i]);
		}
		printf("\n");

		free(temp);

		if (VerifyMem(iter->address, iter->length, iter->data) != STATUS_OK)
		{
			printf("Critical Error! Verification of memory block 0x%04x failed!\n", iter->address);
			goto error;
		}
	}

	delta = HAL_GetTick() - now;
	printf("Successfully wrote target flash. Time elapsed %01lu:%02lu.%03lu\n", delta / 60000, (delta / 1000) % 60, delta % 1000);

error:
	printf("Shutting down JTAG connection...\n");
	ReleaseDevice(V_RESET);

	// cleanup
	free(c);
	free_linkedlist(program);

	printf("Exit\n");
}

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
