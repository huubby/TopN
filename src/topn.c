/*
 * topn.c
 *
 *  Created on: 2012-6-6
 *      Author: Administrator
 */

#include "topn.h"
#include "tree.h"
#include "memory.h"

struct _topn {
    tree_t *tree;
    void *min_key;
    uint topn;
    compare_func key_compare_func;
    void *user_data;
};

static int my_key_compare_func(const void *a, const void *b, void *user_data)
{
    topn_t *topn_tree = (topn_t *) user_data;
    int ret;
    ret = topn_tree->key_compare_func(a, b, topn_tree->user_data);

    return ret == 0 ? -1 : ret;
}

topn_t* topn_new(uint topn, compare_func key_compare_func, void *user_data,
        destroy_func key_destroy_func, destroy_func value_destroy_func)
{
    topn_t *topn_tree = (struct _topn*) mem_alloc(sizeof(topn_t));
    topn_tree->tree = tree_new_full(topn + 1, my_key_compare_func, topn_tree,
            key_destroy_func, value_destroy_func);
    topn_tree->min_key = NULL;
    topn_tree->topn = topn;
    topn_tree->key_compare_func = key_compare_func;
    topn_tree->user_data = user_data;

    return topn_tree;
}

void topn_insert(topn_t *topn, void* key, void* value)
{
    if (tree_nnodes(topn->tree) < topn->topn) { //没满
        tree_insert(topn->tree, key, value);
        if (topn->min_key == NULL
                || topn->key_compare_func(topn->min_key, key, topn->user_data)
                        > 0) {
            topn->min_key = key;
        }
    } else {
        if (topn->key_compare_func(topn->min_key, key, topn->user_data) >= 0) { //小于最小值
            return;
        } else { //插入
            topn->min_key = tree_remove_first_node(topn->tree);
            tree_insert(topn->tree, key, value);
        }
    }
}

void topn_free(topn_t *topn)
{
    tree_free(topn->tree);
    mem_free(topn);
}

uint topn_size(topn_t *topn)
{
    return tree_nnodes(topn->tree);
}

void topn_foreach(topn_t *topn, traverse_pair_func func, void* user_data)
{
    tree_reverse_foreach(topn->tree, func, user_data);
}
