#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#define MAX_CHAR 256

// Make Node structures
typedef struct Node {
    unsigned char character;
    int freq;
    struct Node *left, *right;
} Node;

// Make a min-heap for prio queue
typedef struct MiniHeap {
    int size;
    Node *array[MAX_CHAR];
} MiniHeap;

int debug = 0;

// Huffman encoding stages
void count_frequencies(FILE *file, int freqs[]);
MiniHeap *build_min_heap(int freqs[]);
Node *build_huffman_tree(MiniHeap *heap);
void generate_codes(Node *root, char *code, int depth, char *codes[]);
void encode_file(FILE *infile, FILE *outfile, char *codes[]);

// Helper functions for the min-heap
void insert_heap(MiniHeap *heap, Node *node);
Node *remove_min(MiniHeap *heap);
void swap_nodes(Node **a, Node **b);
void min_heapify(MiniHeap *heap, int index);


int main(int argc, char *argv[]) {
    int opt;
    char *input_filename = NULL, *output_filename = "output.huff";
    FILE *infile, *outfile;
    
    while ((opt = getopt(argc, argv, "d")) != -1) {
        if (opt == 'd') debug = 1;
    }
    
    if (optind >= argc) {
        fprintf(stderr, "Usage: %s [-d] inputfile\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    input_filename = argv[optind];
    infile = fopen(input_filename, "rb");
    if (!infile) {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
    }
    
    int freqs[MAX_CHAR] = {0};
    count_frequencies(infile, freqs);
    rewind(infile);
    
    MiniHeap *heap = build_min_heap(freqs);
    Node *root = build_huffman_tree(heap);
    
    char *codes[MAX_CHAR] = {NULL};
    char temp_code[MAX_CHAR];
    generate_codes(root, temp_code, 0, codes);
    
    outfile = fopen(output_filename, "wb");
    if (!outfile) {
        perror("Error opening output file");
        exit(EXIT_FAILURE);
    }
    
    encode_file(infile, outfile, codes);
    fclose(infile);
    fclose(outfile);
    
    printf("Encoding complete. Output written to %s\n", output_filename);
    return 0;
}

void count_frequencies(FILE *file, int freqs[]) {
    int ch;
    while ((ch = fgetc(file)) != EOF) {
        freqs[ch]++;
    }
    if (debug) {
        printf("Character freqs:\n");
        for (int i = 0; i < MAX_CHAR; i++) {
            if (freqs[i] > 0) {
                printf("%c: %d\n", i, freqs[i]);
            }
        }
    }
}

MiniHeap *build_min_heap(int freqs[]) {
    MiniHeap *heap = malloc(sizeof(MiniHeap));
    heap->size = 0;
    for (int i = 0; i < MAX_CHAR; i++) {
        if (freqs[i] > 0) {
            Node *node = malloc(sizeof(Node));
            node->character = i;
            node->freq = freqs[i];
            node->left = node->right = NULL;
            insert_heap(heap, node);
        }
    }
    return heap;
}

Node *build_huffman_tree(MiniHeap *heap) {
    while (heap->size > 1) {
        Node *left = remove_min(heap);
        Node *right = remove_min(heap);
        Node *new_node = malloc(sizeof(Node));
        new_node->character = '\0';
        new_node->freq = left->freq + right->freq;
        new_node->left = left;
        new_node->right = right;
        insert_heap(heap, new_node);
    }
    return remove_min(heap);
}

void generate_codes(Node *root, char *code, int depth, char *codes[]) {
    if (!root) return;
    if (!root->left && !root->right) {
        code[depth] = '\0';
        codes[root->character] = strdup(code);
        if (debug) printf("%c: %s\n", root->character, codes[root->character]);
    }
    code[depth] = '0';
    generate_codes(root->left, code, depth + 1, codes);
    code[depth] = '1';
    generate_codes(root->right, code, depth + 1, codes);
}

void encode_file(FILE *infile, FILE *outfile, char *codes[]) {
    rewind(infile);
    unsigned char buffer = 0;
    int bit_count = 0;
    int ch;
    while ((ch = fgetc(infile)) != EOF) {
        char *code = codes[ch];
        for (int i = 0; code[i] != '\0'; i++) {
            buffer = (buffer << 1) | (code[i] - '0');
            bit_count++;
            if (bit_count == 8) {
                fwrite(&buffer, 1, 1, outfile);
                bit_count = 0;
                buffer = 0;
            }
        }
    }
    if (bit_count > 0) {
        buffer <<= (8 - bit_count);
        fwrite(&buffer, 1, 1, outfile);
    }
}
