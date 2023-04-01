#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define NR_OF_RECORDS_POSITION 48
#define START_OF_RECODS_POSITION 512
#define NUMBER_SIZE 30
#define NETWORK_SIZE 10
#define TYPE_SIZE 15
#define BUFFER_SIZE 1000

typedef struct
{
    char number[NUMBER_SIZE];
    char network[NETWORK_SIZE];
    char type[TYPE_SIZE];
} record_disk_type;

typedef struct
{
    char number[NUMBER_SIZE + 1];
    char network[NETWORK_SIZE + 1];
    char type[TYPE_SIZE + 1];
} record_type;

FILE *fp = NULL;
uint32_t nr_of_records;
record_type *records;

void open_file();
void cleanup();
void exit_error(char *message);
void read_nr_of_records();

void open_file()
{
    if ((fp = fopen("AT_SDB_DATA_TBL", "rb")) == NULL)
    {
        exit_error("Could not open input file\n");
    }
}

void cleanup()
{
    if (fp != NULL)
    {
        fclose(fp);
    }
    if (records!=NULL){
        free(records);
    }
}

void exit_error(char *message)
{
    fprintf(stderr, message);
    cleanup();
    exit(1);
}

void read_nr_of_records()
{
    fseek(fp, NR_OF_RECORDS_POSITION, SEEK_SET);
    fread(&nr_of_records, sizeof(nr_of_records), 1, fp);
    printf("Nr of records: %d\n", nr_of_records);
}

void copy_null_terminated(char *src, char *dest, uint32_t size)
{
    char *end = src + size;
    while (*src != ' ' && src <= end)
    {
        *dest = *src;
        src++;
        dest++;
    }
    *dest = 0;
}

void print_record(record_type *record)
{
    printf("%s,%s,%s\n", record->number, record->network, record->type);
}

void read_records()
{
    fseek(fp, START_OF_RECODS_POSITION, SEEK_SET);
    record_disk_type buffer[BUFFER_SIZE];
    
    size_t records_read;
    size_t index = 0;

    if ((records = malloc(nr_of_records * sizeof(record_type))) == NULL)
    {
        exit_error("Could not allocate memory for records\n");
    }

    while ((records_read = fread(buffer, sizeof(record_disk_type), BUFFER_SIZE, fp)) > 0)
    {
        for (int i = 0; i < records_read; i++)
        {
            copy_null_terminated(&(buffer[i].number), &(records[index].number), NUMBER_SIZE);
            copy_null_terminated(&(buffer[i].network), &(records[index].network), NETWORK_SIZE);
            copy_null_terminated(&(buffer[i].type), &(records[index].type), TYPE_SIZE);
            index++;
        }
    }
}

int main(int argc, char **argv)
{
    open_file();
    read_nr_of_records();
    read_records();
    cleanup();
    return 0;
}