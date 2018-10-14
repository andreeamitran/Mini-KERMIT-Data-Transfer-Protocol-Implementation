#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

//build ACK/NAK package
msg ack_nak_pkg (char seq, char type) {
	mini_kermit *pkg = (mini_kermit *) malloc (sizeof(mini_kermit));
	//fill the fields
	pkg->soh = 0x01;
	pkg->len = 5;
	pkg->seq = seq;
	pkg->type = type;
	pkg->data = NULL;

	//calculate crc and make buffer for payload
	char *buf = (char *) malloc (8);
	memcpy (buf, pkg, 5);
	unsigned short crc = crc16_ccitt(buf, 5);

	pkg->check = crc;
	pkg->mark = 0x0D;
	buf[5] = pkg->check & 0xFF;
	buf[6] = (pkg->check >> 8) & 0xFF;
	buf[7] = pkg->mark;

	//copy buffer to payload
	msg pkg_to_send;
	memset (pkg_to_send.payload, 0, 1400);
	memcpy (&pkg_to_send.payload, buf, 8);
	pkg_to_send.len = 8;
	free(buf);

	//return package with payload and len to send
	return pkg_to_send;
}

char *header_name (char *name, unsigned char len) {
	char *file_name = (char *) calloc (50, 1);
	strcpy (file_name, "recv_");
	int i = 0;
	for (; i < len; i++) {
		file_name[i+5] = name[i];
	}
	file_name[i+5] = '\0';

	return file_name;
}

int main(int argc, char** argv) {
	msg *r, tr;
	unsigned short crc = 0, check = 0;
	char *file_name;
	int file, w;
	tr = ack_nak_pkg(-1, 'Y');
	int count = 0;
	int seq = -1;

	init(HOST, PORT);

	//wait for 15 seconds for send_init
	r = receive_message_timeout (3 * 5000);
	if (r == NULL)
		exit(1);
	else
		send_message (&tr);

	while (1) {
		//verify received
		r = receive_message_timeout (5000);
		if (r == NULL) {
			count++;
			printf ("[%s] Did not receive PKG with seq %d - TIMEOUT ERROR\n",
				argv[0], (int) tr.payload[2]+1);
			if (count < 3) {
				send_message(&tr);
				continue;
			}
			else
				exit(1);
		}
		
		//verify crc
		count = 0;
		crc = crc16_ccitt (r->payload, r->len - 3);
		check = get_crc (r->payload[r->len - 2], r->payload[r->len - 3]);
		if (crc != check) {
			printf ("[%s] Didn't receive pkg with seq %d\n", 
				argv[0], tr.payload[2]+1);
			tr = ack_nak_pkg (r->payload[2], 'N');
			send_message (&tr);
			continue;
		}
		else {
			//form ACK package
			printf ("[%s] Received pkg with seq %d\n",
				argv[0], r->payload[2]);
			tr = ack_nak_pkg (r->payload[2], 'Y');
			
			//if package has been successfully received before, drop it
			if (r->payload[2] == seq) {
				send_message(&tr);
				continue;
			}

			//if package is send_init
			if (r->payload[3] == 'S') {
				seq = r->payload[2];
				send_message (&tr);
				continue;
			}
			//if package is file_header open file to write
			else if (r->payload[3] == 'F') {
				file_name = header_name (&r->payload[4], r->len - 7);
				file = open (file_name, O_WRONLY | O_CREAT, 0644);
				if (file < 0)
					fatal ("Can't open file!");
			}
			//if package is data write to opened file
			else if (r->payload[3] == 'D') {
				size_t len = (size_t) r->len - 7;
 				w = write (file, &r->payload[4], len);
 				if (w < 0)
 					fatal ("Can't write to file!");
			}
			//if package is EOF, close the file
			else if (r->payload[3] == 'Z') {
				free (file_name);
				close(file);
			}
			//if package is EOT, end transmission
			else {
				seq = r->payload[2];
				send_message (&tr);
				break;
			}
			//update seq with the current package received seq
			seq = r->payload[2];
			//send response
			send_message (&tr);
		}
	}

	return 0;
}
