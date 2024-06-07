#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

typedef struct Node_ {
    char ch;
    int weight;
    struct Node_ * left;
    struct Node_ * right;
} Node;

void printTree(Node_ * root, int depth)
{
    if (root != NULL)
    {
        if ((*root).left != NULL)
        {
            printTree((*root).left, depth + 1);
        }
        for (int i = 0; i < depth; i++)
        {
            fprintf(stdout, " ");
        }
        fprintf(stdout, "Depth: %d, Node: ch = %c, weight = %d\n", depth, (*root).ch, (*root).weight);
        if ((*root).right != NULL)
        {
            printTree((*root).right, depth + 1);
        }
    }
}

int main(int argc, char ** args)
{
    if (argc > 1)
    {
        char * filename = args[1];
        FILE * fp = fopen(filename, "r");
        
        if (fp != NULL)
        {
            int len = 4096;
            char * chs = (char *)calloc(len, sizeof(char));
            int * occ = (int *)calloc(len, sizeof(int));

            if (chs != NULL && occ != NULL)
            {
                char ch;
                while ((ch = fgetc(fp)) != EOF) {
                    // count += 1;
                    // fprintf(stdout, "ch: %c\n", ch);
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

                fprintf(stdout, "Total number of characters: %d\n", size);

                fprintf(stdout, "Occurences: \n");
                for (int i = 0; i < len; ++i)
                {
                    if (chs[i] == '\0') break;
                    // if (chs[i] == 't') 
                    // {
                    //     fprintf(stdout, "%c: %d\n", chs[i], occ[i]);
                    //     assert(occ[i] == 223000); // assertions for les-miserable.txt
                    // }
                    // if (chs[i] == 'X')
                    // {
                    //     fprintf(stdout, "%c: %d", chs[i], occ[i]);
                    //      assert(occ[i] == 333); // assertions for les-miserable.txt
                    // }
                    fprintf(stdout, "%c: %d\n", chs[i], occ[i]);
                }

                // build huffman tree
                // TODO: use priority queue
                Node_ * arr = (Node_ *)calloc(size * 2 - 1, sizeof(Node_)); // there is n - 1 combined nodes, so n + n - 1 = 2n - 1
                for (int i = 0; i < size; i++)
                {
                    arr[i] = Node_ { ch: chs[i], weight: occ[i] };
                }


                // start and end of range of nodes we are looking at
                int start = 0;
                int end = size - 1;
                for(int s = 0; s < size - 1; s ++) // we run the node combining n - 1 times, see visual of tree to confirm
                {
                    fprintf(stdout, "start = %d, end = %d, size =%d\n", start, end, size);
                    // sort the range of nodes we are looking at
                    for (int i = start; i < end; i++)
                    {
                        for (int j = i + 1; j <= end; j++)
                        {
                            if (arr[i].weight > arr[j].weight)
                            {
                                Node_ temp = arr[i];
                                arr[i] = arr[j];
                                arr[j] = temp;
                            }
                        }
                    }

                    // pick first (lowest) two and combine                    
                    Node_ * combined = (Node_ *)malloc(sizeof(Node_));
                    combined->weight = arr[start].weight + arr[start + 1].weight;
                    combined->left = &arr[start];
                    combined->right = &arr[start + 1];
                    start += 2;
                    end += 1;
                    arr[end] = *combined; // put combined at the end of the list
                    // so for next iteration, we dont look at beginning two (left and right) here and look "ahead" to where new combined node was added

                    for (int i = 0; i <= end; i++)
                    {
                        fprintf(stdout, "%d: ch = %c weight = %d\n", i, arr[i].ch, arr[i].weight);
                    }
                    fprintf(stdout, "-----------\n");
                }
                Node root = arr[end];
                printTree(&root, 0);
            }
            else
            {
                fprintf(stderr, "Memory allocation failed \n");    
            }
        }
        else
        {
            fprintf(stderr, "Error reading file: %s\n", filename);
        }
    }
    else
    {
        fprintf(stderr, "Usage: huff <filename>\n");
    }

    return 0;
}