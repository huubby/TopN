#ifndef  _LIST_H_
#define  _LIST_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif 

typedef struct _list list_t;

struct list_node {
    void *data;
    struct list_node *next;
    struct list_node *prev;
};

/*
 *@brief        创建列表
 *@param in     size      默认节点个数，如果超过这个个数，每次增加这么多个，如果为0，采取默认策略
 *@param in     func      销毁列表时对数组内的每个元素调用该函数，如果该参数为NULL，则销毁数组时什么都不做
 *@return       新列表
 */
list_t* list_new(uint size, destroy_func func);

/*
 *@brief        释放列表
 *@param in     list      待释放列表
 *@return
 */
void list_free(list_t *list);

/*
 *@brief        向列表末尾添加数据
 *@param in     list   待操作列表
 *@param in     data   要插入的数据
 *@return       新插入的数据对应的节点
 */
struct list_node* list_append(list_t *list, void *data);

/*
 *@brief        向列表末尾添加数据，如果列表中已经存在相等的数据，则不插入
 *@param in     list   待操作列表
 *@param in     data   要插入的数据
 *@param in     func    比较函数
 *@return       新插入的数据对应的节点，如果数据已经存在，返回已存在的节点
 */
struct list_node* list_append_unique(list_t *list, void *data, compare_func func, void *user_data);

/*
 *@brief        向列表前面添加数据
 *@param in     list   待操作列表
 *@param in     data   要插入的数据
 *@return       新插入的数据对应的节点
 */
struct list_node* list_prepend(list_t *list, void *data);

/*
 *@brief        向列表中插入数据，依次和列表中已有的数据做比较
 *              如果找到比待插入数据大的节点，则插入到找到的节点前面
 *@param in     list        待操作列表
 *@param in     data        要插入的数据
 *@param in     func        比较函数
 *@return       新插入的数据对应的节点
 *
 *@notice       如果每次插入的时候都调用这个函数，并且指定比较函数
 *              则最后的列表是从小到大有序的
 */
struct list_node* list_insert_sorted(list_t *list, void *data, compare_func func, void *user_data);

/*
 *@brief        在列表的指定节点前插入数据，确保sibling节点存在，否则结果未知
 *@param in     list        待操作列表
 *@param in     sibling     待插入数据的位置
 *@param in     data        要插入的数据
 *@return       新插入的数据对应的节点
 */
struct list_node* list_insert_before(list_t *list, struct list_node *sibling, void *data);

/*
 *@brief        在列表的指定节点后插入数据，确保sibling节点存在，否则结果未知
 *@param in     list        待操作列表
 *@param in     sibling     待插入数据的位置
 *@param in     data        要插入的数据
 *@return       新插入的数据对应的节点
 */
struct list_node* list_insert_before(list_t *list, struct list_node *sibling, void *data);


/*
 *@brief        连接list1和list2
 *              list1变成两个列表的和，list2变成空列表
 *@param in     list1       目的列表
 *@param in     list2       源列表
 *@return
 *
 *@notice       连接的两个列表的destroy_func要保持一致，否则会出现内存问题
 */
void list_concat(list_t *list1, list_t *list2);

/*
 *@brief        移除列表中第一个等于指定数据的节点
 *              移除时会调用destroy_func
 *@param in     list        待操作列表
 *@param in     data        待移除的数据
 *@param in     func        比较函数
 *@return       如果找到要删除数据，返回true，否则返回false
 */
bool list_remove(list_t *list, const void *data, compare_func func, void *user_data);

/*
 *@brief        删除列表节点, 要保证节点属于该列表, 否则结果未知
 *              会调用destroy_func
 *@param in     list        待操作列表
 *@param in     node        待删除的节点
 *@return
 */
void list_remove_node(list_t *list, struct list_node *node);

/*
 *@brief        移除列表中所有节点
 *              移除时会调用destroy_func
 *@param in     list        待操作列表
 *@return
 */
void list_remove_all(list_t *list);

/*
 *@brief        从列表中获取一个节点
 *              该节点会从列表中删除，但不会调用destroy_func
 *@param in     list        待操作列表
 *@param in     data        待获取节点对应的数据
 *@param in     func        比较函数
 *@return       获取到的节点, 如果没找到，返回NULL
 */
//struct list_node * list_steal_data(list_t *list, const void *data, compare_func func);

/*
 *@brief        从列表中获取一个节点
 *              该节点会从列表中删除，但不会调用destroy_func
 *@param in     list        待操作列表
 *@param in     node        待获取的节点
 *@return       获取到的节点, 等于参数中传入的节点
 */
//struct list_node * list_steal(list_t *list, struct list_node *node);


/*
 *@brief        反转列表
 *@param in     list        待操作列表
 *@return
 */
void list_reverse(list_t *list);

/*
 *@brief        拷贝列表
 *@param in     list        待拷贝列表
 *@param in     func        新列表的destroy_func
 *@return       拷贝出来的新的列表
 *
 *@notice       新列表和原来列表内部的数据相等，都指向同一个位置
 */
list_t* list_duplicate(const list_t *list, destroy_func func);

/*
 *@brief        查找节点
 *@param in     list        待查找列表
 *@param in     data        待查找的数据
 *@param in     func        比较函数
 *@return       找到的节点，如果没找到，返回NULL
 */
struct list_node*  list_find(list_t *list, const void *data, compare_func func, void *user_data);

/*
 *@brief        取列表尾部节点（最后一个）
 *@param in     list        列表
 *@return       列表尾，如果列表为空，返回NULL
 */
struct list_node * list_tail(list_t *list);

/*
 *@brief        取列表头部节点（第一个）
 *@param in     list        列表
 *@return       列表头，如果列表为空，返回NULL
 */
struct list_node * list_head(list_t *list);

/*
 *@brief        取列表节点个数
 *@param in     list        列表
 *@return       列表节点个数
 */
uint list_size(list_t *list);

/*
 *@brief        遍历列表，当遍历函数返回true时，停止遍历
 *@param in     list        列表
 *@param in     func        遍历函数
 *@param in     user_data   调用遍历函数时传入的参数
 *@return
 */
void list_foreach(list_t *list, traverse_func func, void *user_data);

/*
 *@brief        排序
 *@param in     list        列表
 *@param in     func        比较函数
 *@param in     asc         tree/false：升序/降序
 *@return
 */
//void list_sort(list_t *list, compare_func func, bool asc);


//用来释放列表的destroy_func函数
extern void list_destroy_func(void* data);

#ifdef __cplusplus
}
#endif 

#endif /*  _LIST_H_ */
