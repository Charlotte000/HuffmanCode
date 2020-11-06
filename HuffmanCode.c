#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct Node
{
	int freq;
	char symbol[256];
	struct Node* left;
	struct Node* right;
} Node;

void _countSymbols(char* string, int table[256], int* tableSize)
{
	// Count symbols
	*tableSize = 0;
	for (int i = 0; i < strlen(string); i++)
	{
		unsigned char charCode = (unsigned char)string[i];
		if (table[charCode] == 0)
		{
			(*tableSize)++;
		}
		table[charCode]++;
	}
}

Node** _createForest(int* table, int tableSize)
{
	// Create forest
	Node** forest = malloc(tableSize * sizeof(Node*));
	if (forest == NULL)
	{
		return NULL;
	}

	// Fill the forest
	for (int i = 0; i < 256; i++)
	{
		if (table[i] != 0)
		{
			Node* newNode = malloc(sizeof(Node));
			if (newNode == NULL)
			{
				return NULL;
			}
			newNode->freq = table[i];
			newNode->symbol[0] = i;
			newNode->symbol[1] = '\0';
			newNode->left = NULL;
			newNode->right = NULL;
			forest[--tableSize] = newNode;
		}
	}
	return forest;
}

int _findMin(Node** forest, int forestSize, int diff)
{
	int index = -1;
	for (int i = 0; i < forestSize; i++)
	{
		if (forest[i] != NULL && forest[i]->freq != -1 && i != diff)
		{
			if (index == -1 || forest[i]->freq < forest[index]->freq)
			{
				index = i;
			}
		}
	}
	return index;
}

Node* _buildTree(Node** forest, int forestSize)
{
	while (true)
	{
		// Get two minimal nodes
		int one = _findMin(forest, forestSize, -1);
		int two = _findMin(forest, forestSize, one);

		if (one == -1)
		{
			return forest[two];
		}
		else if (two == -1)
		{
			return forest[one];
		}

		// Create new node
		Node* newNode = malloc(sizeof(Node));
		if (newNode == NULL)
		{
			return NULL;
		}
		newNode->freq = forest[one]->freq + forest[two]->freq;
		newNode->symbol[0] = '\0';
		strcat(newNode->symbol, forest[one]->symbol);
		strcat(newNode->symbol, forest[two]->symbol);
		newNode->left = forest[one];
		newNode->right = forest[two];
		forest[one] = NULL;
		forest[two] = NULL;
		forest[one] = newNode;
	}
}

void _savePartTree(Node* tree, FILE* file)
{
	char output[100] = "";
	if (tree->left != NULL)
	{
		_savePartTree(tree->left, file);
	}
	if (tree->right != NULL)
	{
		_savePartTree(tree->right, file);
	}
	if (tree->left == NULL && tree->right == NULL)
	{
		fprintf(file, "%s%d\n", tree->symbol, tree->freq);
	}
}

void _saveTree(Node* tree, FILE* file, int stringLength)
{
	_savePartTree(tree, file);
	fprintf(file, "|-1\n");
	fprintf(file, "%d", stringLength);
}

Node* _loadTree(FILE* file)
{
	int table[256] = { 0 };
	int tableSize = 0;
	while (true)
	{
		char symbol;
		int freq;
		fscanf(file, "%c%d%*c", &symbol, &freq);
		if (feof(file) || (symbol == '|' && freq == -1))
		{
			break;
		}
		table[symbol] = freq;
		tableSize++;
	}
	
	Node** forest = _createForest(table, tableSize);
	Node* tree = _buildTree(forest, tableSize);
	return tree;
}

void encodeString(char* string, char* fileName)
{
	int tableSize = 0;
	int table[256] = { 0 };
	_countSymbols(string, table, &tableSize);

	Node** forest = _createForest(table, tableSize);
	Node* tree = _buildTree(forest, tableSize);

	FILE* file = fopen(fileName, "w+b");
	_saveTree(tree, file, strlen(string));
	int code = 0;
	int codeIndex = 0;
	for (int i = 0; i < strlen(string); i++)
	{
		Node* cursor = tree;

		while (true)
		{
			if (cursor->left != NULL && strchr(cursor->left->symbol, string[i]) != NULL)
			{
				code &= ~(1 << codeIndex);
				codeIndex++;
				cursor = cursor->left;

				if (codeIndex >= sizeof(int) * 8)
				{
					fwrite(&code, sizeof(int), 1, file);
					code = 0;
					codeIndex = 0;
				}
			}
			else if (cursor->right != NULL && strchr(cursor->right->symbol, string[i]) != NULL)
			{
				code |= 1 << codeIndex;
				codeIndex++;
				cursor = cursor->right;

				if (codeIndex >= sizeof(int) * 8)
				{
					fwrite(&code, sizeof(int), 1, file);
					code = 0;
					codeIndex = 0;
				}
			}
			else
			{
				break;
			}
		}
	}
	if (code != 0)
	{
		fwrite(&code, sizeof(int), 1, file);
	}
	fclose(file);

	for (int i = 0; i < tableSize; i++)
	{
		free(forest[i]);
	}
	free(forest);
}

void decodeString(char* fileName)
{
	int stringLength = 0;
	FILE* file = fopen(fileName, "rb");
	Node* tree = _loadTree(file);
	fscanf(file, "%d", &stringLength);
	Node* cursor = tree;
	int code;
	int codeIndex = 0;
	fread(&code, sizeof(int), 1, file);
	while (true)
	{
		if (!((code >> codeIndex) & 1) && cursor->left != NULL)
		{
			cursor = cursor->left;

			codeIndex++;

			if (codeIndex >= sizeof(int) * 8)
			{
				fread(&code, sizeof(int), 1, file);
				codeIndex = 0;
				if (feof(file)) break;
			}
		}
		else if (((code >> codeIndex) & 1) && cursor->right != NULL)
		{
			cursor = cursor->right;
			codeIndex++;

			if (codeIndex >= sizeof(int) * 8)
			{
				fread(&code, sizeof(int), 1, file);
				codeIndex = 0;
				if (feof(file)) break;
			}
		}
		else if (!((code >> codeIndex) & 1) && cursor->left == NULL)
		{
			printf("%s", cursor->symbol);
			stringLength--;
			cursor = tree;
		}
		else if (((code >> codeIndex) & 1) && cursor->right == NULL)
		{
			printf("%s", cursor->symbol);
			stringLength--;
			cursor = tree;
		}
		if (stringLength <= 0)
		{
			break;
		}
	}
	fclose(file);
	free(tree);
}