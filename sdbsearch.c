#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define NR_OF_RECORDS_POSITION 48
#define START_OF_RECODS_POSITION 512
#define NUMBER_SIZE 30
#define NETWORK_SIZE 10
#define TYPE_SIZE 15
#define BUFFER_SIZE 1000

typedef struct {
    char number[NUMBER_SIZE];
    char network[NETWORK_SIZE];
    char type[TYPE_SIZE];
} record_disk_type;

typedef struct {
    char number[NUMBER_SIZE + 1];
    char network[NETWORK_SIZE + 1];
    char type[TYPE_SIZE + 1];
} record_type;

typedef struct node {
    struct node **nodes;
    record_type *record;
} node;

FILE *fp = NULL;
uint32_t nr_of_records;
record_type *records;
node *root;

void open_file();

void cleanup();

void exit_error(char *message);

void read_nr_of_records();

void init_tree();

node *new_node();

void add_to_tree(record_type *record);

void open_file() {
    if ((fp = fopen("AT_SDB_DATA_TBL", "rb")) == NULL) {
        exit_error("Could not open input file\n");
    }
}

void cleanup() {
    if (fp != NULL) {
        fclose(fp);
    }
    if (records != NULL) {
        free(records);
    }
}

void exit_error(char *message) {
    fprintf(stderr, message);
    cleanup();
    exit(1);
}

void read_nr_of_records() {
    fseek(fp, NR_OF_RECORDS_POSITION, SEEK_SET);
    fread(&nr_of_records, sizeof(nr_of_records), 1, fp);
    printf("Nr of records: %d\n", nr_of_records);
}

void copy_null_terminated(char *src, char *dest, uint32_t size) {
    char *end = src + size;
    while (*src != ' ' && src <= end) {
        *dest = *src;
        src++;
        dest++;
    }
    *dest = 0;
}

void print_record(record_type *record) {
    printf("%s,%s,%s\n", record->number, record->network, record->type);
}

void read_records() {
    fseek(fp, START_OF_RECODS_POSITION, SEEK_SET);
    record_disk_type buffer[BUFFER_SIZE];
    size_t records_read;
    size_t index = 0;

    if ((records = malloc(nr_of_records * sizeof(record_type))) == NULL) {
        exit_error("Could not allocate memory for records\n");
    }

    init_tree();

    while ((records_read = fread(buffer, sizeof(record_disk_type), BUFFER_SIZE, fp)) > 0) {
        for (int i = 0; i < records_read; i++) {
            copy_null_terminated(&(buffer[i].number), &(records[index].number), NUMBER_SIZE);
            copy_null_terminated(&(buffer[i].network), &(records[index].network), NETWORK_SIZE);
            copy_null_terminated(&(buffer[i].type), &(records[index].type), TYPE_SIZE);
            if (strlen(records[index].number) < 20 && records[index].number[0]=='4' && records[index].number[1]=='3') {
                add_to_tree(&records[index]);
                //print_record(&records[index]);
                index++;
            }
        }
    }
}

int cnt = 0;

node *new_node() {
    node *node = malloc(sizeof(node));
    node->record = NULL;
    node->nodes = calloc(10, sizeof(*node));
    cnt++;
    return node;
}

void init_tree() {
    if ((root = new_node()) == NULL) {
        exit_error("Could not allocate memory for tree\n");
    }
}

void add_to_tree(record_type *record) {
    char *number = record->number;
    node *current_node = root;
    while (*number != 0) {
        uint8_t slot = (*number) - '0';
        if (current_node->nodes[slot] == NULL) {
            current_node->nodes[slot] = new_node();
        }
        current_node = current_node->nodes[slot];
        number++;
    }
    current_node->record = record;
}

void find(char *search) {
    char *number = search;
    node *current_node = root;

    while (*number != 0) {
        uint8_t slot = (*number) - '0';
        if (current_node->nodes[slot] == NULL) {
            printf("number %s not found\n", search);
            return;
        }
        current_node = current_node->nodes[slot];
        number++;
    }
    if (current_node->record != NULL) {
        printf("number %s found: ", search);
        print_record(current_node->record);
    }
}

int main(int argc, char **argv) {
    open_file();
    read_nr_of_records();
    read_records();
    printf("Nodes: %d\n", cnt);

    find("431");
    find("4301");
    cleanup();
    return 0;
}