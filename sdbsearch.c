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

uint8_t verbose = 0;
uint8_t export = 0;
uint8_t find_all = 0;
uint8_t filter = 1;

int parse_arguments(int argc, char **argv);

void parse_long_argument(char *arg);

void parse_short_argument(char *arg);

node_t *new_node();

void add_to_tree(record_t *record);

void argument_error(char *argv);

void copy_null_terminated(char *src, char *dest, uint32_t size);

void exit_error(char *message);

void find(char *search);

void open_file();

void print_help(FILE *fp);

void print_statistics();

void read_nr_of_records();

void read_records();

void set_export();


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

void print_statistics() {
    if (verbose && !export) {
        fprintf(stderr, "\n");
        fprintf(stderr, "%d records read\n", nr_of_records_read);
        fprintf(stderr, "%d records filtered\n", nr_of_records - nr_of_records_read);
        fprintf(stderr, "%d nodes created\n\n", nr_of_nodes);
        fflush(stderr);
    }
}

void print_help(FILE *fp) {
    fprintf(fp, "sdbsearch V0.1\n\n");
    fprintf(fp, "usage: sdbserch [OPTIONS] [NUMBER 1] [NUMBER 2] ... [NUMBER n]\n");
    fprintf(fp, "options:\n");
    fprintf(fp, "        -h, --help       print this help\n");
    fprintf(fp, "        -a, --all        show all matching numbers\n");
    fprintf(fp, "        -n, --nofilter   do not filter 43 and length 20\n");
    fprintf(fp, "        -e, --export     write all records in csv format to stdout\n");
    fprintf(fp, "        -v, --verbose    verbose output\n");
    fprintf(fp, "\n");
}

int parse_arguments(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == '-') {
            parse_long_argument(argv[i]);
        } else if (argv[i][0] == '-') {
            parse_short_argument(argv[i]);
        } else {
            return i;
        }
    }
    if (!export) {
        print_help(stdout);
        exit_error("You have to give at least an option or a number to search");
        return 0;
    }
}

void parse_short_argument(char *arg) {
    if (arg[2] != 0) {
        argument_error(arg);
    }
    switch (arg[1]) {
        case 'h':
        case 'H':
            print_help(stdout);
            exit(0);
        case 'v':
        case 'V':
            verbose = 1;
            break;
        case 'e':
        case 'E':
            set_export();
            break;
        case 'a':
        case 'A':
            find_all = 1;
            break;
        case 'n':
        case 'N':
            filter = 0;
            break;
        default:
            argument_error(arg);
    }
}

void parse_long_argument(char *arg) {
    if (strcmp(arg, "--help") == 0) {
        print_help(stdout);
        exit(0);
    } else if (strcmp(arg, "--verbose") == 0) {
        verbose = 1;
    } else if (strcmp(arg, "--export") == 0) {
        set_export();
    } else if (strcmp(arg, "--all") == 0) {
        find_all = 1;
    } else if (strcmp(arg, "--nofilter") == 0) {
        filter = 0;
    } else {
        argument_error(arg);
    }
}

void argument_error(char *argv) {
    print_help(stderr);
    fprintf(stderr, "Error: unknown argument %s\n", argv);
    exit(1);
}

void set_export() {
    export = 1;
    setvbuf(stdout, export_output_buffer, _IOFBF, EXPORT_OUTPUT_BUFFER_SIZE);
}

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


void print_record(record_t *record) {
    printf("%s,%s,%s\n", record->number, record->network, record->type);
}

void read_records() {
    fseek(fp, START_OF_RECORDS_POSITION, SEEK_SET);
    record_disk_t *buffer = malloc(sizeof(record_disk_t) * BUFFER_SIZE);
    size_t records_read;
    size_t index = 0;
    uint32_t records_to_allocate = export ? 1 : nr_of_records;

    if ((records = malloc(records_to_allocate * sizeof(record_t))) == NULL) {
        exit_error("Could not allocate memory for records\n");
    }

    root = new_node();

    if (verbose) {
        fprintf(stderr, "reading records");
        fflush(stderr);
    }

    if (export) {
        printf("NUMBER,NETWORK,TYPE\n");
    }

    while ((records_read = fread(buffer, sizeof(record_disk_t), BUFFER_SIZE, fp)) > 0) {
        for (int i = 0; i < records_read; i++) {
            copy_null_terminated((char *) &(buffer[i].number), (char *) &(records[index].number), NUMBER_SIZE);
            copy_null_terminated((char *) &(buffer[i].network), (char *) &(records[index].network), NETWORK_SIZE);
            copy_null_terminated((char *) &(buffer[i].type), (char *) &(records[index].type), TYPE_SIZE);
            if (!filter || (strlen(records[index].number) < 20 && records[index].number[0] == '4' && records[index].number[1] == '3')) {
                if (export) {
                    print_record(&records[index]);
                } else {
                    add_to_tree(&records[index]);
                    index++;
                }
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

void copy_null_terminated(char *src, char *dest, uint32_t size) {
    char *end = src + size;
    while (*src != ' ' && src <= end) {
        *dest = *src;
        src++;
        dest++;
    }
    *dest = 0;
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
        if (find_all && current_node->record != NULL) {
            printf("found: ", current_node->record->number);
            print_record(current_node->record);
        }
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
