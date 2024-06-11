#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "heap.cpp"

typedef uint32_t u32;

void printCode(HeapNode * node)
{
    u32 code = node->code;
    for (int i = node->codeLength - 1; i >= 0; i--)
    {
        if (((code >> i) & 1) == 1)
        {
            fprintf(stdout, "1");
        }
        else
        {
            fprintf(stdout, "0");
        }
    }
}

void labelCodes(HeapNode * root, u32 depth, u32 code)
{
    if(root != NULL)
    {
        fprintf(stdout, "ch = %c freq = %d code = ", root->ch, root->freq);
        root->code = code;
        root->codeLength = depth + 1;
        printCode(root);
        fprintf(stdout, "\n");
        if(root->left)
        {
            labelCodes(root->left, depth + 1, (code << 1) | 0);
        }
        if(root->right)
        {
            labelCodes(root->right, depth + 1, (code << 1) | 1);
        }
    }
}

void verifyHeap(HeapNode * root)
{
    if (root != NULL)
    {
        
        if (root->left && root->right)
        {
            assert(root->freq == root->left->freq + root->right->freq);
            
        }
        else if (!root->left && !root->right)
        {
            fprintf(stdout, "ch = %c freq = %d code = ");
            printCode(root);
            fprintf(stdout, "\n");
        }
        else
        {
            assert(false); // should not have just 1 child;
        }
        verifyHeap(root->left);
        verifyHeap(root->right);
    }
}

int main(int argc, char ** args)
{
    if (argc > 1)
    {
        int decode = 0;
        char * filename;

        if (argc > 2)
        {
            if (strcmp(args[1], "-d") == 0)
            {
                decode = 1;
                filename = args[2];
            }
            else
            {
                fprintf(stderr, "Usage: huff [-d] <filename>\n");
                return -1;    
            }
        }
        else
        {
            filename = args[1];
        }
        
        FILE * fp = fopen(filename, "r");
        FILE * fp2 = fp;

        
        if (fp != NULL)
        {

            if (decode)
            {

            }
            else
            {
                int len = 4096;
                char * chs = (char *)calloc(len, sizeof(char));
                int * occ = (int *)calloc(len, sizeof(int));

                if (chs != NULL && occ != NULL)
                {
                    char ch;
                    while ((ch = fgetc(fp)) != EOF) {
                        for (int i = 0; i < len; ++i)
                        {
                            if (chs[i] == '\0')
                            {
                                chs[i] = ch;
                                occ[i] += 1;
                                break;
                            }
                            else if (chs[i] == ch)
                            {
                                occ[i] += 1;
                                break;
                            }
                        }
                    }

                    int size = 0;
                    char * t = chs;
                    while(*t++ != '\0')
                    {
                        size++;
                    }

                    // min heap used here as a priority queue for building the huffman tree
                    Heap * heap = (Heap *)malloc(sizeof(Heap));
                    heap->capacity = size;
                    heap->size = 0;
                    heap->array = (HeapNode **)malloc(heap->capacity * sizeof(HeapNode *));
                    //have array of leaves to use for later
                    HeapNode ** leaves = (HeapNode **)malloc(heap->capacity * sizeof(HeapNode *));
                    
                    for (int i = 0; i < size; i++)
                    {
                        HeapNode * node = (HeapNode *)malloc(sizeof(HeapNode));
                        node->ch = chs[i];
                        node->freq = occ[i];
                        leaves[i] = node;
                        insertHeap(heap, node);
                    }

                    // combining always reduces the size of the heap by 1. we exit when we have 1 node in the heap, the root of the huffman tree.
                    while(heap->size > 1)
                    {
                        HeapNode * first = extractMin(heap);
                        HeapNode * second = extractMin(heap);
                        HeapNode * combined = (HeapNode *)malloc(sizeof(HeapNode));
                        combined->freq = first->freq + second->freq;
                        combined->left = first;
                        combined->right = second;
                        insertHeap(heap, combined);
                    }

                    assert(heap->size == 1);
                    // verifyHeap(heap->array[0]);

                    HeapNode * huffmanRoot = heap->array[0];
                    labelCodes(huffmanRoot->left, 0, 0);
                    labelCodes(huffmanRoot->right, 0, 1);
                    
                    FILE * outputFile = fopen("output.bin", "wb");
                    for (int i = 0; i < size; i++)
                    {
                        // fprintf(stdout, "ch = %c freq = %d code = ", leaves[i]->ch, leaves[i]->freq);
                        // printCode(leaves[i]);
                        // fprintf(stdout, "\n");
                        fwrite(&leaves[i]->ch, sizeof(leaves[i]->ch), 1, outputFile);
                        fwrite(&leaves[i]->code, sizeof(leaves[i]->code), 1, outputFile);
                    }
                    
                    unsigned char buffer = 0;
                    int bits_in_buffer = 0;
                    while ((ch = fgetc(fp2)) != EOF) {
                        for (int i = 0; i < size; i++) // TODO: use a map if size is large
                        {
                            if (ch == leaves[i]->ch)
                            {
                                //pack code into bytes
                                for (int c = 0; c < leaves[i]->codeLength; c++)
                                {
                                    buffer = (buffer << 1) | ((leaves[i]->code >> c) & 1);
                                    bits_in_buffer++;
                                    if (bits_in_buffer == 8)
                                    {
                                        fwrite(&buffer, 1, 1, outputFile);
                                        buffer = 0;
                                        bits_in_buffer = 0;
                                    }
                                }
                                break; // once matched no need to check other leaves
                            }
                        }
                    }

                    if (bits_in_buffer > 0) {
                        buffer <<= (8 - bits_in_buffer); // Shift to left since will be reading bits from left to right
                        fwrite(&buffer, 1, 1, outputFile);
                        // Write the number of valid bits in the last byte
                        fwrite(&bits_in_buffer, 1, 1, outputFile);
                    }

                }
                else
                {
                    fprintf(stderr, "Memory allocation failed \n");    
                }
            }
                
        }
        else
        {
            fprintf(stderr, "Error reading file: %s\n", filename);
        }
    }
    else
    {
        fprintf(stderr, "Usage: huff [-d] <filename>\n");
    }

    return 0;
}