#include <stdlib.h>
#include <stdio.h>

#include "tree.h"

node_t *tree_create_node(tree_t *t, void *data)
{
	node_t *n;

	n = (node_t *) malloc(sizeof(node_t));

	n->data = data;
	n->left = 0;
	n->right = 0;

}

tree_t *tree_create(void)
{
	tree_t	*t;

	t = (tree_t *) malloc(sizeof(tree_t));
	t->top = 0;

	return t;
}

// choose() < 0 means follow left node, choose() > 0 means follow right node
// choose() == 0 means we found the node
// note: choose() should never choose left or right if they are null
// returns 0 when not found

node_t *tree_find_node(tree_t *t, int (*choose)(node_t *), int insert)
{
	node_t *n;
	int done;

	n = t->top;

	done = 0;
	while (n && !done)
	{
		choice = choose(n);
		if (choice < 0)
		{
			if (!n->left && insert)
				n->left = tree_create_node(t);
			n = n->left;
		}
		else if (choice > 0)
		{
			if (!n->right && insert)
				n->right = tree_create_node(t);
			n = n->right;
		}
		else
			done = 1;
	}

	return n;

}

node_t *tree_insert_node(tree_t *t, void *data, int (*choose)(node_t *))
{

	n = tree_find_node(t, choose, 1);
	n->data = data;

	return n;

}

void tree_destroy_node(tree_t *t, node_t *n)
{
	if (!n->left && !n->right)
	{
		t->destructor(n->data);
		n->data = 0;
	}
	if (n->left)
	{
		tree_destroy_node(t, n->left);
		free(n->left);
		n->left = 0;
	}
	if (n->right)
	{
		tree_destroy_node(t, n->right);
		free(n->right);
		n->right = 0;
	}
}

void tree_destroy(tree_t *t, void (*destructor)(void *))
{

	if (destructor)
		t->destructor = destructor;
	else
		destructor = (void (*)(void*)) free;

	tree_destroy_node(t, t->top);
	t->top = 0;

}

