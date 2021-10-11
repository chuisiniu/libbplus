#include <gtest/gtest.h>

extern "C" {
	#include "libbplus.h"

	extern int bp_bi_search_one(
		unsigned char *content,
		int            content_len,
		int            item_size,
		unsigned char *target,
		int            target_size,
		int            offset,
		bp_compare_f   compare,
		int           *found_idx);

	extern int bp_bi_search_first(
		unsigned char *content,
		int            content_len,
		int            item_size,
		unsigned char *target,
		int            target_size,
		int            offset,
		bp_compare_f   compare,
		int           *found_idx);

	extern int bp_bi_search_last(
		unsigned char *content,
		int            content_len,
		int            item_size,
		unsigned char *target,
		int            target_size,
		int            offset,
		bp_compare_f   compare,
		int           *found_idx);

	extern bp_node_t *bp_create_inner_node(int max_key_num, int key_size, bp_compare_f compare);

	extern bp_node_t *bp_create_data_node(
		int          max_kv_num,
		int          min_kv_num,
		int          key_size,
		int          value_size,
		bp_compare_f compare);

	extern int bp_data_node_insert_data(
		bp_node_t      *node,
		unsigned char  *key,
		unsigned char  *position,
		bp_node_t     **pp_new);

	extern int bp_calc_data_node_content_len(
		int max_kv_num,
		int key_size,
		int value_size);

	extern int bp_data_node_split(
		bp_node_t  *old,
		bp_node_t **p_new);

	extern int bp_inner_node_insert_data(
		bp_node_t      *node,
		unsigned char  *key,
		unsigned char  *position,
		bp_node_t     **pp_new);

	extern int bp_node_get_key_num(bp_node_t *node);
	extern unsigned char *bp_node_get_content(bp_node_t *node);
}

TEST(BiSearch, FindOne)
{
	unsigned char  content[] = "112233445566778899";
	int            idx;
	unsigned char *target;

	target = (unsigned char *)"3";
	EXPECT_EQ(1, bp_bi_search_one(content, sizeof(content) - 1, 2,
								  target, 1, 1, NULL, &idx));
	EXPECT_EQ(idx, 2);

	target = (unsigned char *)"11";
	EXPECT_EQ(1, bp_bi_search_one(content, sizeof(content) - 1, 2,
								  target, 2, 0, NULL, &idx));
	EXPECT_EQ(idx, 0);

	target = (unsigned char *)"12";
	EXPECT_EQ(0, bp_bi_search_one(content, sizeof(content) - 1, 2,
								  target, 2, 0, NULL, &idx));
	EXPECT_EQ(idx, 1);

	target = (unsigned char *)"11";
	EXPECT_EQ(1, bp_bi_search_one(content, 2, 2, target, 2, 0, NULL, &idx));
	EXPECT_EQ(idx, 0);

	target = (unsigned char *)"12";
	EXPECT_EQ(0, bp_bi_search_one(content, 2, 2, target, 2, 0, NULL, &idx));
	EXPECT_EQ(idx, 1);

	target = (unsigned char *)"12";
	EXPECT_EQ(0, bp_bi_search_one(content, 0, 2, target, 2, 0, NULL, &idx));
	EXPECT_EQ(idx, 0);

	target = (unsigned char *)"aa";
	EXPECT_EQ(0, bp_bi_search_one(content, sizeof(content) - 1, 2,
								  target, 2, 0, NULL, &idx));
	EXPECT_EQ(idx, 9);
}

TEST(BiSearch, FindFirst)
{
	unsigned char  content[] = "111133335555777799";
	int            idx;
	unsigned char *target;

	target = (unsigned char *)"1";
	EXPECT_EQ(1, bp_bi_search_first(content, sizeof(content) - 1, 2,
									target, 1, 1, NULL, &idx));
	EXPECT_EQ(idx, 0);

	target = (unsigned char *)"3";
	EXPECT_EQ(1, bp_bi_search_first(content, sizeof(content) - 1, 2,
									target, 1, 1, NULL, &idx));
	EXPECT_EQ(idx, 2);

	target = (unsigned char *)"5";
	EXPECT_EQ(1, bp_bi_search_first(content, sizeof(content) - 1, 2,
									target, 1, 1, NULL, &idx));
	EXPECT_EQ(idx, 4);

	target = (unsigned char *)"7";
	EXPECT_EQ(1, bp_bi_search_first(content, sizeof(content) - 1, 2,
									target, 1, 1, NULL, &idx));
	EXPECT_EQ(idx, 6);

	target = (unsigned char *)"9";
	EXPECT_EQ(1, bp_bi_search_first(content, sizeof(content) - 1, 2,
									target, 1, 1, NULL, &idx));
	EXPECT_EQ(idx, 8);

	target = (unsigned char *)"0";
	EXPECT_EQ(0, bp_bi_search_first(content, sizeof(content) - 1, 2,
									target, 1, 1, NULL, &idx));
	EXPECT_EQ(idx, 0);

	target = (unsigned char *)"a";
	EXPECT_EQ(0, bp_bi_search_first(content, sizeof(content) - 1, 2,
									target, 1, 1, NULL, &idx));
	EXPECT_EQ(idx, 9);
}

TEST(BiSearch, FindLast)
{
	unsigned char  content[] = "111133335555777799";
	int            idx;
	unsigned char *target;

	target = (unsigned char *)"1";
	EXPECT_EQ(1, bp_bi_search_last(content, sizeof(content) - 1, 2,
								   target, 1, 1, NULL, &idx));
	EXPECT_EQ(idx, 1);

	target = (unsigned char *)"3";
	EXPECT_EQ(1, bp_bi_search_last(content, sizeof(content) - 1, 2,
								   target, 1, 1, NULL, &idx));
	EXPECT_EQ(idx, 3);

	target = (unsigned char *)"5";
	EXPECT_EQ(1, bp_bi_search_last(content, sizeof(content) - 1, 2,
								   target, 1, 1, NULL, &idx));
	EXPECT_EQ(idx, 5);

	target = (unsigned char *)"7";
	EXPECT_EQ(1, bp_bi_search_last(content, sizeof(content) - 1, 2,
								   target, 1, 1, NULL, &idx));
	EXPECT_EQ(idx, 7);

	target = (unsigned char *)"9";
	EXPECT_EQ(1, bp_bi_search_last(content, sizeof(content) - 1, 2,
								   target, 1, 1, NULL, &idx));
	EXPECT_EQ(idx, 8);

	target = (unsigned char *)"0";
	EXPECT_EQ(0, bp_bi_search_last(content, sizeof(content) - 1, 2,
								   target, 1, 1, NULL, &idx));
	EXPECT_EQ(idx, 0);

	target = (unsigned char *)"a";
	EXPECT_EQ(0, bp_bi_search_last(content, sizeof(content) - 1, 2,
								   target, 1, 1, NULL, &idx));
	EXPECT_EQ(idx, 9);
}

TEST(LeafNode, Insert)
{
	bp_node_t     *data;
	bp_node_t     *split;
	unsigned char  k;
	unsigned char  p;
	unsigned char *content;

	data    = bp_create_data_node(6, 3, 1, 1, NULL);
	content = bp_node_get_content(data);

	k = '1';
	p = '1';
	EXPECT_EQ(0, bp_data_node_insert_data(data, &k, &p, &split));
	EXPECT_EQ(1, bp_node_get_key_num(data));
	EXPECT_EQ(0, strncmp("11", (char *)content, 2));
	EXPECT_TRUE(split == NULL);

	k = '3';
	p = '5';
	EXPECT_EQ(0, bp_data_node_insert_data(data, &k, &p, &split));
	EXPECT_EQ(2, bp_node_get_key_num(data));
	EXPECT_EQ(0, strncmp("1135", (char *)content, 4));
	EXPECT_TRUE(split == NULL);

	k = '2';
	p = '3';
	EXPECT_EQ(0, bp_data_node_insert_data(data, &k, &p, &split));
	EXPECT_EQ(3, bp_node_get_key_num(data));
	EXPECT_EQ(0, strncmp("112335", (char *)content, 6));
	EXPECT_TRUE(split == NULL);

	k = '2';
	p = '4';
	EXPECT_EQ(0, bp_data_node_insert_data(data, &k, &p, &split));
	EXPECT_EQ(4, bp_node_get_key_num(data));
	EXPECT_EQ(0, strncmp("11232435", (char *)content, 8));
	EXPECT_TRUE(split == NULL);

	k = '3';
	p = '6';
	EXPECT_EQ(0, bp_data_node_insert_data(data, &k, &p, &split));
	EXPECT_EQ(5, bp_node_get_key_num(data));
	EXPECT_EQ(0, strncmp("1123243536", (char *)content, 10));
	EXPECT_TRUE(split == NULL);

	k = '1';
	p = '2';
	EXPECT_EQ(0, bp_data_node_insert_data(data, &k, &p, &split));
	EXPECT_EQ(6, bp_node_get_key_num(data));
	EXPECT_EQ(0, strncmp("111223243536", (char *)content, 12));
	EXPECT_TRUE(split == NULL);

	free(data);
}

TEST(LeafNode, Split)
{
	bp_node_t     *data;
	//bp_node_t     *data1;
	bp_node_t     *split;
	int            k;
	int            p;
	unsigned char *content;
	//unsigned char *content1;

	data = bp_create_data_node(6, 3, sizeof(k), sizeof(p), NULL);
	content = bp_node_get_content(data);

	k = 1;
	p = 1;
	EXPECT_EQ(0, bp_data_node_insert_data(data, (unsigned char *)&k,
										  (unsigned char *)&p, &split));
	EXPECT_TRUE(split == NULL);

	k = 3;
	p = 5;
	EXPECT_EQ(0, bp_data_node_insert_data(data, (unsigned char *)&k,
										  (unsigned char *)&p, &split));
	EXPECT_TRUE(split == NULL);

	k = 2;
	p = 3;
	EXPECT_EQ(0, bp_data_node_insert_data(data, (unsigned char *)&k,
										  (unsigned char *)&p, &split));
	EXPECT_TRUE(split == NULL);

	k = 2;
	p = 4;
	EXPECT_EQ(0, bp_data_node_insert_data(data, (unsigned char *)&k,
										  (unsigned char *)&p, &split));
	EXPECT_TRUE(split == NULL);

	k = 3;
	p = 6;
	EXPECT_EQ(0, bp_data_node_insert_data(data, (unsigned char *)&k,
										  (unsigned char *)&p, &split));
	EXPECT_TRUE(split == NULL);

	k = 1;
	p = 2;
	EXPECT_EQ(0, bp_data_node_insert_data(data, (unsigned char *)&k,
										  (unsigned char *)&p, &split));
	EXPECT_TRUE(split == NULL);
	EXPECT_EQ(6, bp_node_get_key_num(data));
	EXPECT_EQ(1, *((int *)content));
	EXPECT_EQ(1, *(((int *)content) + 1));
	EXPECT_EQ(1, *(((int *)content) + 2));
	EXPECT_EQ(2, *(((int *)content) + 3));
	EXPECT_EQ(2, *(((int *)content) + 4));
	EXPECT_EQ(3, *(((int *)content) + 5));
	EXPECT_EQ(2, *(((int *)content) + 6));
	EXPECT_EQ(4, *(((int *)content) + 7));
	EXPECT_EQ(3, *(((int *)content) + 8));
	EXPECT_EQ(5, *(((int *)content) + 9));
	EXPECT_EQ(3, *(((int *)content) + 10));
	EXPECT_EQ(6, *(((int *)content) + 11));
}

TEST(InnerNode, Insert)
{
	bp_node_t     *head;
	bp_node_t     *split_head;
	bp_node_t     *data;
	bp_node_t     *split_data;
	unsigned char *head_content;
	unsigned char *data_content;
	unsigned char *split_content;

	head = bp_create_inner_node(4, 2, NULL);
	data = bp_create_data_node(4, 2, 2, 2, NULL);

	head_content = bp_node_get_content(head);
	data_content = bp_node_get_content(data);

	memcpy(head_content, &data, sizeof(data));

	EXPECT_EQ(0,
		bp_inner_node_insert_data(head, (unsigned char *)"55",
			(unsigned char *)"55", &split_head));
	EXPECT_EQ(0, memcmp(head_content, &data, sizeof(data)));
	EXPECT_EQ(0, strncmp((char *)head_content + sizeof(data), "55", 2));
	EXPECT_EQ(0, strncmp((char *)data_content, "5555", 4));
	EXPECT_TRUE(split_head == NULL);

	EXPECT_EQ(0,
		bp_inner_node_insert_data(head, (unsigned char *)"88",
			(unsigned char *)"88", &split_head));
	EXPECT_EQ(0, memcmp(head_content, &data, sizeof(data)));
	EXPECT_EQ(0, strncmp((char *)head_content + sizeof(data), "88", 2));
	EXPECT_EQ(0, strncmp((char *)data_content, "55558888", 8));
	EXPECT_TRUE(split_head == NULL);

	EXPECT_EQ(0,
		bp_inner_node_insert_data(head, (unsigned char *)"11",
			(unsigned char *)"11", &split_head));
	EXPECT_EQ(0, memcmp(head_content, &data, sizeof(data)));
	EXPECT_EQ(0, strncmp((char *)head_content + sizeof(data), "88", 2));
	EXPECT_EQ(0, strncmp((char *)data_content, "111155558888", 12));
	EXPECT_TRUE(split_head == NULL);

	EXPECT_EQ(0,
		bp_inner_node_insert_data(head, (unsigned char *)"22",
			(unsigned char *)"22", &split_head));
	EXPECT_EQ(0, memcmp(head_content, &data, sizeof(data)));
	EXPECT_EQ(0, strncmp((char *)head_content + sizeof(data), "88", 2));
	EXPECT_EQ(0, strncmp((char *)data_content, "1111222255558888", 16));
	EXPECT_TRUE(split_head == NULL);

	EXPECT_EQ(0,
		bp_inner_node_insert_data(head, (unsigned char *)"33",
			(unsigned char *)"33", &split_head));
	EXPECT_EQ(2, bp_node_get_key_num(head));
	EXPECT_EQ(0, memcmp(head_content, &data, sizeof(data)));
	EXPECT_EQ(0, strncmp((char *)head_content + sizeof(data), "22", 2));
	split_data    = *((bp_node_t **)(head_content + sizeof(bp_node_t *) + 2));
	split_content = bp_node_get_content(split_data);
	EXPECT_EQ(2, bp_node_get_key_num(data));
	EXPECT_EQ(3, bp_node_get_key_num(split_data));
	EXPECT_EQ(0, strncmp((char *)head_content + sizeof(data) + 2 + sizeof(data),
		"88", 2));
	EXPECT_EQ(0, strncmp((char *)data_content, "11112222", 8));
	EXPECT_EQ(0, strncmp((char *)split_content, "333355558888", 12));
	EXPECT_TRUE(split_head == NULL);

	EXPECT_EQ(0,
		bp_inner_node_insert_data(head, (unsigned char *)"12",
			(unsigned char *)"12", &split_head));
	EXPECT_EQ(2, bp_node_get_key_num(head));
	EXPECT_EQ(0, memcmp(head_content, &data, sizeof(data)));
	EXPECT_EQ(0, strncmp((char *)head_content + sizeof(data), "22", 2));
	EXPECT_EQ(0, memcmp(head_content + sizeof(data) + 2, &split_data, sizeof(split_data)));
	EXPECT_EQ(3, bp_node_get_key_num(data));
	EXPECT_EQ(3, bp_node_get_key_num(split_data));
	EXPECT_EQ(0, strncmp((char *)head_content + sizeof(data) + 2 + sizeof(data),
		"88", 2));
	EXPECT_EQ(0, strncmp((char *)data_content, "111112122222", 12));
	EXPECT_EQ(0, strncmp((char *)split_content, "333355558888", 12));
	EXPECT_TRUE(split_head == NULL);
}
