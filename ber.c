#include <stdio.h>

#define DUMP_WIDTH  16
#define DUMP_FORMAT "%02x "
#define DUMP_FWIDTH 3

#define DEBUG_FILE stdout

void dump(char *prefix, unsigned char *buf, unsigned int len) {
	int i;
	int c = 0;
	char s[DUMP_WIDTH+1] = "";

	fprintf(DEBUG_FILE, "%s @ %p, len %d\n", prefix, buf, len);
	for (i=0; i<len; i++) {
		fprintf(DEBUG_FILE, DUMP_FORMAT, buf[i]);
		if (buf[i] < 32 || buf[i] > 126) {
			s[c] = '.';
		} else {
			s[c] = buf[i];
		}
		c++;
		if (c >= DUMP_WIDTH) {
			s[c] = '\0';
			fprintf(DEBUG_FILE, ": %s\n", s);
			c = 0;
		}
	}
	if (c != 0) {
		s[c] = '\0';
		for (i=0; i<DUMP_FWIDTH*DUMP_WIDTH-DUMP_FWIDTH*c; i++) printf(" ");
		fprintf(DEBUG_FILE, ": %s\n", s);
	}
	fflush(DEBUG_FILE);
}

void minidump(char *prefix, unsigned char *buf, unsigned int len) {
	int i;
	printf("%s", prefix);
	for (i=0; i<len; i++) {
		printf("%02x ", buf[i]);
	}
	printf("\n");
}

#define ui8  unsigned char
#define ui32 unsigned long

#define CLASS_MASK        0xc0
#define CLASS_UNIVERSAL   0x00
#define CLASS_APPLICATION 0x40
#define CLASS_CONTEXT     0x80
#define CLASS_PRIVATE     0xc0

#define P_C_MASK          0x20

#define TYPE_MASK         0x1f
#define TYPE_BOOLEAN      0x01
#define TYPE_INTEGER      0x02
#define TYPE_OCTET_STRING 0x04
#define TYPE_NULL         0x05
#define TYPE_OID          0x06
#define TYPE_SEQUENCE     0x10

ui32 decode_type(ui8 *buf, ui32 alen, ui32 *ttype) {
	char *class = "";
	char *p_c   = "";
	char *type  = "";
	ui32 len = alen;
	//dump("decode_type:", buf, len);
	switch (*buf & CLASS_MASK) {
		case CLASS_UNIVERSAL:
			class = "universal";
			break;
		case CLASS_APPLICATION:
			class = "application";
			break;
		case CLASS_CONTEXT:
			class = "context";
			break;
		case CLASS_PRIVATE:
			class = "private";
			break;
		default:
			return 0;
	}
	printf("%s ", class);
	if (*buf & P_C_MASK) {
		p_c = "constructed";
	} else {
		p_c = "primitive";
	}
	printf("%s ", p_c);
	switch (*buf & TYPE_MASK) {
		case TYPE_SEQUENCE:
			type = "sequence\n\b";
			len--;
			break;
		case TYPE_BOOLEAN:
			type = "boolean";
			len--;
			break;
		case TYPE_INTEGER:
			type = "integer";
			len--;
			break;
		case TYPE_OCTET_STRING:
			type = "octet-string";
			len--;
			break;
		case TYPE_NULL:
			type = "null";
			len--;
			break;
		case TYPE_OID:
			type = "oid";
			len--;
			break;
		case TYPE_MASK: // for types that don't fit in 5-bits
			printf("BIG TYPE...\n");
			while (len) {
				buf++;
				len--;
				if (*buf & 0x80) {
					printf("type-extension: %02x\n", *buf & 0x80);
				} else {
					printf("type-extension: %02x\n", *buf & 0x80);
					break;
				}
			}
			break;
		default:
			if ((*buf & 0xe0) == 0xa0) {
				printf("pdu:%02x\n", *buf & 0xf);
				len--;
				if (ttype) *ttype = (ui32)TYPE_SEQUENCE;
				return alen-len;
			} else {
				type = "??? ";
				len--;
			}
			break;
	}
	if (ttype) *ttype = (ui32)*buf;
	printf("%s ", type);
	return alen-len;
}

ui32 decode_len(ui8 *buf, ui32 alen, ui32 *blen) {
	ui32 len = alen;
	ui32 rlen = 0;
	ui32 i;
	//dump("decode_len:", buf, alen);
	if (*buf & 0x80) {
		ui8 tlen = *buf & 0x7f;
		buf++;
		len--;
		for (i=0; i<tlen; i++) {
			printf("[%d] = %02x, rlen:%u\n", i, buf[i], rlen);
			rlen = (rlen << 8) + buf[i];
		}
		buf += tlen;
		len -= tlen;
		printf("rlen = %u\n", rlen);
	} else {
		rlen = *buf & 0x7f;
		//printf("len:%d\n", rlen);
		len--;
	}
	if (blen) *blen = rlen;
	return alen-len;
}

ui32 decode_value(ui8 *buf, ui32 len, ui32 blen, ui32 type) {
	int i;
	int acc = 0;
	ui8 n;
	//printf("decode_value(type:%x)::", type);
	switch (type) {
		case TYPE_OID:
			n = buf[0];
			if (n <= 39) {
				printf("itu-t.%d", n);
			} else if (n >= 40 || n <= 79) {
				//printf("iso.%d", n-40);
				printf(".1.%d", n-40);
			} else if (n >= 80) {
				printf("joint-iso-itu-t.%d", n-80);
			}
			for (i=1; i<blen; i++) {
				n = buf[i];
				if (n & 0x80) {
					acc = (n & 0x7f) << 8;
				} else {
					printf(".%d", acc+n);
				}
			}
			printf("\n");
			break;
		case TYPE_INTEGER:
			acc = 0;
			for (i=0; i<blen; i++) {
				n = buf[i];
				acc = (acc << 8) + n;
			}
			printf("%d\n", acc);
			break;
		default:
			minidump("", buf, blen);
			break;
	}
	return blen;
}

ui8 trap2c[] = {
0x30, 0x47, 0x02, 0x01, 0x01, 0x04, 0x06, 0x70, 0x75, 0x62, 0x6c, 0x69, 0x63, 0xa7, 0x3a, 0x02, // 0G.....public.:.
0x04, 0x13, 0x1c, 0x33, 0x6b, 0x02, 0x01, 0x00, 0x02, 0x01, 0x00, 0x30, 0x2c, 0x30, 0x16, 0x06, // ...3k......0,0..
0x0a, 0x2b, 0x06, 0x01, 0x06, 0x03, 0x01, 0x01, 0x04, 0x01, 0x00, 0x06, 0x08, 0x2b, 0x06, 0x01, // .+...........+..
0x02, 0x01, 0x45, 0x02, 0x01, 0x30, 0x12, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x02, 0x01, 0x45, 0x02, // ..E..0...+....E.
0x01, 0x04, 0x06, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,                                           // ...."3DUf
};

ui8 groups[] = {
0x30, 0x84, 0x00, 0x00, 0x00, 0x10, 0x02, 0x01, 0x01, 0x60, 0x84, 0x00, 0x00, 0x00, 0x07, 0x02,
0x01, 0x03, 0x04, 0x00, 0x80, 0x00,
};

ui32 parse(ui8 *ptr, ui32 len, ui32 level) {
	int i;
	ui32 used;
	ui32 vlen;
	ui32 type;

	used = 0;
	vlen = 0;
	type = 0;

	if (level == 0) dump("parse:", ptr, len);

	while (1) {
		type = 0;
		used = decode_type(ptr, len, &type);
		if (used) {
			ptr += used;
			len -= used;
		} else {
			break;
		}
		used = decode_len(ptr, len, &vlen);
		if (used) {
			ptr += used;
			len -= used;
		} else {
			break;
		}
		if (type & TYPE_SEQUENCE) {
			used = parse(ptr, vlen, level+1);
			if (used) {
				ptr += used;
				len -= len;
			} else {
				break;
			}
		} else {
			//printf("ptr:%p len:%d\n", ptr, len);
			used = decode_value(ptr, len, vlen, type);
			if (used) {
				ptr += used;
				len -= used;
			} else {
				break;
			}
		}
		if (len == 0) break;
	}
	//printf("<<<parse used:%d\n", used);
	return used;
}

#define WHICH trap2c
//#define WHICH groups

#if 0

int main(int argc, char *argv[]) {
	parse(WHICH, sizeof(WHICH), 0);
	return 0;
}

#else

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define READ(var,fmt) n = read(f, &var, sizeof(var)); if (n < 0) { perror("read()"); return 0; } printf(fmt, var)

#define NREAD(buffer, len) n = read(f, buffer, len); if (n < 0) { perror("read()"); return 0; }

#define SKIP(dist) n = lseek(f, dist, SEEK_CUR); if (n < 0) { perror("lseek()"); return 0; }

int main(int argc, char *argv[]) {
	char *file = "snmpout";
	int f = -1;
	int n;
	unsigned char c;
	unsigned short s;
	unsigned long l;
	unsigned long alen;
	unsigned long ulen;
	ui8 buf[2048];

	if (argc > 1) file = argv[1];
	f = open(file, O_RDONLY);
	if (f < 0) {
		perror("open()");
		return 0;
	}
	READ(l, "magic:%x");
	READ(s, " major:%d");
	READ(s, " minor:%d");
	READ(l, " tzone:%d");
	READ(l, " time:%d");
	READ(l, " slen:%d");
	READ(l, " ltype:%d\n");

	while (1) {
		READ(l, "ntime:%d");
		READ(l, " usec:%d");
		READ(l, " flen:%d");
		READ(alen, " alen:%d\n");

		// skip unknown tcpdump bytes
		SKIP(14);

		READ(c, "ver/ihl:%x");
		
		READ(c, " tos:%x");
		READ(s, " ip_len:%d");

		READ(s, " id:%d");
		READ(s, " frag:%d");

		READ(c, " ttl:%d");
		READ(c, " proto:%d");
		READ(s, " cksum:%x");

		READ(l, " src:%x");
		READ(l, " dst:%x\n");

		READ(s, "src-port:%d");
		READ(s, " dst-port:%d");
		READ(ulen, " ulen:%d");
		READ(s, " cksum:%x\n");

		n = read(f, &buf, ulen-8);
		if (n < 0) {
			perror("read()");
			return 0;
		}
		dump("buf:", buf, ulen-8);
	}
	return 0;
}

#endif
