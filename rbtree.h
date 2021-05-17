#ifndef __h_rbtree_H__
#define __h_rbtree_H__

#define rbt_red(node)               ((node)->color = 1)
#define rbt_black(node)             ((node)->color = 0)
#define rbt_is_red(node)            ((node)->color)
#define rbt_is_black(node)          (!rbt_is_red(node))
#define rbt_copy_color(n1, n2)      (n1->color = n2->color)

typedef struct rbtree_node  RBTREE_NODE;
struct rbtree_node {
    unsigned long int    key;
    RBTREE_NODE     *left;
    RBTREE_NODE     *right;
    RBTREE_NODE     *parent;
    unsigned char   color;
    unsigned char   data;
};
typedef struct rbtree  RBTREE;
typedef void (*rbtree_insert_pt) (RBTREE_NODE *root, RBTREE_NODE *node, RBTREE_NODE *sentinel);
struct rbtree {
    RBTREE_NODE     *root;
    RBTREE_NODE     *sentinel;
    rbtree_insert_pt   insert;
};

void rbtree_init(RBTREE *tree, RBTREE_NODE *node, void *args);
void rbtree_insert(RBTREE *tree, RBTREE_NODE *node);
void rbtree_delete(RBTREE *tree, RBTREE_NODE *node);
void rbtree_insert_value(RBTREE_NODE *root, RBTREE_NODE *node,RBTREE_NODE *sentinel);
void rbtree_insert_timer_value(RBTREE_NODE *root,RBTREE_NODE *node, RBTREE_NODE *sentinel);
RBTREE_NODE *rbtree_next(RBTREE *tree,RBTREE_NODE *node);
RBTREE_NODE * rbtree_min(RBTREE_NODE *node, RBTREE_NODE *sentinel);

/* a sentinel must be black */
#endif
