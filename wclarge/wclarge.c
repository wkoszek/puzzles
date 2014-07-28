/*
 * BSDv2
 * Copyright (c) 2014 Wojciech A. Koszek <wkoszek@FreeBSD.org>
 */
#define _BSD_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <ctype.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef __linux__
#include <getopt.h>
#endif

/*
 * WARN: All the structures and predefinitions should land in wclarge.h
 * probably, but left it in 1 file, since had no time to do the actual
 * cleanup. Same with trie_* API
 */

/*
 * Simple trie structure. This is pretty dump, as no optimization is done
 * whether word is there, or not. Better approach would be use a hash only
 * for letters which are there.
 */
typedef struct trie	trie_t;
struct trie {
	trie_t	*children[0xff];	// Children
	int	count;			// Number of occurences
	int	c;			// Actual character
};

/*
 * Mmaped() file structure
 */
struct mmap_file {
	int	size;	/* size of underlying file */
	char	*mem;	/* memory pointer */
	int	fd;	/* file descriptor of the opened file */
	int	offset;	/* for allocation, this is where we're in the memory */
};
typedef struct mmap_file mmap_file_t;

/*
 * Some debugging support
 */
static int	g_debug;
#define DBG(...)	do {			\
	if (!g_debug) {				\
		break;				\
	}					\
	printf("%s(%d) ", __func__, __LINE__);	\
	printf(__VA_ARGS__);			\
	printf("\n");				\
} while (0)

/*
 * This is a helper structure for threads--we hold start--end region for
 * data processing here.
 */
struct chunk {
	char	*mem;	/* Pointer to the memory */
	int	b;	/* Start pointer of the chunk */
	int	e;	/* End pointer of the chunk */
	int	is_last;	/* Is this the last chunk? */
} g_chunks[4];
typedef struct chunk chunk_t;

/*
 * Global data and predefinitions. Pasted output from cproto(1) below to
 * speedup development.
 */
pthread_mutex_t	g_out_mtx = PTHREAD_MUTEX_INITIALIZER;
static trie_t		g_trie;
static int		g_threads_num;
static pthread_t	g_thread[4];
static uint64_t		g_output_size;
static mmap_file_t	g_inf;
static mmap_file_t	g_outf;
/* wclarge.c */
static void adjust(int i, char *mem, struct chunk *chptr, int n);
void trie_init(trie_t *t);
void trie_add(trie_t *t, const char *str, int len, void *(*f)(size_t));
void trie_print(trie_t *t, int depth);
int mmap_file_open(mmap_file_t *mfptr, const char *fn, const char *flagstr, uint64_t wrsize);
int main(int argc, char **argv);

/*
 * Adjust the chunk regions based on whether the chunk end landed in the
 * middle of the word.
 */
static void
adjust(int i, char *mem, struct chunk *chptr, int n)
{
	int	ei;
	int	e_adj;

	DBG("in adjust()\n");
	ei = g_chunks[i].e;
	DBG("ei=%d\n", ei);
	DBG("char=%d(%c)\n", mem[ei], mem[ei]);

	e_adj = 0;
	while (!g_chunks[i].is_last && !isspace(mem[ei + e_adj])) {
		e_adj++;
	}
	e_adj++;
	DBG("e_adj=%d\n", e_adj);
	g_chunks[i].e += e_adj;
	if ((i + 1) < n) {
		g_chunks[i + 1].b += e_adj;
	}
}

/*
 * Main worder thread.
 */
static void *
word_worker(void *data)
{
	char	*bptr, *eptr;
	char	*wbptr, *weptr;
	chunk_t	*chunkptr = data;

	assert(chunkptr != NULL);
	bptr = chunkptr->mem + chunkptr->b;
	eptr = chunkptr->mem + chunkptr->e;

	wbptr = weptr = bptr;
	while (bptr < eptr) {
		/* 
		 * For each character we check if it's a space. If it is, we
		 * have the word. We lock the mutex and add it to is.
		 */
		if (!isspace(*weptr)) {
			weptr++;
		} else {
			pthread_mutex_lock(&g_out_mtx);
			trie_add(&g_trie, wbptr, weptr - wbptr, malloc);
			pthread_mutex_unlock(&g_out_mtx);
			wbptr = weptr + 1;
			weptr = wbptr;
		}
		bptr++;
	}
	return (NULL);
}

/*
 * WARN: Below is the trie_ API. It's fairly easy, so no explanations are provided
 */

void
trie_init(trie_t *t)
{
	int	i;

	assert(t != NULL);
	for (i = 0; i < 0xff; i++) {
		t->children[i] = NULL;
	}
	t->count = 0;
	t->c = -3;
}

void
trie_add(trie_t *t, const char *str, int len, void *(*trie_alloc)(size_t))
{
	trie_t	*curptr, *newptr;
	int	i, c;

	assert(t != NULL);
	assert(str != NULL);

	curptr = t;
	DBG("t=%p len=%d", (void *)t, len);
	for (i = 0; i < len; i++) {
		c = str[i];
		DBG("c=%c", c);
		if (curptr->children[c] != NULL) {
			curptr = curptr->children[c];
			DBG("seen, curptr=%p", (void *)curptr);
		} else {
			DBG("new char");
			newptr = trie_alloc(sizeof(trie_t));
			DBG("newptr=%p", (void *)newptr);
			assert(newptr != NULL);
			trie_init(newptr);
			newptr->c = c;
			curptr->children[c] = newptr;
			curptr = curptr->children[c];
			DBG("newptr->count=%d", newptr->count);
		}
		if (i == len - 1) {
			DBG("LAST!");
			curptr->count++;
			DBG("curptr->count=%d", curptr->count);
		}
	}
}

/*
 * XXX: Function would not be re-enterant because of the need of a buffer.
 * Buf for this program we print the trie only after we merged the flow from
 * N threads back to single-threaded mode, so it's OK.
 *
 * This is recursize function. Each time I go deeper in the trie, I push the
 * node's value (letter) to the __ugly_trie_print_buf. This way I can print
 * the word currently being processed.
 */
static int	__ugly_trie_print_buf[100];
void
trie_print(trie_t *t, int depth)
{
	int	*stackptr;
	int	i, has_child;

	assert(depth < 100);

	if (t == NULL) {
		return;
	}

	stackptr = __ugly_trie_print_buf;
	for (i = 0, has_child = 0; i < 0xff; i++) {
		if (t->children[i] != NULL) {
			stackptr[depth] = t->children[i]->c;
			trie_print(t->children[i], depth + 1);
			has_child++;
		}
	}
	if (!has_child || t->count > 0) {
		printf("      %d ", t->count);
		for (i = 0; i < depth; i++) {
			printf("%c%s", stackptr[i],
					(i == depth-1) ? "\n" : "");
		}
		stackptr[depth] = 0;
	}
}

/*
 * Open the file and mmap() it. Based on flagstr map for read ("r") or write
 * ("w") -- as if it was fopen(). For write, use wrsize to specify how big
 * the map must be.
 */
int
mmap_file_open(mmap_file_t *mfptr, const char *fn, const char *flagstr,
							uint64_t wrsize)
{
	void		*mem;
	struct stat	st;
	const char	*cp;
	int		fd, er, open_flags, mmap_flags, size;

	assert(mfptr != NULL);

	open_flags = 0;
	mmap_flags = 0;
	for (cp = flagstr; *cp != '\0'; cp++) {
		switch (*cp) {
		case 'r':
			open_flags |= O_RDONLY;
			mmap_flags |= PROT_READ;
			DBG("open_flags=%08x mmap_flags=%08x",
						open_flags, mmap_flags);
			break;
		case 'w':
			open_flags |= O_RDWR|O_TRUNC|O_CREAT;
			mmap_flags |= PROT_WRITE;
			DBG("open_flags=%08x mmap_flags=%08x",
						open_flags, mmap_flags);
			break;
		default:
			fprintf(stderr, "wrong mmap_file() flags\n");
			break;
		}
	}

	fd = open(fn, open_flags, 0755);
	if (fd == -1) {
		err(EXIT_FAILURE, "open()");
	}
	er = fstat(fd, &st);
	if (er == -1) {
		err(EXIT_FAILURE, "fstat()");
	}
	size = st.st_size;
	if (open_flags & O_RDWR) {
		size = wrsize;
		er = ftruncate(fd, size);
		if (er == -1) {
			err(EXIT_FAILURE, "ftruncate()");
		}
	}
	DBG("size=%d, mmap_flags=%08x, fd=%d",
			size, mmap_flags, fd);
	mem = mmap(NULL, size, mmap_flags, MAP_PRIVATE, fd, 0);
	if (mem == MAP_FAILED) {
		err(EXIT_FAILURE, "mmap()");
	}
	if (open_flags & O_RDWR) {
		memset(mem, 0, size);
	}
	mfptr->fd = fd;
	mfptr->size = size;
	mfptr->mem = mem;
	mfptr->offset = 0;
	return 0;
}

static void
mmap_file_close(mmap_file_t *mfptr)
{
	int	er;

	assert(mfptr != NULL);
	assert(mfptr->mem != NULL);
	assert(mfptr->size > 0);

	er = munmap(mfptr->mem, mfptr->size);
	assert(er == 0);
	er = close(mfptr->fd);
	DBG("close ret = %d, %s", er, strerror(errno));
	assert(er == 0);
}

/*
 * Simple allocator to pass to trie_add. This is when we want to allocate
 * the data from the backing memory.
 *
 * XX: There's no de-allocation here. We assume we're only counting the
 * words.
 */
void *
mmap_file_alloc(size_t sz)
{
	char	*ptr;

	assert(g_outf.fd > 0);
	assert(g_outf.offset < g_outf.size && "you must increate -o value");

	ptr = g_outf.mem;
	ptr += g_outf.offset;
	g_outf.offset += sz;
	return ((void *)ptr);
}

static void
usage(void)
{

	fprintf(stderr, "wclarge [-d] in_file\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	const char *g_infnstr;
	int	o, er, chunk_size, i;

	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	g_infnstr = NULL;
	g_threads_num = 1;
	g_debug = 0;
	g_output_size = 1024*1024;
	while ((o = getopt(argc, argv, "dn:o:")) != -1) {
		switch (o) {
		case 'd':
			g_debug++;
			break;
		case 'n':
			g_threads_num = atoi(optarg);
			break;
		case 'o':
			g_output_size = atoi(optarg);
			break;
		default:
			abort();
			break;
		}
	}
	argc -= optind;
	argv += optind;
	
	DBG("argc=%d g_debug=%d, g_threads_num=%d, g_output_size=%jd",
		argc, g_debug, g_threads_num, g_output_size);

	if (argc < 1) {
		usage();
	}
	g_infnstr = argv[0];
	DBG("g_infnstr=%s", g_infnstr);

	DBG("input file open");
	mmap_file_open(&g_inf, g_infnstr, "r", 0);
	mmap_file_open(&g_outf, "./_.map", "rw", g_output_size);

	chunk_size = g_inf.size / g_threads_num;
	for (i = 0; i < g_threads_num; i++) {
		g_chunks[i].mem = g_inf.mem;
		g_chunks[i].b = (i + 0)*chunk_size;
		g_chunks[i].e = (i + 1)*chunk_size;
		g_chunks[i].is_last = (i == g_threads_num - 1);
	}

	DBG("Before adjustment:");
	for (i = 0; i < g_threads_num; i++) {
		DBG("[%d] b=%d e=%d", i, g_chunks[i].b, g_chunks[i].e);
	}

	DBG("Adjusting");
	for (i = 0; i < g_threads_num; i++) {
		DBG("adjusting %d", i);
		adjust(i, g_inf.mem, g_chunks, g_threads_num);
	}
	for (i = 0; i < g_threads_num; i++) {
		DBG("[%d] b=%d e=%d", i, g_chunks[i].b, g_chunks[i].e);
	}
	for (i = 0; i < g_threads_num; i++) {
		DBG("[%d] b=%d e=%d", i, g_chunks[i].b, g_chunks[i].e);
		DBG("    >");
		for (int j = 0; j < 10; j++) {
			DBG("%c", g_inf.mem[g_chunks[i].b + j]);
		}
		DBG(" ");
	}


	trie_init(&g_trie);
	for (i = 0; i < g_threads_num; i++) {
		er = pthread_create(&g_thread[i], NULL, word_worker, &(g_chunks[i]));
		DBG("[%d] create er = %d", i, er);
	}
	for (i = 0; i < g_threads_num; i++) {
		er = pthread_join(g_thread[i], NULL);
		DBG("[%d] join er = %d", i, er);
	}

	DBG("-");
	mmap_file_close(&g_inf);
	DBG("----------------------");

	DBG("fd=%d", g_outf.fd);
	mmap_file_close(&g_outf);

	trie_print(&g_trie, 0);
	return 0;
}
