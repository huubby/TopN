/**
 *Description:      topn排序，内部采用tree_t实现
 *
 **/

#ifndef TOPN_H_
#define TOPN_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _topn topn_t;

/*
 *@brief        创建topn对象
 *@param in     topn                topn值
 *@param in     key_compare_func    key比较函数
 *@param in     key_destroy_func    key释放函数
 *@param in     value_destroy_func  value释放函数
 *@return       新topn对象
 *
 *@notice       由于数的
 */
topn_t* topn_new(uint topn, compare_func key_compare_func, void *user_data,
        destroy_func key_destroy_func, destroy_func value_destroy_func);

/*
 *@brief        往topn对象中插入数据
 *@param in     topn    topn对象
 *@param in     key     key
 *@param in     value   value
 *@return
 *
 *@notice       任意两个key的地址不能相同
 */
void topn_insert(topn_t *topn, void* key, void* value);

/*
 *@brief        释放topn对象
 *@param in     topn    topn对象
 *@return
 */
void topn_free(topn_t *topn);

/*
 *@brief        获取存储的数据数量
 *@param in     topn    topn对象
 *@return
 */
uint topn_size(topn_t *topn);

/*
 *@brief        遍历topn里面的所有数据，当遍历函数返回true时，停止遍历
 *@param in     topn    topn对象
 *@param in     func        遍历函数
 *@param in     user_data   调用遍历函数时传入的参数
 *@return
 */
void topn_foreach(topn_t *topn, traverse_pair_func func, void* user_data);


#ifdef __cplusplus
}
#endif

#endif /* TOPN_H_ */
