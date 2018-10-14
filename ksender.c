#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000
#define DATA_SIZE_S 11
#define DATA_SIZE 250

//building a new package to send
msg pkg (unsigned char len, char type, char *data, unsigned char seq) {
	mini_kermit *pkg = (mini_kermit *) malloc (sizeof(mini_kermit));
	// fill first fields
	pkg->soh = 0x01;
	pkg->type = type;
	pkg->seq = seq;
	pkg->len = len;
	pkg->data = data;

	//copy in buffer to calculate crc on the fields before check
	unsigned short crc = 0;
	size_t length;
	char *buf = (char *) malloc ((unsigned char) pkg->len + 2);
	memcpy (buf, pkg, 4);
	if (pkg->data) {
		length = ((unsigned char) pkg->len) - 5;
		memcpy (buf + 4, pkg->data, length);
	}
	crc = crc16_ccitt (buf, (unsigned char) pkg->len - 1);

	//fill the rest of the fields
	pkg->check = crc;
	pkg->mark = 0x0D;

	//put check and mark fields in the buffer
	buf[(unsigned char) pkg->len - 1] = pkg->check & 0xFF;
	buf[(unsigned char) pkg->len] = (pkg->check >> 8) & 0xFF;
	buf[(unsigned char) pkg->len + 1] = pkg->mark;

	//form message
	msg pkg_to_send;
	//copy buffer into payload
	memset (pkg_to_send.payload, 0, 1400);
	memcpy (&(pkg_to_send.payload), buf, (unsigned char) pkg->len + 2);
	pkg_to_send.len = (unsigned char) pkg->len + 2;

	//return package with payload and len to send
	return pkg_to_send;
}

//sending packages function
void send (msg pkg, char *name) {
	msg *t;
	msg send = pkg;
	int count = 0;
	while (1) {
		//send 
		send_message(&send);

		//verify sending
		t = receive_message_timeout(5000);
		if (t == NULL) {
			printf ("[%s] Did not receive ACK/NAK for seq %d - TIMEOUT ERROR\n",
					 name, pkg.payload[2]);
			count++;
			if (count < 3) {
				send = pkg;
				continue;
			}
			else
				exit(1);
		}
		//verify if crc != check
		else if (get_crc(t->payload[t->len - 2], t->payload[t->len - 3]) !=
					crc16_ccitt (t->payload, t->len - 3)) {
			send = pkg;
			continue;
		}
		//verify if different seq
		else if ((unsigned char) t->payload[2] != 
					(unsigned char) pkg.payload[2]) {
			count = 0;
			send = pkg;
		}
		//verify if NAK
		else if (t->payload[3] == 'N') {
			printf ("[%s] Sending again pkg with seq %d...\n",
				name, pkg.payload[2]);
			count = 0;
			send = pkg;
		}
		//verify if EOT
		else if (pkg.payload[3] == 'B') {
			printf ("END OF TRANSMISION, WELL DONE!\n");
			break;
		}
		//verify if ACK
		else {
			count = 0;
			printf ("[%s] Got very well ACK for seq %d.\n",
				name, t->payload[2]);
			break;
		}
	}
}

int main(int argc, char** argv) {

	int seq = 0;
	char *data = NULL;
	msg to_send;

	init(HOST, PORT);
	//send_init data
	data = (char *) malloc (DATA_SIZE_S);					//DATA
	data[0] = 250;											//MAXL
	data[1] = 5;											//TIME
	data[2] = 0x00;											//NPAD
	data[3] = 0x00;											//PADC
	data[4] = 0x0D;											//EOL
	//QCTL, QBIN, CHKT, REPT, CAPA, R
	int i = 5;
	for (; i < DATA_SIZE_S; i++)
		data[i] = 0x00;
	msg send_init = pkg (16, 'S', data, 0);

	//send send_init pkg
	send(send_init, argv[0]);

	free(data);
	data = NULL;
	int file, f;

	int nr_files = 0;
	while (nr_files < argc - 1) {
		seq = (seq + 1) % 64;
		
		//send file_header
		data = argv[nr_files+1];
		to_send = pkg (5 + strlen(argv[nr_files+1]), 'F',
			data, (unsigned char) seq);
		send (to_send, argv[0]);

		seq = (seq + 1) % 64;

		//send data of the file
		file = open (argv[nr_files+1], O_RDONLY);
		if (file < 0)
			fatal ("Can't open file!");
		char datas[250];
		f = read (file, datas, DATA_SIZE);
		if (f < 0)
			fatal ("Can't read from file!");
		//read while not at EOF
		while (f > 0) {
			to_send = pkg (5 + f, 'D', datas, (unsigned char) seq);
			send (to_send, argv[0]);
			memset(datas, 0, sizeof(f));
			seq = (seq + 1) % 64;
			f = read (file, datas, DATA_SIZE);
		}

		//send end of file
		data = NULL;
		to_send = pkg (6, 'Z', NULL, (unsigned char) seq);
		send (to_send, argv[0]);

		nr_files++;
	}

	seq = (seq + 1) % 64;
	//end of transmission
	to_send = pkg (6, 'B', NULL, (unsigned char) seq);
	send (to_send, argv[0]);

	return 0;
}
