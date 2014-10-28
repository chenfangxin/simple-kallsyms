#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void output_label(char *label)
{
	printf(".globl %s\n", label);
	printf("\tALIGN\n");
	printf("%s:\n", label);
}

struct sym_entry{
	unsigned long long addr;
	unsigned int len;
	unsigned char *sym;
};

static int table_size=0, table_cnt=0;
struct sym_entry *table=NULL;

static int read_symbol(FILE *in, struct sym_entry *s)
{
	char str[256];
	char *sym;
	int rc;
	unsigned long long addr;
	char stype;

	memset(str, 0, 256*sizeof(char));
	rc = fscanf(in, "%llx %c %255s\n", &addr, &stype, str);
	if(rc != 3){
		if(rc != EOF){
			fgets(str, 256, in);
		}
		return -1;
	}

	if(toupper(stype) != 'T'){
		return -1;
	}

	sym = str;
	s->addr = addr;	
	s->len = strlen(str);
	s->sym = malloc(s->len+1);
	if(NULL==s->sym){
		printf("failed to malloc for symbol.\n");
		exit(1);
	}
	memset(s->sym, 0, s->len+1);
	strcpy((char *)s->sym, str);

	return 0;
}

static void read_map(FILE *in)
{
	while(!feof(in)){
		if(table_cnt >= table_size){
			table_size += 10000;
			table = realloc(table, sizeof(*table)*table_size);
			if(NULL==table){
				printf("out of memory.\n");
				exit(1);
			}
		}
		if(read_symbol(in, &table[table_cnt])==0){
			table_cnt++;
		}
	}
	return;
}

static void write_src(FILE *out)
{
	int i=0,j=0,len=0;

	fprintf(out, "#define PTR .quad\n");	
	fprintf(out, "#define ALGN .align 8\n");	

	fprintf(out, "\n.data\n");
	fprintf(out, ".globl linux_syms_addresses\n");
	fprintf(out, "\tALGN\n");
	fprintf(out, "linux_syms_addresses:\n");
	for(i=0;i<table_cnt;i++){
		fprintf(out, "\tPTR 0x%llx\n", table[i].addr);
	}

	fprintf(out, "\n.globl linux_syms_num\n");
	fprintf(out, "\tALGN\n");
	fprintf(out, "linux_syms_num:\n");
	fprintf(out, "\tPTR %d\n", table_cnt);

	fprintf(out, "\n.globl linux_syms_names\n");
	fprintf(out, "\tALGN\n");
	fprintf(out, "linux_syms_names:\n");
	for(i=0;i<table_cnt;i++){
		len = table[i].len;
		fprintf(out, ".byte 0x%x,", len+1);
		for(j=0;j<len;j++){
			fprintf(out, "0x%x,", table[i].sym[j]);
		}
		fprintf(out, "0x00\n");
	}
	
	return;
}

int main(int argc, char *argv[])
{
	FILE *in=NULL, *out=NULL;
	if(argc<2){
		printf("Usage: %s input_file output_file\n", argv[0]);
		return 0;
	}
	in = fopen(argv[1], "r");
	if(NULL==in){
		printf("Failed to open file %s.\n", argv[1]);
		return -1;	
	}
	out = fopen(argv[2], "w");
	if(NULL==out){
		printf("Failed to open file %s\n", argv[2]);
		fclose(in);
		return -1;
	}
	read_map(in);
	write_src(out);

	fclose(in);
	fclose(out);
	free(table);
}
