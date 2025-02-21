#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#define MAX_CHAR 256 // For Max possible number of ASCII values.

// Make Nodes to put in Huffman Tree.
typedef struct Node {
    unsigned char character;
    int freq;
    struct Node *left, *right;
} Node;

// Use priority-q to build tree. 
typedef struct MiniHeap {
    int size;
    Node *array[MAX_CHAR];
} MiniHeap;

int debug = 0;

// Read input file and count character freqeuencies. 
void count_frequencies(FILE *file, int freqs[]);

MiniHeap *create_mini_heap(int freqs[]);

// Build huffman tree from mini-heap.
Node *build_huffman_tree(MiniHeap *heap);

// Traverse tree and initiate huffman codes.
void ini_codes(Node *root, char *code, int depth, char *codes[]);

// Encode input file.
void encode_file(FILE *infile, FILE *outfile, char *codes[]);

// Free allocated memory.
void free_tree(Node *root);
void free_codes(char *codes[]);

// Insert node(s) into queue.
void insert_to_heap(MiniHeap *heap, Node *node);

// Remove and return  node with smallest frequency from the mini-heap.
Node *remove_mini(MiniHeap *heap);

// Maintain heap after removing a node. 
void mini_maintain(MiniHeap *heap, int index);

int main(int argc, char *argv[]) {
    int opt;
    char *inp_file = "completeShakespeare.txt";
    char *out_file = "huffman.out";
    FILE *infile, *outfile;

    // Go through command-line arguments for input, output and debug.
    while ((opt = getopt(argc, argv, "i:o:d")) != -1) {
        switch (opt) {
            case 'i':
                inp_file = optarg;
                break;
            case 'o':
                out_file = optarg;
                break;
            case 'd':
                debug = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-i inputfile] [-o outputfile] [-d]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Open input file.
    infile = fopen(inp_file, "rb");
    if (!infile) {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
    }

    // Counting frequencies in the file.
    int freqs[MAX_CHAR] = {0};
    count_frequencies(infile, freqs);
    rewind(infile); // Reset pointer to beginning.

    // Building the Huffman tree.
    MiniHeap *heap = create_mini_heap(freqs);
    Node *root = build_huffman_tree(heap);

    // Initiate Huffman codes for every character.
    char *codes[MAX_CHAR] = {NULL};
    char temp_code[MAX_CHAR];
    ini_codes(root, temp_code, 0, codes);

    // Open output file.
    outfile = fopen(out_file, "wb");
    if (!outfile) {
        perror("Error opening output file");
        fclose(infile);
        exit(EXIT_FAILURE);
    }

    
    encode_file(infile, outfile, codes);

    // Close files and free allocated memory. 
    fclose(infile);
    fclose(outfile);
    free_tree(root);
    free_codes(codes);

    return 0;
}

// Count the character frequencies in the open file.
void count_frequencies(FILE *file, int freqs[]) {
    int ch;
    while ((ch = fgetc(file)) != EOF) {
        freqs[ch]++;
    }
    if (debug) {
        printf("Character Frequencies:\n");
        for (int i = 0; i < MAX_CHAR; i++) {
            if (freqs[i] > 0) {
                printf("%c: %d\n", i, freqs[i]);
            }
        }
    }
}

// Construct the prio-q from character frequency array.
MiniHeap *create_mini_heap(int freqs[]) {
    MiniHeap *heap = malloc(sizeof(MiniHeap));
    heap->size = 0;
    for (int i = 0; i < MAX_CHAR; i++) {
        if (freqs[i] > 0) {
            Node *node = malloc(sizeof(Node));
            node->character = i;
            node->freq = freqs[i];
            node->left = node->right = NULL;
            insert_to_heap(heap, node);
        }
    }
    return heap;
}

// Generate Huffman tree from the mini-heap.
Node *build_huffman_tree(MiniHeap *heap) {
    while (heap->size > 1) {
        Node *left = remove_mini(heap);
        Node *right = remove_mini(heap);
        Node *new_node = malloc(sizeof(Node));
        new_node->character = '\0';
        new_node->freq = left->freq + right->freq;
        new_node->left = left;
        new_node->right = right;
        insert_to_heap(heap, new_node);
    }
    return remove_mini(heap);
}

// Initiate Huffman codes for characters in our tree. 
void ini_codes(Node *root, char *code, int depth, char *codes[]) {
    if (!root) return;
    if (!root->left && !root->right) {
        code[depth] = '\0';
        codes[root->character] = strdup(code);
        if (debug) printf("%c: %s\n", root->character, codes[root->character]);
    }
    code[depth] = '0';
    ini_codes(root->left, code, depth + 1, codes);
    code[depth] = '1';
    ini_codes(root->right, code, depth + 1, codes);
}

// Encode file
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


void insert_to_heap(MiniHeap *heap, Node *node) {
    int i = heap->size++;
    while (i > 0 && heap->array[(i - 1) / 2]->freq > node->freq) {
        heap->array[i] = heap->array[(i - 1) / 2];
        i = (i - 1) / 2;
    }
    heap->array[i] = node;
}

Node *remove_mini(MiniHeap *heap) {
    Node *min_value = heap->array[0];
    heap->array[0] = heap->array[--heap->size];
    mini_maintain(heap, 0);
    return min_value;
}

// Ensure heap property is still in effect. 
void mini_maintain(MiniHeap *heap, int index) {
    int smallest = index, left = 2 * index + 1, right = 2 * index + 2;
    if (left < heap->size && heap->array[left]->freq < heap->array[smallest]->freq)
        smallest = left;
    if (right < heap->size && heap->array[right]->freq < heap->array[smallest]->freq)
        smallest = right;
    if (smallest != index) {
        Node *temp = heap->array[index];
        heap->array[index] = heap->array[smallest];
        heap->array[smallest] = temp;
        mini_maintain(heap, smallest);
    }
}

// Freeing allocated memory functions for codes and tree. 
void free_tree(Node *root) {
    if (root) {
        free_tree(root->left);
        free_tree(root->right);
        free(root);
    }
}

void free_codes(char *codes[]) {
    for (int i = 0; i < MAX_CHAR; i++) {
        if (codes[i]) free(codes[i]);
    }
}
