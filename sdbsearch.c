#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define NR_OF_RECORDS_POSITION 48
#define START_OF_RECORDS_POSITION 512
#define NUMBER_SIZE 30
#define NETWORK_SIZE 10
#define TYPE_SIZE 15
#define BUFFER_SIZE 100000
#define PRE_ALLOC_SIZE 1000000
#define EXPORT_OUTPUT_BUFFER_SIZE (4*1024)

typedef struct {
    char number[NUMBER_SIZE];
    char network[NETWORK_SIZE];
    char type[TYPE_SIZE];
} record_disk_t;

typedef struct {
    char number[NUMBER_SIZE + 1];
    char network[NETWORK_SIZE + 1];
    char type[TYPE_SIZE + 1];
} record_t;

typedef struct node_t {
    struct node_t *nodes[10];
    record_t *record;
} node_t;

typedef struct {
    node_t *nodes;
    size_t size;
    size_t index;
} pre_alloc_t;

FILE *fp = NULL;
uint32_t nr_of_records = 0;
uint32_t nr_of_records_read = 0;
uint32_t nr_of_nodes = 0;

record_t *records = NULL;
node_t *root = NULL;
pre_alloc_t pre_alloc = {NULL, 0, 0};
char export_output_buffer[EXPORT_OUTPUT_BUFFER_SIZE];

int verbose = 0;
int export = 0;

void open_file();

void exit_error(char *message);

void read_nr_of_records();

node_t *new_node();

void add_to_tree(record_t *record);

int parse_arguments(int argc, char **pString);

void print_help();

void print_statistics();

void set_export();

void open_file() {
    if ((fp = fopen("AT_SDB_DATA_TBL", "rb")) == NULL) {
        exit_error("Could not open input file\n");
    }
}

void exit_error(char *message) {
    fprintf(stderr, "Error: ");
    fprintf(stderr, message);
    exit(1);
}

void read_nr_of_records() {
    fseek(fp, NR_OF_RECORDS_POSITION, SEEK_SET);
    fread(&nr_of_records, sizeof(nr_of_records), 1, fp);
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

void print_record(record_t *record) {
    printf("%s,%s,%s\n", record->number, record->network, record->type);
}

void read_records() {
    fseek(fp, START_OF_RECORDS_POSITION, SEEK_SET);
    record_disk_t *buffer = malloc(sizeof(record_disk_t) * BUFFER_SIZE);
    size_t records_read;
    size_t index = 0;

    if ((records = malloc(nr_of_records * sizeof(record_t))) == NULL) {
        exit_error("Could not allocate memory for records\n");
    }

    root = new_node();

    if (verbose) {
        fprintf(stderr, "reading records");
        fflush(stderr);
    }
    while ((records_read = fread(buffer, sizeof(record_disk_t), BUFFER_SIZE, fp)) > 0) {
        for (int i = 0; i < records_read; i++) {
            copy_null_terminated((char *) &(buffer[i].number), (char *) &(records[index].number), NUMBER_SIZE);
            copy_null_terminated((char *) &(buffer[i].network), (char *) &(records[index].network), NETWORK_SIZE);
            copy_null_terminated((char *) &(buffer[i].type), (char *) &(records[index].type), TYPE_SIZE);
            if (strlen(records[index].number) < 20 && records[index].number[0] == '4' &&
                records[index].number[1] == '3') {
                add_to_tree(&records[index]);
                if (export) {
                    print_record(&records[index]);
                }
                index++;
            }
        }
        if (verbose) {
            fprintf(stderr, ".");
            fflush(stderr);
        }
    }
    if (verbose) {
        nr_of_records_read = index;
        fprintf(stderr, "\n");
    }
}

node_t *new_node() {
    if (pre_alloc.index == pre_alloc.size) {
        pre_alloc.index = 0;
        pre_alloc.size = PRE_ALLOC_SIZE;
        pre_alloc.nodes = calloc(PRE_ALLOC_SIZE, sizeof(node_t));
        if (pre_alloc.nodes == NULL) {
            exit_error("Could not allocate memory for nodes\n");
        }
    }
    nr_of_nodes++;
    return &pre_alloc.nodes[pre_alloc.index++];
}

void add_to_tree(record_t *record) {
    char *number = record->number;
    node_t *current_node = root;
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
    node_t *current_node = root;

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
    } else {
        printf("number %s not found\n", search);
    }
}

int parse_arguments(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == '-') {
            if (strcmp(argv[i], "--help") == 0) {
                print_help();
                exit(0);
            } else if (strcmp(argv[i], "--verbose") == 0) {
                verbose = 1;
            } else if (strcmp(argv[i], "--export") == 0) {
                set_export();
            }
        } else if (argv[i][0] == '-') {
            switch (argv[i][1]) {
                case 'h':
                case 'H':
                    print_help();
                    exit(0);
                case 'v':
                case 'V':
                    verbose = 1;
                    break;
                case 'e':
                case 'E':
                    set_export();
                    break;
            }
        } else {
            return i;
        }
    }
    if (!export) {
        print_help();
        exit_error("You have to give at least an option or a number to search");
        return 0;
    }
}

void set_export() {
    export = 1;
    setvbuf(stdout, export_output_buffer, _IOFBF, EXPORT_OUTPUT_BUFFER_SIZE);
}

void print_help() {
    printf("sdbsearch V0.1\n\n");
    printf("usage: sdbserch [OPTIONS] [NUMBER 1] [NUMBER 2] ... [NUMBER n]\n");
    printf("options:\n");
    printf("        -h, --help       print this help\n");
    printf("        -v, --verbose    verbose output\n");
    printf("        -e, --export     write all records in csv format to stdout\n");
    printf("\n");
}

void print_statistics() {
    if (verbose) {
        fprintf(stderr, "\n");
        fprintf(stderr, "%d records read\n", nr_of_records_read);
        fprintf(stderr, "%d records filtered\n", nr_of_records - nr_of_records_read);
        fprintf(stderr, "%d nodes created\n\n", nr_of_nodes);
        fflush(stderr);
    }
}

int main(int argc, char **argv) {
    int search = parse_arguments(argc, argv);
    open_file();
    read_nr_of_records();
    read_records();
    print_statistics();
    if (search > 0) {
        for (int s = search; s < argc; s++) {
            find(argv[s]);
        }
    }
    return 0;
}

