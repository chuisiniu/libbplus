/**
 * @file bplus.c
 * @brief B+树索引实现
 * @version 0.1
 * @date 2021-10-01
 *
 * B+树用于创建索引，B+树本质上保存的是 key-value 对，其中 key 是被索引项，value是被索引
 * 项的位置信息。B+树上的叶子结点是被索引项的一级索引，保存了所有的被索引项在磁盘上的位置信息，
 * 叶子结点的父结点为次级索引，用于被索引项的查找。
 *
 * 一棵B+树大概是下面这样的：
 *                                 (node 1)
 *                             +-------------+
 *                             |P1 K1n|P2 K2n|
 *                             +-------------+
 *                              /       \
 *                             /         \
 *         (node 2)           /           \         (node3)
 * +----------------------------------+   +----------------------------------+
 * |K11 V11|K12 V12|...|K1n V1n|P1next|-->|K21 V21|K22 V22|...|K2n V2n|P2next|
 * +----------------------------------+   +----------------------------------+
 * 其中 node 2 和 node 3 都是数据结点，保存了所有的被索引项（Kij）和被索引项的位置（Vij）
 * 以及下一个叶子结点的位置，用于顺序访问被索引项。node 1 是内部结点，其中 node 1 的 P1
 * 指向 node 2 ， K1n 是 node 2 中数据的最大值；node 1 的 P2 指向 node 3 ， K2n 是
 * node 3 中数据的最大值。
 *
 * B+树有两个阶（order），一个用于内部结点，一个用于叶子结点，设用于内部结点的阶为 a ，用于
 * 叶子结点的阶为 b 。
 *
 * 对于阶为 a 的内部结点：
 * +----------------------------------------------------------------------------+
 * |P_1        K_1|P_2        K_2|...|P_i        K_i|...|P_n-1        K_n-1|P_n |
 * +----------------------------------------------------------------------------+
 *  |              |                  |                  |                  |
 *  V              V                  V                  V                  V
 * {x:x<K_1}     {x:K_1<x<=K_2}     {x:K_i-1<x<=K_i}   {x:K_n-2<x<=K_n-1} {x:x>K_n-1}
 * 有：
 * 1. 对于每一个 <P1,K1,P2,K2,...Pn,Kn> 的内部结点，n <= a ，Pi指向其子树的根结点，Ki
 *    为被索引项的值
 * 2. Ki满足 K1 < K2 < K3 < ... < Kn
 * 3. 对于Pi指向的子树Xi中的元素X，满足 1 < i < n 时 K_i-1 < X < K_i ；
 *    i = n 时 X > K_n-1 ； i = 1 时 X < K_1
 * 4. 每个内部结点最多有 a 个指向子树的指针，即 n 最大取 a
 * 5. 根结点至少包含两个指向子树的结点指针，其它内部结点包含最少 ceil(a/2) 个指向子树的指针
 * 6. 如果任意一个内部结点包含 n 个指向孩子结点的指针且 n <= a，则该结点包含 n - 1 个被索
 *    引项
 *
 * 对于阶为 b 的叶子结点：
 * +--------------------------------------------------+
 * |K_1 V_1|K_2 V_2|...|K_i V_i|...|K_n-1 V_n-1|P_next|
 * +--------------------------------------------------+
 * 有：
 * 1. K_i 为被索引项， V_i 为位置信息， P_next 为下一个叶子结点的指针
 * 2. n <= b
 * 3. 每个叶子结点至少包含 ceil(a/2) 个值
 * 4. 所有的叶子结点在同一层
 */

#include <string.h>
#include <stdlib.h>

#include "libbplus.h"

typedef int (* bp_node_insert_f)(
	bp_node_t      *node,
	unsigned char  *key,
	unsigned char  *position,
	bp_node_t     **pp_new);

typedef int (* bp_node_get_max_key_f)(
	bp_node_t     *node,
	unsigned char *key_buf,
	int            key_buf_len);

/**
 * @brief
 *  内部结点和数据结点通用的信息
 *
 * @details
 *  作为内部结点和数据结点的第一个成员变量，这样就可以根据 type 值把
 *  bp_node_common_t 转换为对应的指针
 *
 */
typedef struct bp_node_common {
	bp_node_type_e type;

	bp_node_insert_f      insert; /** 用于向当前结点插入数据 */
	bp_node_get_max_key_f max_key; /** 用于获取当前结点保存的最大 key 值 */
	bp_compare_f          compare; /** 用于比较 key 值 */

	int max_key_num; /** 可以保存的被索引项最大个数 */
	int min_key_num; /** 至少要保存的被索引项的个数 */
	int key_num; /** 实际保存的被索引项的个数 */
} bp_node_common_t;

/**
 * @brief
 *  表示一个数据结点
 */
typedef struct bp_data_node {
	bp_node_common_t common; /** B+树结点通用结构 */

	int key_size; /** 被索引项的大小 */
	int value_size; /** 位置信息的大小 */

	int            content_len; /** content的长度 */
	unsigned char  content[0]; /** 保存的数据 */
} bp_data_node_t;

typedef struct bp_inner_node {
	bp_node_common_t common; /** B+树结点通用结构 */

	int key_size; /** 被索引项的大小 */
	int key_total; /** 所有子树最终指向的数据结点的被索引项的个数 */

	int            content_len; /** content的长度 */
	unsigned char  content[0]; /** 保存的数据 */
} bp_inner_node_t;

bp_node_t *bp_create_data_node(
	int          max_kv_num,
	int          min_kv_num,
	int          key_size,
	int          value_size,
	bp_compare_f compare);

bp_node_t *bp_create_inner_node(int max_key_num, int key_size, bp_compare_f compare);

#define bp_data_node_get_item_size(_node) \
	((_node)->key_size + (_node)->value_size)
#define bp_data_node_get_valid_data_len(_node, _item_size) \
	((_node)->common.key_num * item_size)

/**
 * @brief 计算内部结点保存数据需要的占用的内存
 *
 * @param max_key_num 内部节点可以保存的最大的 key 的个数
 * @param key_size 被索引项的数据长度
 * @return int 内部结点数据区域的大小
 */
int bp_calc_inner_node_content_len(int max_key_num, int key_size)
{
	// 需要保存 max_key_num 个 key 和 max_key_num + 1 个指针
	return max_key_num * (key_size + sizeof(bp_node_t *)) + sizeof(bp_node_t *);
}

/**
 * @brief 计算要保存需要数据需要的数据结点占用的内存
 *
 * @param max_kv_num 叶子结点保存键值对的最大个数
 * @param key_size 被索引项的数据长度
 * @param value_size 位置信息的数据长度
 * @return int 数据结点数据区域的大小
 */
int bp_calc_data_node_content_len(
	int max_kv_num,
	int key_size,
	int value_size)
{
	// 需要保存 max_kv_num 个 key value 和 1 个指针
	return max_kv_num * (key_size + value_size)	+ sizeof(bp_data_node_t *);
}

static int bp_compare(unsigned char *a, unsigned char *b, int size)
{
	return memcmp(a, b, size);
}

/**
 * @brief 在节点 content 的 item 列表中查找 target
 *
 * @param content 结点内容，可以是内部结点也可以是叶子结点
 * @param content_len 内容长度，这里需要传入的是实际已经插入的数据的长度，而不是全部长度
 * @param item_len content 中一个数据项的长度，对于内部结点，数据项内容为 ptr + key，
 *                 item_len 为 sizeof(ptr) + sizeof(key)，对于叶子结点，数据项内容
 *                 为 key + value， item_len 为 sizeof(key) + sizeof(value)
 * @param target 要查找的内容
 * @param target_size 要查找内容的长度
 * @param offset 要查找内容在 item 中的偏移量，内部结点的数据格式为 ptr + key，需要传入
 *               的查找偏移量为 sizeof(ptr)，叶子结点的数据格式为 key + value，需要传
 *               入的查找偏移量为 0
 * @param compare 比较 key 值的函数
 * @param found_idx 如果找到了则返回找到的位置，否则返回比 target 大的第一个值的位置
 * @return int 找到返回1，否则返回0
 */
int bp_bi_search_one(
	unsigned char *content,
	int            content_len,
	int            item_size,
	unsigned char *target,
	int            target_size,
	int            offset,
	bp_compare_f   compare,
	int           *found_idx)
{
	int            item_num;
	int            comp_res;
	int            half_item_num;
	unsigned char *low;
	unsigned char *high;
	unsigned char *mid;

	if (0 == content_len) {
		*found_idx = 0;

		return 0;
	}

	compare = compare ? compare : bp_compare;
	item_num = content_len / item_size;
	low      = content;
	high     = low + (item_num - 1) * item_size;
	while (low <= high) {
		half_item_num = (item_num) / 2;
		half_item_num = half_item_num == 0 ? 1 : half_item_num;
		mid           = low + (half_item_num - 1) * item_size;
		comp_res      = compare(target, mid + offset, target_size);

		if (0 == comp_res) {
			*found_idx = (mid - content) / item_size;

			return 1;
		}

		if (0 < comp_res) {
			low      = mid + item_size;
			item_num = item_num - half_item_num;
		} else {
			high     = mid - item_size;
			item_num = half_item_num - 1;
		}
	}

	if (comp_res < 0)
		*found_idx = (mid - content) / item_size;
	else
		*found_idx = (low - content) / item_size;

	return 0;
}

/**
 * @brief 在节点 content 的 item 列表中查找第一个 target
 *
 * @param content 结点内容，可以是内部结点也可以是叶子结点
 * @param content_len 内容长度，这里需要传入的是实际已经插入的数据的长度，而不是全部长度
 * @param item_len content 中一个数据项的长度，对于内部结点，数据项内容为 ptr + key，
 *                 item_len 为 sizeof(ptr) + sizeof(key)，对于叶子结点，数据项内容
 *                 为 key + value， item_len 为 sizeof(key) + sizeof(value)
 * @param target 要查找的内容
 * @param target_size 要查找内容的长度
 * @param offset 要查找内容在 item 中的偏移量，内部结点的数据格式为 ptr + key，需要传入
 *               的查找偏移量为 sizeof(ptr)，叶子结点的数据格式为 key + value，需要传
 *               入的查找偏移量为 0
 * @param found_idx 如果找到了则返回找到的第一个数据的位置，否则返回第一个大于 target
 *                  的第一个数据的位置
 * @param compare 比较 key 值的函数
 * @return int 找到返回1，否则返回0
 */
int bp_bi_search_first(
	unsigned char *content,
	int            content_len,
	int            item_size,
	unsigned char *target,
	int            target_size,
	int            offset,
	bp_compare_f   compare,
	int           *found_idx)
{
	int found;

	compare = compare ? compare : bp_compare;

	found = bp_bi_search_one(content, content_len, item_size, target,
							 target_size, offset, compare, found_idx);

	if (0 == found)
		return 0;

	while (*found_idx > 0)
		if (0 == compare(content + (*found_idx - 1) * item_size + offset,
							target, target_size))
			*found_idx = *found_idx - 1;
		else
			break;

	return 1;
}

/**
 * @brief 在节点 content 的 item 列表中查找最后一个 target
 *
 * @param content 结点内容，可以是内部结点也可以是叶子结点
 * @param content_len 内容长度，这里需要传入的是实际已经插入的数据的长度，而不是全部长度
 * @param item_len content 中一个数据项的长度，对于内部结点，数据项内容为 ptr + key，
 *                 item_len 为 sizeof(ptr) + sizeof(key)，对于叶子结点，数据项内容
 *                 为 key + value， item_len 为 sizeof(key) + sizeof(value)
 * @param target 要查找的内容
 * @param target_size 要查找内容的长度
 * @param offset 要查找内容在 item 中的偏移量，内部结点的数据格式为 ptr + key，需要传入
 *               的查找偏移量为 sizeof(ptr)，叶子结点的数据格式为 key + value，需要传
 *               入的查找偏移量为 0
 * @param compare 比较 key 值的函数
 * @param found_idx 如果找到了则返回找到的最后一个数据的位置，否则返回第一个大于 target
 *                  的第一个数据的位置
 * @return int 找到返回1，否则返回0
 */
int bp_bi_search_last(
	unsigned char *content,
	int            content_len,
	int            item_size,
	unsigned char *target,
	int            target_size,
	int            offset,
	bp_compare_f   compare,
	int           *found_idx)
{
	int found;

	compare = compare ? compare : bp_compare;
	found = bp_bi_search_one(content, content_len, item_size, target,
							 target_size, offset, compare, found_idx);

	if (0 == found)
		return 0;

	while (*found_idx < content_len / item_size - 1)
		if (0 == compare(content + (*found_idx + 1) * item_size + offset,
							target, target_size))
			*found_idx = *found_idx + 1;
		else
			break;

	return 1;
}

/**
 * @brief 数据结点是否需要分裂
 *
 * @param data 数据结点
 * @return int 需要分裂返回 1 ，否则返回 0
 */
int bp_data_node_need_split(bp_data_node_t *data)
{
	return data->common.key_num >= data->common.max_key_num;
}

/**
 * @brief 从数据结点的 content 上找到日后的 pnext 的保存位置
 *
 * @param data 数据结点
 * @return data 的 content 上保存 pnext 的地方
 */
bp_data_node_t **bp_data_node_get_pnext_save_position(bp_data_node_t *data)
{
	return (bp_data_node_t **)(data->content + data->content_len
		- sizeof(bp_data_node_t *));
}

/**
 * @brief 返回 data 结点的 pnext 值
 *
 * @param data 数据结点
 * @return data 结点的 pnext 值
 */
bp_data_node_t *bp_data_node_get_pnext(bp_data_node_t *data)
{
	return *bp_data_node_get_pnext_save_position(data);
}

/**
 * @brief 设置 data 结点的 pnext 值
 *
 * @param data 数据结点
 * @param pnext 要设置的值
 */
void bp_data_node_set_pnext(bp_data_node_t *data, bp_data_node_t *pnext)
{
	bp_data_node_t **pp_next;

	pp_next = bp_data_node_get_pnext_save_position(data);

	*pp_next = pnext;
}

/**
 * @brief 数据结点分裂
 *
 * @param to_split 要分裂的数据结点
 * @param pp_new 分裂出的新结点
 * @return int 分裂是否成功
 */
int bp_data_node_split(
	bp_node_t  *to_split,
	bp_node_t **pp_new)
{
	bp_data_node_t *old;
	bp_data_node_t *new;
	int             old_data_num;
	int             offset;
	int             copy_len;

	old = (bp_data_node_t *)to_split;
	new = (bp_data_node_t *)bp_create_data_node(
		old->common.max_key_num, old->common.min_key_num,
		old->key_size, old->value_size, old->common.compare);
	if (NULL == new) {
		*pp_new = NULL;

		return -1;
	}

	// 旧结点保存小的 1/2 数据，新结点保存大的 1/2 数据
	old_data_num = old->common.key_num;
	old->common.key_num = old_data_num / 2;
	new->common.key_num = old_data_num - old->common.key_num;

	// 复制数据到新的结点
	offset   = old->common.key_num * (old->key_size + old->value_size);
	copy_len = new->common.key_num * (new->key_size + new->value_size);
	memcpy(new->content, old->content + offset, copy_len);

	// 新结点的 pnext 指向旧结点的 pnext，旧结点的 pnext 指向新结点
	bp_data_node_set_pnext(new, bp_data_node_get_pnext(old));
	bp_data_node_set_pnext(old, new);

	*pp_new = (bp_node_t *)new;

	return 0;
}

/**
 * @brief 插入被索引项和位置信息
 *
 * @param data 数据结点
 * @param key 被索引项
 * @param position 位置信息
 * @param pp_new 如果 data 保存满了则进行结点分裂，返回分裂出的新结点
 * @return int 成功返回 0 ，否则返回 -1
 */
int bp_data_node_insert_data(
	bp_node_t      *node,
	unsigned char  *key,
	unsigned char  *position,
	bp_node_t     **pp_new)
{
	bp_data_node_t *data;
	int             valid_data_len;
	int             item_size;
	int             found;
	int             found_idx;
	int             cmp_res;
	unsigned char  *dst;
	unsigned char  *tmp;
	unsigned char  *max_key_of_data;
	unsigned char  *min_key_of_new;

	data    = (bp_data_node_t *)node;
	*pp_new = NULL;

	if (bp_data_node_need_split(data)
		&& -1 == bp_data_node_split(node, pp_new))
		return -1;

	// 如果分裂了，需要找出 key 值插入旧结点还是新结点
	if (*pp_new) {
		max_key_of_data = data->content + (data->common.key_num - 1) *
			(data->key_size + data->value_size);
		cmp_res = memcmp(key, max_key_of_data, data->key_size);
		if (0 < cmp_res) {
			// key 大于 max_key_of_data 时，新 key 应该插入到分裂出的结点
			data = (bp_data_node_t *)*pp_new;
		} else if (0 == cmp_res) {
			// 如果 data 存在相等的 key 值，且分裂点正好在相等的 key 值中间时
			// 新 key 应该插入到分裂出的结点
			min_key_of_new = ((bp_data_node_t *)(*pp_new))->content;
			if (0 == memcpy(key, min_key_of_new, data->key_size))
				data = (bp_data_node_t *)*pp_new;
		}
	}

	// 查找合适的位置插入，存在相同 key 值时，要确保 key 值要插入到相同 key 值的后面
	item_size      = bp_data_node_get_item_size(data);
	valid_data_len = bp_data_node_get_valid_data_len(data, item_size);
	found = bp_bi_search_last(data->content, valid_data_len, item_size, key,
							  data->key_size, 0, data->common.compare, &found_idx);
	dst = data->content + found_idx * item_size;
	if (found)
		dst += item_size;

	// 向后移动数据，腾出 key position 的位置
	if (dst < data->content + valid_data_len)
		for (tmp = data->content + valid_data_len + item_size;
			 tmp > dst;
			 tmp -= item_size)
			memcpy(tmp, tmp - item_size, item_size);

	memcpy(dst, key, data->key_size);
	memcpy(dst + data->key_size, position, data->value_size);

	data->common.key_num += 1;

	return 0;
}

/**
 * @brief 返回数据结点中保存的最大 key 值，也就是最后一个 key
 *
 * @param node 数据结点
 * @param key_buf 用于返回 key 值
 * @param key_buf_len key_buf 的长度
 * @return 如果数据结点是空的，返回 -1 ，否则返回 key_buf 上实际数据的长度
 */
int bp_data_node_max_key(
	bp_node_t     *node,
	unsigned char *key_buf,
	int            key_buf_len)
{
	bp_data_node_t *data;
	int             cp_len;

	data = (bp_data_node_t *)node;
	if (data->common.key_num <= 0)
		return -1;

	cp_len = key_buf_len < data->key_size ? key_buf_len : data->key_size;
	memcpy(key_buf, data->content + (data->common.key_num - 1)
		* (data->key_size + data->value_size), cp_len);

	return cp_len;
}

/**
 * @brief 创建一个空的叶子结点
 *
 * @param max_kv_num 叶子结点保存键值对的最大个数
 * @param min_kv_num 叶子结点保存键值对的最小个数
 * @param key_size 被索引项的数据长度
 * @param value_size 位置信息的数据长度
 * @param compare 比较 key 值的函数
 * @return bp_node_t* 新建的结点
 */
bp_node_t *bp_create_data_node(
	int          max_kv_num,
	int          min_kv_num,
	int          key_size,
	int          value_size,
	bp_compare_f compare)
{
	bp_data_node_t *new;
	int             content_len;

	content_len = bp_calc_data_node_content_len(max_kv_num, key_size, value_size);
	new = malloc(sizeof(*new) + content_len);
	if (NULL == new)
		return NULL;

	new->common.type        = BP_NODE_TYPE_DATA;
	new->common.max_key_num = max_kv_num;
	new->common.min_key_num = min_kv_num;
	new->common.key_num     = 0;
	new->common.insert      = bp_data_node_insert_data;
	new->common.max_key     = bp_data_node_max_key;
	new->common.compare     = compare;

	new->key_size    = key_size;
	new->value_size  = value_size;
	new->content_len = content_len;
	memset(new->content, 0, new->content_len);

	return (bp_node_t *)new;
}

bp_node_t *bp_inner_node_get_insert_position(
	bp_inner_node_t *inner,
	int              found_idx)
{
	// 如果 found_idx 落在 content 的有效数据区域内，说明 found_pos 指向的 p-k 对
	// 中的 k 是第一个大于等于 key 的值，key 应该插入到 p 指向的结点；否则的话说明 key
	// 大于当前结点的最大值，需要取最后一个 p-k 对中的 p ，在插入完成后更新最后一个 p-k
	// 对的 k
	if (found_idx < inner->common.key_num)
		return *((bp_node_t **)(inner->content
			+ found_idx * (sizeof(bp_node_t *) + inner->key_size)));

	return *((bp_node_t **)(inner->content
		+ (inner->common.key_num - 1)
		* (sizeof(bp_node_t *) + inner->key_size)));
}

int bp_inner_node_split(bp_node_t *to_split, bp_node_t **pp_new)
{
	bp_inner_node_t *old;
	bp_inner_node_t *new;
	int              old_data_num;
	int              offset;
	int              copy_len;

	old = (bp_inner_node_t *)to_split;
	new = (bp_inner_node_t *)bp_create_inner_node(
		old->common.max_key_num, old->key_size, old->common.compare);
	if (NULL == new) {
		*pp_new = NULL;

		return -1;
	}

	old_data_num = old->common.key_num;
	old->common.key_num = old_data_num / 2;
	new->common.key_num = old_data_num - old->common.key_num;

	offset   = old->common.key_num * (sizeof(bp_node_t *) + old->key_size);
	copy_len = new->common.key_num * (sizeof(bp_node_t *) + new->key_size);

	memcpy(new->content, old->content + offset, copy_len);

	*pp_new = (bp_node_t *)new;

	return 0;
}

int bp_inner_node_add_split_child(
	bp_inner_node_t   *inner,
	bp_node_common_t  *child,
	bp_node_common_t  *split_child,
	int                child_idx,
	bp_inner_node_t  **split_inner)
{
	int              item_size;
	int              valid_data_len;
	unsigned char   *child_position;
	unsigned char   *tmp;
	bp_inner_node_t *inner_contains_child;

	*split_inner = NULL;
	if (inner->common.key_num == inner->common.max_key_num)
		if (-1 == bp_inner_node_split((bp_node_t *)inner,
									  (bp_node_t **)split_inner))
			return -1;

	inner_contains_child = inner;
	if (*split_inner) {
		if (child_idx < inner->common.key_num) {
			inner_contains_child = inner;
		} else {
			inner_contains_child = *split_inner;
			child_idx -= inner->common.key_num;
		}
	}

	item_size      = sizeof(bp_node_t *) + inner->key_size;
	valid_data_len = inner->common.key_num * item_size;
	// 更新 child 所在的 child_position 中的最大 key 值为 child 的最大 key 值
	child_position = inner_contains_child->content + child_idx * item_size;
	child->max_key((bp_node_t *)child, child_position + sizeof(bp_node_t *),
		inner->key_size);

	// child 后面的数据后移，腾出 child_split 的空间
	for (tmp = inner_contains_child->content + valid_data_len - item_size;
		 tmp > child_position;
		 tmp -= item_size)
		memcpy(tmp + item_size, tmp, item_size);

	// 插入 child_split
	tmp = child_position + item_size;
	memcpy(tmp, &split_child, sizeof(split_child));
	split_child->max_key((bp_node_t *)split_child, tmp + sizeof(bp_node_t *),
		inner_contains_child->key_size);
	inner_contains_child->common.key_num += 1;

	return 0;
}

int bp_inner_node_insert_data(
	bp_node_t      *node,
	unsigned char  *key,
	unsigned char  *position,
	bp_node_t     **pp_new)
{
	bp_inner_node_t  *inner;
	bp_node_common_t *child;
	bp_node_common_t *split_child;
	int               item_size;
	int               valid_data_len;
	int               found_idx;

	inner   = (bp_inner_node_t *)node;
	*pp_new = NULL;

	// 空B+树插入第一个 key 时，使用第一个 key 作为第一个子结点的最大值
	if (0 == inner->common.key_num) {
		memcpy(inner->content + sizeof(bp_node_t *), key, inner->key_size);

		inner->common.key_num = 1;
	}

	item_size      = sizeof(bp_node_t *) + inner->key_size;
	valid_data_len = inner->common.key_num * item_size;
	bp_bi_search_last(inner->content, valid_data_len, item_size, key,
					  inner->key_size, sizeof(bp_node_t *),
					  inner->common.compare, &found_idx);
	if (found_idx == inner->common.key_num)
		memcpy(inner->content + (found_idx - 1) * item_size + sizeof(bp_node_t *),
			   key, inner->key_size);
	child = (bp_node_common_t *)bp_inner_node_get_insert_position(
		inner, found_idx);
	if (-1 == child->insert((bp_node_t *)child, key, position,
							(bp_node_t **)&split_child))
		return -1;

	if (split_child)
		bp_inner_node_add_split_child(inner, child, split_child, found_idx,
									  (bp_inner_node_t **)pp_new);

	inner->key_total += 1;

	return 0;
}

int bp_inner_node_max_key(
	bp_node_t     *node,
	unsigned char *key_buf,
	int            key_buf_len)
{
	bp_inner_node_t *inner;
	int              cp_len;

	inner = (bp_inner_node_t *)node;
	if (inner->common.key_num <= 0)
		return -1;

	cp_len = key_buf_len < inner->key_size ? key_buf_len : inner->key_size;
	memcpy(key_buf, inner->content + (inner->common.key_num - 1)
		* (sizeof(bp_node_t *) + inner->key_size) + sizeof(bp_node_t *),
		cp_len);

	return cp_len;
}

/**
 * @brief 创建一个空的内部节点
 *
 * @param max_key_num 内部节点可以保存的最大的 key 的个数
 * @param key_size 被索引项的数据长度
 * @param compare 比较 key 值的函数
 * @return bp_node_t* 新建的内部节点
 */
bp_node_t *bp_create_inner_node(int max_key_num, int key_size, bp_compare_f compare)
{
	bp_inner_node_t *new;
	int              content_len;

	content_len = bp_calc_inner_node_content_len(max_key_num, key_size);
	new = malloc(sizeof(*new) + content_len);
	if (NULL == new)
		return NULL;

	new->common.type        = BP_NODE_TYPE_INNER;
	new->common.max_key_num = max_key_num;
	new->common.min_key_num = max_key_num / 2;
	new->common.key_num     = 0;
	new->common.insert      = bp_inner_node_insert_data;
	new->common.compare     = compare;
	new->common.max_key     = NULL;

	new->key_size    = key_size;
	new->key_total   = 0;
	new->content_len = content_len;
	memset(new->content, 0, new->content_len);

	return (bp_node_t *)new;
}

/**
 * @brief 创建B+树设置头结点的第一个指针指向第一个数据结点
 *
 * @param inner B+树的头结点
 * @param first_data B+树的第一个数据结点
 */
void bp_inner_node_set_first_ptr(bp_inner_node_t *inner, bp_node_t *first_data)
{
	memcpy(inner->content, &first_data, sizeof(bp_node_t *));
}

/**
 * @brief 创建一棵B+树
 *
 * @param max_idx_num 内部结点的包含的被索引项的最大个数
 * @param max_data_num 叶子结点中包含被索引项及其位置信息的最大个数
 * @param key_size 被索引项的数据长度
 * @param value_size 位置信息的数据长度
 * @return bp_tree_t* 创建的B+树
 */
bp_tree_t *bp_create_tree(
	int          max_idx_num,
	int          max_data_num,
	int          key_size,
	int          value_size,
	bp_compare_f compare)
{
	bp_tree_t     *new;

	// 数据结点分裂的时候，每个新结点至少要有 max_idx_num/2 个数据，所以需要数据
	// 结点的最大数据个数要大于内部结点的最大数据个数
	if (max_data_num < max_idx_num)
		return NULL;

	new = malloc(sizeof(*new));
	if (NULL == new)
		return NULL;

	memset(new, 0, sizeof(*new));
	new->max_idx_num  = max_idx_num;
	new->max_data_num = max_data_num;
	new->key_size     = key_size;
	new->value_size   = value_size;

	new->head = bp_create_inner_node(max_idx_num, key_size, compare);
	if (NULL == new->head) {
		free(new);

		return NULL;
	}

	new->data = bp_create_data_node(max_data_num, max_idx_num / 2,
									key_size, value_size, compare);
	if (NULL == new->data) {
		free(new->head);
		free(new);

		return NULL;
	}

	// 头结点的第一个指针指向第一个数据节点
	bp_inner_node_set_first_ptr((bp_inner_node_t *)new->head, new->data);

	return new;
}

/**
 * @brief 向B+树中插入一个被索引项及其位置信息
 *
 * @param tree B+树
 * @param key 被索引项
 * @param key_len 被索引项的长度
 * @param position 位置信息
 * @param position_len 位置信息的长度
 * @return int 成功返回 0 ，否则返回 -1
 */
int bp_insert(
	bp_tree_t     *tree,
	unsigned char *key,
	int            key_len,
	unsigned char *position,
	int            position_len)
{
	if (tree->key_size != key_len)
		return -1;

	if (tree->value_size != position_len)
		return -1;

	return 0;
}

int bp_node_get_key_num(bp_node_t *node)
{
	return ((bp_node_common_t *)node)->key_num;
}

unsigned char *bp_node_get_content(bp_node_t *node)
{
	if (BP_NODE_TYPE_DATA == node->type)
		return ((bp_data_node_t *)node)->content;
	else
		return ((bp_inner_node_t *)node)->content;
}
