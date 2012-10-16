#include "seg_tree.h"
#include <stdlib.h>
#include "types.h"
#include "tree.h"
#include "hash_table.h"
#include "log.h"
#include "list.h"
#include <string.h>

#define DEFAULT_SEG_TREE_NODE_COUNT 512         //默认节点个数
typedef struct tree_key {
    uint start;
    uint end;
} tree_key_t;

struct _seg_tree {
    tree_t *tree;
    seg_traverse_func traverse_func; //遍历的时候用到
    void *user_data;            //遍历的时候用到
};

static int key_equal(const void* a, const void* b, void *user_data)
{
    tree_key_t* v1 = (tree_key_t*) a;
    tree_key_t* v2 = (tree_key_t*) b;

    //v1包含v2
    if (v1->start <= v2->start && v1->end >= v2->end) {
        return 0;
    }
    //v2包含v1
    else if (v2->start <= v1->start && v2->end >= v1->end) {
        return 0;
    }
    //v2 > v1
    else if (v2->start > v1->end) {
        return -1;
    }
    //v1 > v2
    else if (v1->start > v2->end) {
        return 1;
    }

    //默认小于
    return -1;
}

struct _insert_parm {
    struct tree_key *new_key;
    list_t* list;
};

static bool seg_tree_insert_interal(void *key, void *value, void* user_data)
{
    struct _insert_parm *param = (struct _insert_parm*) user_data;
    struct tree_key *new_key = param->new_key;
    struct tree_key *old_key = (struct tree_key*) key;
    list_t* list = param->list;

    //这里的顺序很关键，不要随意交换位置
    if (old_key->start >= new_key->start && old_key->end <= new_key->end) {
        list_append(list, key);
        return false;
    } else if (old_key->start == new_key->end+1) {            //后面相邻
        new_key->end = old_key->end;
        list_append(list, key);
        return false;
    } else if (old_key->end + 1 == new_key->start) {            //前面相邻
        new_key->start = old_key->start;
        list_append(list, key);
        return false;
    }else if (old_key->end < new_key->start) {            //前面
        return false;
    } else if (old_key->start > new_key->end) {            //后面，结束遍历
        return true;
    }

    return true;
}

static bool seg_tree_remove_interal(void *key, void* user_data)
{
    tree_t *tree = (tree_t *) user_data;
    tree_remove(tree, key);
    return false;
}

static void adjust_tree(seg_tree_t *seg_tree, tree_key_t *new_key)
{

    struct _insert_parm param;
    param.new_key = new_key;
    param.list = list_new(32, NULL);
    tree_foreach(seg_tree->tree, seg_tree_insert_interal, &param);

    list_foreach(param.list, seg_tree_remove_interal, seg_tree->tree);
    list_free(param.list);

    tree_key_t *insert_key = (tree_key_t *) malloc(sizeof(tree_key_t));
    insert_key->start = new_key->start;
    insert_key->end = new_key->end;
    tree_insert(seg_tree->tree, insert_key, NULL);
}

seg_tree_t* seg_tree_new(destroy_func data_destroy_func)
{
    struct _seg_tree *seg_tree = (struct _seg_tree *) mem_alloc(
            sizeof(struct _seg_tree));

    seg_tree->tree = tree_new_full(DEFAULT_SEG_TREE_NODE_COUNT, key_equal, NULL,
            default_destroy_func, NULL);

    return seg_tree;
}

void seg_tree_insert(seg_tree_t *seg_tree, uint start, uint end)
{
    tree_key_t lookup_key1, lookup_key2;
    tree_key_t *orig_key1, *orig_key2;
    tree_key_t new_key;
    // tree_value_t *new_value;
    bool find_end, find_start;

    if (start > end) {
        LOG(LOG_LEVEL_WARNING, "end must larger than start");
        return;
    }

    //空树
    if (tree_nnodes(seg_tree->tree) == 0) {
        tree_key_t *temp_key = (tree_key_t *) mem_alloc(sizeof(tree_key_t));
        temp_key->start = start;
        temp_key->end = end;
        tree_insert(seg_tree->tree, temp_key, NULL);
        return;
    }

    lookup_key1.start = start;
    lookup_key1.end = start;
    find_start = tree_lookup_extended(seg_tree->tree,
            (const void*) &lookup_key1, (void**) &orig_key1, NULL);
    lookup_key2.start = end;
    lookup_key2.end = end;
    find_end = tree_lookup_extended(seg_tree->tree, (const void*) &lookup_key2,
            (void**) &orig_key2, NULL);

    if (find_start && find_end) {
        //包含关系
        if (orig_key1 == orig_key2) {
            return;
        } else {
            new_key.start = orig_key1->start;
            new_key.end = orig_key2->end;
        }
    } else if (!find_start && !find_end) {
        new_key.start = start;
        new_key.end = end;
    } else if (find_start) {
        new_key.start = orig_key1->start;
        new_key.end = end;
    } else if (find_end) {
        new_key.start = start;
        new_key.end = orig_key2->end;
    }

    adjust_tree(seg_tree, &new_key);

}

bool seg_tree_contains(seg_tree_t *seg_tree, const uint key)
{
    tree_key_t tree_key;

    tree_key.start = key;
    tree_key.end = key;

    return tree_lookup_extended(seg_tree->tree, &tree_key, NULL, NULL);
}

void seg_tree_free(seg_tree_t *seg_tree)
{
    tree_free(seg_tree->tree);

    mem_free(seg_tree);
}

static bool local_traverse_func(void *key, void* value, void* user_data)
{
    seg_tree_t *seg_tree = (seg_tree_t *) user_data;
    tree_key_t *tree_key = (tree_key_t *) key;

    return seg_tree->traverse_func(tree_key->start, tree_key->end,
            seg_tree->user_data);
}

void seg_tree_foreach(seg_tree_t *seg_tree, seg_traverse_func func,
        void* user_data)
{
    seg_tree->user_data = user_data;
    seg_tree->traverse_func = func;
    tree_foreach(seg_tree->tree, local_traverse_func, seg_tree);
}

uint seg_tree_nnodes(seg_tree_t *seg_tree)
{
    return tree_nnodes(seg_tree->tree);
}
