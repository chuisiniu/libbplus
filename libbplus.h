#ifndef _LIBBPLUS_H_
#define _LIBBPLUS_H_

#ifndef NULL
#define NULL ((void*)0)
#endif

/**
 * @brief 标记结点的类型
 *
 */
typedef enum bp_node_type {
	BP_NODE_TYPE_INNER, /** 表示内部结点 */
	BP_NODE_TYPE_DATA, /** 表示数据结点 */

	BP_NODE_TYPE_MAX,
} bp_node_type_e;

/**
 * @brief 表示B+树上的一个结点
 *
 */
typedef struct bp_node {
	bp_node_type_e type; /** B+树结点的类型 */
} bp_node_t;

/**
 * @brief 表示一棵B+树
 *
 */
typedef struct bp_tree {
	int        max_idx_num;  /** 每个内部索引结点保存索引项的个数 */
	int        max_data_num; /** 每个吐字数据结点保存的数据项的个数 */
	int        key_size;     /** 索引对象的数据长度，比如给 ipv4 数据建索引，长度就是4 */
	int        value_size;   /** 位置信息的数据长度 */
	bp_node_t *head;
	bp_node_t *data;
} bp_tree_t;

typedef int (* bp_compare_f)(unsigned char *a, unsigned char *b, int size);

#endif /* _LIBBPLUS_H_ */
