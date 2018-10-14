#ifndef LIB
#define LIB

typedef struct {
	int len;
	char payload[1400];
} msg;

typedef struct __attribute__((__packed__)) mini_kermit_pkg {
	char soh;
	char len;
	char seq;
	char type;
	char *data;
	unsigned short check;
	char mark;
} mini_kermit;

void fatal (char *mesaj_eroare) {
	perror (mesaj_eroare);
	exit(1);
}

unsigned short get_crc (unsigned char a, unsigned char b) {
	unsigned short us = ((unsigned char) a << 8) | (unsigned char) b;
	return us;
}

void init(char* remote, int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);
msg* receive_message_timeout(int timeout); //timeout in milliseconds
unsigned short crc16_ccitt(const void *buf, int len);

#endif

