#include "hashtable.h"

#include <stdlib.h>
#include <string.h>

#include "../errstack.h"

#define DENSITY_THRESHOLD_UP   (2.0)
#define DENSITY_THRESHOLD_DOWN (0.25)

static size_t _primes[] = {
    37,       79,       181,      359,      743,       1511,      3023,      6037,       12073,
    24001,    48017,    96001,    192007,   384001,    768013,    1536011,   3072001,    6144001,
    12288011, 24576001, 49152001, 98304053, 196608007, 393216007, 786432001, 1572864001, 3145728023,
};

typedef struct _node_s
{
	ht_st *owner;
	void *key;
	void *value;
	struct _node_s *next;
} _node_t;

struct ht_s
{
	size_t key_size;
	ht_alloc_func_t key_copy;
	ht_free_func_t key_free;

	size_t value_size;
	ht_alloc_func_t value_copy;
	ht_free_func_t value_free;

	ht_hash_func_t hash;
	ht_cmp_func_t cmp;
	size_t n_nodes;
	size_t buckets;
	_node_t **nodes;
};

int ht_alloc(ht_st **dst,
             ht_hash_func_t hash,
             ht_cmp_func_t cmp,
             size_t key_size,
             ht_alloc_func_t key_copy,
             ht_free_func_t key_free,
             size_t value_size,
             ht_alloc_func_t value_copy,
             ht_free_func_t value_free)
{
	CLEANUP(ht_free) ht_st *tmp;
	ES_ERR_ASRT_NM(dst);
	*dst = NULL;
	ES_ERR_ASRT_NM(hash && cmp);
	ES_ERR_ASRT_NM(tmp = calloc(1, sizeof(ht_st)));
	ES_ERR_ASRT_NM(tmp->nodes = calloc(_primes[0], sizeof(_node_t *)));
	tmp->hash       = hash;
	tmp->cmp        = cmp;
	tmp->key_size   = key_size;
	tmp->key_copy   = key_copy;
	tmp->key_free   = key_free;
	tmp->value_size = value_size;
	tmp->value_copy = value_copy;
	tmp->value_free = value_free;
	*dst            = tmp;
	tmp             = NULL;
	return 1;
}

void _cleanup_node_util(_node_t **tmp, bool skip_value)
{
	if (!*tmp)
		return;

	if ((*tmp)->owner->key_free)
		(*tmp)->owner->key_free((*tmp)->key);
	else if ((*tmp)->owner->key_size)
		free((*tmp)->key);

	if (!skip_value) {
		if ((*tmp)->owner->value_free)
			(*tmp)->owner->value_free((*tmp)->value);
		else if ((*tmp)->owner->value_size)
			free((*tmp)->value);
	}

	(*tmp)->owner->n_nodes--;
	free(*tmp);
}

void _cleanup_node(_node_t **tmp)
{
	_cleanup_node_util(tmp, false);
}

void ht_purge(ht_st *ht)
{
	size_t i;
	for (i = 0; i < ht_buckets(ht); i++) {
		_node_t *next = NULL;
		while (ht->nodes[i]) {
			next = ht->nodes[i]->next;
			_cleanup_node(&(ht->nodes[i]));
			ht->nodes[i] = next;
		}
	}
}

void ht_free(ht_st **to_free)
{
	if (to_free && *to_free) {
		ht_purge(*to_free);
		free(*to_free);
		*to_free = NULL;
	}
}

_node_t **_find_node(ht_st *ht, void *key)
{
	size_t hash    = ht->hash(key) % _primes[ht->buckets];
	_node_t **head = &(ht->nodes[hash]);
	while (*head && ht->cmp((*head)->key, key)) {
		head = &((*head)->next);
	}
	return head;
}

void _adjust_by_density(ht_st *ht)
{
	size_t new_bucks    = ht->buckets;
	_node_t **new_nodes = NULL;
	size_t i;

	if (new_bucks > 0 && ht_density(ht) <= DENSITY_THRESHOLD_DOWN) {
		new_bucks--;
	} else if (new_bucks < (ARRAY_SIZE(_primes) - 1) && ht_density(ht) >= DENSITY_THRESHOLD_UP) {
		new_bucks++;
	} else {
		return;
	}
	new_nodes = calloc(_primes[new_bucks], sizeof(*new_nodes));
	if (!new_nodes)
		return;
	for (i = 0; i < ht_buckets(ht); i++) {
		while (ht->nodes[i]) {
			size_t hash           = ht->hash(ht->nodes[i]->key) % _primes[new_bucks];
			_node_t *new_next     = new_nodes[hash];
			new_nodes[hash]       = ht->nodes[i];
			ht->nodes[i]          = ht->nodes[i]->next;
			new_nodes[hash]->next = new_next;
		}
	}
	free(ht->nodes);
	ht->nodes   = new_nodes;
	ht->buckets = new_bucks;
}

int _new_node(_node_t **cur_node, ht_st *ht, void *key, void *value)
{
	CLEANUP(_cleanup_node) _node_t *tmp = NULL;
	ES_ERR_ASRT_NM(tmp = calloc(1, sizeof(*tmp)));
	if (ht->key_copy) {
		ES_ERR_INT_NM(ht->key_copy(&tmp->key, key));
	} else if (ht->key_size) {
		ES_ERR_ASRT_NM(tmp->key = malloc(ht->key_size));
		memcpy(tmp->key, key, ht->key_size);
	} else {
		tmp->key = key;
	}

	if (ht->value_copy) {
		ES_PUSH_INT_NM(ht->value_copy(&tmp->value, value));
	} else if (ht->value_size) {
		ES_ERR_ASRT_NM(tmp->value = malloc(ht->value_size));
		memcpy(tmp->value, value, ht->value_size);
	} else {
		tmp->value = value;
	}

	_cleanup_node(cur_node);
	*cur_node = MOVE_PZ(tmp);
	ht->n_nodes++;
	return 1;
}

int ht_set(ht_st *ht, void *key, void *value)
{
	_node_t **head = NULL;
	int new_node   = false;
	ES_ERR_ASRT_NM(ht);
	head     = _find_node(ht, key);
	new_node = !*head;
	ES_ERR_INT_NM(_new_node(head, ht, key, value));
	if (new_node)
		_adjust_by_density(ht);
	return new_node;
}

void *ht_get(ht_st *ht, void *key)
{
	size_t hash;
	_node_t **head = NULL;
	hash           = ht->hash(key) % _primes[ht->buckets];
	head           = &(ht->nodes[hash]);
	while (*head && ht->cmp((*head)->key, key)) {
		head = &((*head)->next);
	}
	if (!*head)
		return NULL;
	return (*head)->value;
}

void *ht_take(ht_st *ht, void *key)
{
	_node_t **head    = _find_node(ht, key);
	_node_t *ret_node = *head;
	if (ret_node) {
		void *ret = ret_node->value;
		*head     = ret_node->next;
		_cleanup_node_util(head, true);
		_adjust_by_density(ht);
		return ret;
	}
	return NULL;
}

void ht_delete(ht_st *ht, void *key)
{
	_node_t **head = _find_node(ht, key);
	if (*head) {
		*head = (*head)->next;
		_cleanup_node(head);
		_adjust_by_density(ht);
	}
	return;
}

int ht_foreach(ht_st *ht, ht_foreach_func_t body)
{
	size_t i;

	ES_ERR_ASRT_NM(ht);
	for (i = 0; i < ht_buckets(ht); i++) {
		_node_t *head = ht->nodes[i];
		while (head) {
			int ret;
			ES_ERR_INT_NM(ret = body(ht, head->key, head->value));
			if (ret == 0)
				return 0;
			head = head->next;
		}
	}
	return 1;
}

size_t ht_buckets(ht_st *ht)
{
	return _primes[ht->buckets];
}

size_t ht_size(ht_st *ht)
{
	return ht->n_nodes;
}

double ht_density(ht_st *ht)
{
	return (double) ht->n_nodes / _primes[ht->buckets];
}

size_t ht_int_hash(void *key)
{
	uint64_t x = (uint64_t) key;
	x          = (x ^ (x >> 31) ^ (x >> 62)) * UINT64_C(0x319642b2d24d8ec3);
	x          = (x ^ (x >> 27) ^ (x >> 54)) * UINT64_C(0x96de1b173f119089);
	x          = x ^ (x >> 30) ^ (x >> 60);
	return x;
}

int64_t ht_int_cmp(void *key1, void *key2)
{
	return key2 - key1;
}

size_t ht_str_hash(const char *key)
{
	uint64_t hash = 5381UL;
	int c;

	while ((c = *key++))
		hash = ((hash << 5) + hash) + c;

	return hash;
}

int64_t ht_str_cmp(const char *key1, const char *key2)
{
	return strcmp(key1, key2);
}

int ht_str_copy(void **dst, void *key)
{
	if (key == NULL) {
		*dst = NULL;
		return 1;
	}
	char *new_key = strdup((char *) key);
	ES_ERR_ASRT_NM(new_key);
	*dst = new_key;
	return 1;
}

void ht_str_free(void *key)
{
	if (key)
		free(key);
}

size_t ht_arb_hash_util(void *key, size_t size)
{
	size_t hash   = 5381;
	uint8_t *data = key;
	for (; size != 0; size--, data++) {
		hash = ((hash << 5) + hash) ^ *data;
	}
	return hash;
}
int64_t ht_arb_cmp_util(void *key1, void *key2, size_t size)
{
	return memcmp(key1, key2, size);
}
