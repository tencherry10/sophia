#ifndef SV_V_H_
#define SV_V_H_

/*
 * sophia database
 * sphia.org
 *
 * Copyright (c) Dmitry Simonenko
 * BSD License
*/

typedef struct svv svv;

struct svv {
	uint64_t lsn;
	uint32_t size;
	uint8_t  flags;
	uint16_t refs;
	void *log;
} sspacked;

extern svif sv_vif;

static inline char*
sv_vpointer(svv *v) {
	return (char*)(v) + sizeof(svv);
}

static inline uint32_t
sv_vsize(svv *v) {
	return sizeof(svv) + v->size;
}

static inline svv*
sv_vbuild(sr *r, sfv *keys, int count, char *data, int size)
{
	assert(r->scheme->count == count);
	int total = sf_size(r->fmt, keys, count, size);
	svv *v = ss_malloc(r->a, sizeof(svv) + total);
	if (ssunlikely(v == NULL))
		return NULL;
	v->size  = total;
	v->lsn   = 0;
	v->flags = 0;
	v->refs  = 1;
	v->log   = NULL;
	char *ptr = sv_vpointer(v);
	sf_write(r->fmt, ptr, keys, count, data, size);
	/* update runtime statistics */
	ss_spinlock(&r->stat->lock);
	r->stat->v_count++;
	r->stat->v_allocated += sizeof(svv) + total;
	ss_spinunlock(&r->stat->lock);
	return v;
}

static inline svv*
sv_vbuildraw(sr *r, char *src, int size)
{
	svv *v = ss_malloc(r->a, sizeof(svv) + size);
	if (ssunlikely(v == NULL))
		return NULL;
	v->size  = size;
	v->flags = 0;
	v->refs  = 1;
	v->lsn   = 0;
	v->log   = NULL;
	memcpy(sv_vpointer(v), src, size);
	/* update runtime statistics */
	ss_spinlock(&r->stat->lock);
	r->stat->v_count++;
	r->stat->v_allocated += sizeof(svv) + size;
	ss_spinunlock(&r->stat->lock);
	return v;
}

static inline svv*
sv_vdup(sr *r, sv *src)
{
	svv *v = sv_vbuildraw(r, sv_pointer(src), sv_size(src));
	if (ssunlikely(v == NULL))
		return NULL;
	v->flags = sv_flags(src);
	v->lsn   = sv_lsn(src);
	return v;
}

static inline void
sv_vref(svv *v) {
	v->refs++;
}

static inline int
sv_vunref(sr *r, svv *v)
{
	if (sslikely(--v->refs == 0)) {
		uint32_t size = sv_vsize(v);
		/* update runtime statistics */
		ss_spinlock(&r->stat->lock);
		assert(r->stat->v_count > 0);
		assert(r->stat->v_allocated >= size);
		r->stat->v_count--;
		r->stat->v_allocated -= size;
		ss_spinunlock(&r->stat->lock);
		ss_free(r->a, v);
		return 1;
	}
	return 0;
}

#endif
