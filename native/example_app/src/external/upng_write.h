#ifndef UPNG_WRITE_INCLUDED
#define UPNG_WRITE_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct upng_buffer {
	void *data;
	size_t size;
} upng_buffer;

upng_buffer upng_write_memory(const void *data, uint32_t width, uint32_t height);
bool upng_write_file(const char *filename, const void *data, uint32_t width, uint32_t height);
bool upng_write_file_len(const char *filename, size_t filename_len, const void *data, uint32_t width, uint32_t height);

#endif

#if defined(UPNG_WRITE_IMPLEMENTATION)
#ifndef UPNG_WRITE_IMPLEMENTED
#define UPNG_WRITE_IMPLEMENTED

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define upngi_assert(cond) assert(cond)

#define upngi_arraycount(arr) (sizeof(arr) / sizeof(*(arr)))


#if defined(_MSC_VER)
	#define upngi_forceinline inline __forceinline
#elif defined(__GNUC__)
	#define upngi_forceinline inline __attribute__((always_inline))
#else
	#define upngi_forceinline inline
#endif

typedef struct {
	uint16_t length, bits;
} upngi_huff_symbol;

typedef struct {
	uint16_t index; uint16_t parent; uint32_t count;
} upngi_huff_node;

static int upngi_cmp_huff_node(const void *va, const void *vb) {
	const upngi_huff_node a = *(const upngi_huff_node*)va, b = *(const upngi_huff_node*)vb;
	if (a.count != b.count) return a.count < b.count ? -1 : 1;
	if (a.index != b.index) return a.index < b.index ? -1 : 1;
	return 0;
}

static uint32_t upngi_bit_reverse(uint32_t mask, uint32_t num_bits)
{
	upngi_assert(num_bits <= 16);
	uint32_t x = mask;
	x = (((x & 0xaaaa) >> 1) | ((x & 0x5555) << 1));
	x = (((x & 0xcccc) >> 2) | ((x & 0x3333) << 2));
	x = (((x & 0xf0f0) >> 4) | ((x & 0x0f0f) << 4));
	x = (((x & 0xff00) >> 8) | ((x & 0x00ff) << 8));
	return x >> (16 - num_bits);
}

void upngi_build_huffman(upngi_huff_symbol *symbols, uint32_t *counts, uint32_t num_symbols, uint32_t max_bits)
{
	upngi_huff_node nodes[1024];

	uint32_t bias = 0;
	for (;;) {
		for (uint32_t i = 0; i < num_symbols; i++) {
			upngi_huff_node node = { (uint16_t)i, UINT16_MAX, counts[i] ? counts[i] + bias : 0 };
			nodes[i] = node;
		}
		qsort(nodes, num_symbols, sizeof(upngi_huff_node), &upngi_cmp_huff_node);
		uint32_t cs = 0, ce = num_symbols, qs = 512, qe = qs;
		while (cs + 2 < ce && nodes[cs].count == 0) cs++;
		while (ce-cs + qe-qs > 1) {
			uint32_t a = ce-cs > 0 && (qe-qs == 0 || nodes[cs].count < nodes[qs].count) ? cs++ : qs++;
			uint32_t b = ce-cs > 0 && (qe-qs == 0 || nodes[cs].count < nodes[qs].count) ? cs++ : qs++;
			upngi_huff_node node = { UINT16_MAX, UINT16_MAX, nodes[a].count + nodes[b].count };
			nodes[a].parent = nodes[b].parent = (uint16_t)qe;
			nodes[qe++] = node;
		}

		bool fail = false;
		uint32_t length_counts[16] = { 0 }, length_codes[16] = { 0 };
		for (uint32_t i = 0; i < num_symbols; i++) {
			uint32_t len = 0;
			for (uint32_t a = nodes[i].parent; a != UINT16_MAX; a = nodes[a].parent) len++;
			if (len > max_bits) { fail = true; break; }
			length_counts[len]++;
			symbols[nodes[i].index].length = (uint16_t)len;
		}
		if (fail) {
			bias = bias ? bias << 1 : 1;
			continue;
		}

		uint32_t code = 0, prev_count = 0;
		for (uint32_t bits = 1; bits < 16; bits++) {
			uint32_t count = length_counts[bits];
			code = (code + prev_count) << 1;
			prev_count = count;
			length_codes[bits] = code;
		}

		for (uint32_t i = 0; i < num_symbols; i++) {
			uint32_t len = symbols[i].length;
			symbols[i].bits = len ? upngi_bit_reverse(length_codes[len]++, len) : 0;
		}

		break;
	}
}

static uint32_t upngi_match_len(const unsigned char *a, const unsigned char *b, uint32_t max_length)
{
	if (max_length > 258) max_length = 258;
	for (uint32_t len = 0; len < max_length; len++) {
		if (*a++ != *b++) return len;
	}
	return max_length;
}

static uint16_t upngi_encode_lit(uint32_t value, uint32_t bits)
{
	return (uint16_t)(value | (0xfffeu << bits));
}

static uint32_t upngi_sym_offset_bits(upngi_huff_symbol *syms, uint32_t code, uint32_t base, const uint16_t *counts, uint32_t value)
{
	for (uint32_t bits = 0; counts[bits]; bits++) {
		uint32_t num = counts[bits] << bits, delta = value - base;
		if (delta < num) {
			return syms[code + (delta >> bits)].length + bits;
		}
		code += counts[bits];
		base += num;
	}
	return syms[code].bits;
}

static size_t upngi_encode_sym_offset(uint16_t *dst, uint16_t code, uint32_t base, const uint16_t *counts, uint32_t value)
{
	for (uint32_t bits = 0; counts[bits]; bits++) {
		uint32_t num = counts[bits] << bits, delta = value - base;
		if (delta < num) {
			dst[0] = code + (delta >> bits);
			if (bits > 0) {
				dst[1] = upngi_encode_lit(delta & ((1 << bits) - 1), bits);
				return 2;
			} else {
				return 1;
			}
		}
		code += counts[bits];
		base += num;
	}
	dst[0] = code;
	return 1;
}

static uint32_t upngi_lz_hash(const unsigned char *d)
{
	uint32_t x = (uint32_t)d[0] | (uint32_t)d[1] << 8 | (uint32_t)d[2] << 16;
	x ^= x >> 16;
	x *= UINT32_C(0x7feb352d);
	x ^= x >> 15;
	x *= UINT32_C(0x846ca68b);
	x ^= x >> 16;
	return x ? x : 1;
}

typedef struct {
	uint32_t hash;
	uint32_t offset;
} upngi_match;

static bool upngi_init_tri_dist(uint16_t *tri_dist, const void *data, uint32_t length)
{
	const uint32_t max_scan = 32;
	const unsigned char *d = (const unsigned char*)data;
	uint32_t mask = 0x1ffff;

	upngi_match *match_table = (upngi_match*)malloc(sizeof(upngi_match) * (mask + 1));
	if (!match_table) return false;
	for (uint32_t i = 0; i <= mask; i++) {
		match_table[i].hash = 0;
		match_table[i].offset = UINT32_C(0x80000000);
	}

	for (uint32_t i = 0; i < length; i++) {
		if (length - i >= 3) {
			uint32_t hash = upngi_lz_hash(d + i), replace_ix = 0, replace_score = 0;
			for (uint32_t scan = 0; scan < max_scan; scan++) {
				uint32_t ix = (hash + scan) & mask;
				uint32_t delta = (uint32_t)(i - match_table[ix].offset);
				if (match_table[ix].hash == hash && delta <= 32768) {
					tri_dist[i] = (uint16_t)delta;
					match_table[ix].offset = i;
					replace_ix = UINT32_MAX;
					break;
				} else {
					uint32_t score = delta <= 32768 ? delta : 32768 + max_scan - scan;
					if (score > replace_score) {
						replace_score = score;
						replace_ix = ix;
						if (score > 32768) break;
					}
				}
			}
			if (replace_ix != UINT32_MAX) {
				match_table[replace_ix].hash = hash;
				match_table[replace_ix].offset = i;
				tri_dist[i] = UINT16_MAX;
			}
		} else {
			tri_dist[i] = UINT16_MAX;
		}
	}

	free(match_table);
	return true;
}

static upngi_forceinline bool upngi_cmp3(const void *a, const void *b)
{
	const char *ca = (const char*)a, *cb = (const char*)b;
	return ca[0] == cb[0] && ca[1] == cb[1] && ca[2] == cb[2];
}

typedef struct {
	uint32_t len, dist, bits;
} upngi_lz_match;

static const uint16_t upngi_len_counts[] = { 8, 4, 4, 4, 4, 4, 0 };
static const uint16_t upngi_dist_counts[] = { 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0 };

static upngi_lz_match find_match(upngi_huff_symbol *syms, const uint16_t *tri_dist, const void *data, uint32_t begin, uint32_t end)
{
	const unsigned char *d = (const unsigned char*)data;
	int32_t max_len = (int32_t)end - begin - 1;
	if (max_len > 258) max_len = 258;
	if (max_len < 3) {
		upngi_lz_match no_match = { 0 };
		return no_match;
	}
	uint32_t best_len = 0, best_dist = 0;
	int32_t m_off = begin - tri_dist[begin], m_end = m_off, m_delta = 0;
	uint32_t best_bits = syms[d[begin + 0]].length + syms[d[begin + 1]].length;
	for (size_t scan = 0; scan < 1000; scan++) {
		if (begin - m_off > 32768 || m_off < 0 || m_end < 0 || m_delta > max_len - 3) break;
		int32_t delta = m_end - m_off - 3;

		bool ok = true;
		if (m_off >= 0 && (m_end - m_off < m_delta || !upngi_cmp3(d + m_off + m_delta, d + begin + m_delta))) {
			m_off -= tri_dist[m_off];
			if (m_off >= 0 && m_end - m_off > m_delta && upngi_cmp3(d + m_off + m_delta, d + begin + m_delta)) {
				m_end = m_off + m_delta;
			}
			ok = false;
		}
		if (m_end >= m_delta && (m_end - m_off < m_delta || !upngi_cmp3(d + m_end - m_delta, d + begin))) {
			m_end -= tri_dist[m_end];
			if (m_end >= m_delta && m_end - m_off < m_delta && upngi_cmp3(d + m_end - m_delta, d + begin)) {
				m_off = m_end - m_delta;
			}
			ok = false;
		}
		if (!ok) continue;

		if (m_delta <= 3 || !memcmp(d + begin, d + m_off, m_delta + 3)) {
			do {
				uint32_t m_len = m_delta + 3, m_dist = begin - m_off;
				uint32_t m_bits = upngi_sym_offset_bits(syms, 257, 3, upngi_len_counts, m_len)
					+ upngi_sym_offset_bits(syms, 286, 1, upngi_dist_counts, m_dist);

				best_bits += syms[d[begin + m_len - 1]].length;
				if (m_bits < best_bits) {
					best_bits = m_bits;
					best_len = m_len;
					best_dist = m_dist;
				}

				m_end++;
				m_delta++;
			} while (m_delta <= max_len - 3 && d[begin + m_delta + 2] == d[m_off + m_delta + 2]);
		} else {
			m_off--;
		}
	}

	upngi_lz_match match = { best_len, best_dist, best_bits };
	return match;
}

static uint32_t upngi_encode_lz(upngi_huff_symbol *syms, uint16_t *dst, const uint16_t *tri_dist, const void *data, uint32_t begin, uint32_t end, uint32_t *p_bits)
{
	const unsigned char *d = (const unsigned char*)data;
	uint16_t *p = dst;
	uint32_t bits = 0;
	for (int32_t i = (int32_t)begin; i < (int32_t)end; i++) {

		upngi_lz_match match = find_match(syms, tri_dist, data, i, end);
		while (match.len > 0) {
			upngi_lz_match next = find_match(syms, tri_dist, data, i + 1, end);
			if (next.len == 0) break;

			uint32_t match_bits = match.bits, next_bits = next.bits + syms[d[i]].length;
			for (uint32_t j = i + match.len; j < i + 1 + next.len; j++) match_bits += syms[d[j]].length;
			for (uint32_t j = i + 1 + next.len; j < i + match.len; j++) next_bits += syms[d[j]].length;
			if (next_bits >= match_bits) break;

			bits += syms[d[i]].length;
			*p++ = d[i++];
			match = next;
		}

		if (match.len > 0) {
			upngi_assert(!memcmp(d + i, d + i - match.dist, match.len));
			p += upngi_encode_sym_offset(p, 257, 3, upngi_len_counts, match.len);
			p += upngi_encode_sym_offset(p, 286, 1, upngi_dist_counts, match.dist);
			i += match.len - 1;
		} else {
			*p++ = d[i];
		}
	}
	return (uint32_t)(p - dst);
}

static size_t upngi_encode_lengths(uint16_t *dst, upngi_huff_symbol *syms, uint32_t *p_count, uint32_t min_count)
{
	uint32_t count = *p_count;
	while (count > min_count && syms[count - 1].length == 0) count--;
	*p_count = count;

	uint16_t *p = dst;
	for (uint32_t begin = 0; begin < count; ) {
		uint16_t len = syms[begin].length;
		uint32_t end = begin + 1;
		while (end < count && syms[end].length == len) {
			end++;
		}

		uint32_t span_begin = begin;
		while (begin < end) {
			uint32_t num = end - begin;
			if (num < 3 || (len > 0 && begin == span_begin)) {
				num = 1;
				*p++ = len;
			} else if (len == 0 && end - begin < 11) {
				*p++ = 17;
				*p++ = upngi_encode_lit(num - 3, 3);
			} else if (len == 0) {
				if (num > 138) num = 138;
				*p++ = 18;
				*p++ = upngi_encode_lit(num - 11, 7);
			} else { 
				if (num > 6) num = 6;
				*p++ = 16;
				*p++ = upngi_encode_lit(num - 3, 2);
			}
			begin += num;
		}
	}
	return (size_t)(p - dst);
}

static size_t upngi_flush_bits(char **p_dst, size_t reg, size_t num)
{
	char *dst = *p_dst;
	for (; num >= 8; num -= 8) {
		*dst++ = (uint8_t)reg;
		reg >>= 8;
	}
	*p_dst = dst;
	return reg;
}

static upngi_forceinline void upngi_push_bits(char **p_dst, size_t *p_reg, size_t *p_num, uint32_t value, uint32_t bits)
{
	if (*p_num + bits > sizeof(size_t) * 8) {
		*p_reg = upngi_flush_bits(p_dst, *p_reg, *p_num);
		*p_num &= 0x7;
	}
	*p_reg |= (size_t)value << *p_num;
	*p_num += bits;
}

static void upngi_encode_syms(char **p_dst, size_t *p_reg, size_t *p_num, upngi_huff_symbol *table, const uint16_t *syms, size_t num_syms)
{
	size_t reg = *p_reg, num = *p_num;
	for (size_t i = 0; i < num_syms; i++) {
		uint32_t sym = syms[i];
		if (sym & 0x8000) {
			// TODO: BSR?
			uint32_t bits = 15;
			while (sym & (1 << bits)) bits--;
			upngi_push_bits(p_dst, &reg, &num, sym & ((1 << bits) - 1), bits);
		} else {
			upngi_huff_symbol hs = table[sym];
			upngi_push_bits(p_dst, &reg, &num, hs.bits, hs.length);
		}
	}
	*p_reg = reg;
	*p_num = num;
}

static uint32_t upngi_adler32(const void *data, size_t size)
{
	size_t a = 1, b = 0;
	const char *p = (const char*)data;
	const size_t num_before_wrap = sizeof(size_t) == 8 ? 380368439u : 5552u;
	size_t size_left = size;
	while (size_left > 0) {
		size_t num = size_left <= num_before_wrap ? size_left : num_before_wrap;
		size_left -= num;
		const char *end = p + num;
		while (p != end) {
			a += (size_t)(uint8_t)*p++; b += a;
		}
		a %= 65521u;
		b %= 65521u;
	}
	return (uint32_t)((b << 16) | (a & 0xffff));
}

static size_t upngi_deflate(void *dst, const void *data, size_t length)
{
	if (!dst) return 0;
	char *dp = (char*)dst;

	static const uint32_t lz_block_size = 8*1024*1024;
	static const uint32_t huff_block_size = 64*1024;

	size_t num_sym = huff_block_size < length ? huff_block_size : length;
	size_t num_tri = lz_block_size < length ? lz_block_size : length;
	uint16_t *sym_buf = (uint16_t*)calloc(sizeof(uint16_t), num_sym);
	uint16_t *tri_buf = (uint16_t*)calloc(sizeof(uint16_t), num_tri);
	if (!sym_buf || !tri_buf) {
		free(sym_buf);
		free(tri_buf);
		return 0;
	}

	size_t reg = 0, num = 0;

	upngi_push_bits(&dp, &reg, &num, 0x78, 8);
	upngi_push_bits(&dp, &reg, &num, 0x9c, 8);

	for (size_t lz_base = 0; lz_base < length; lz_base += lz_block_size) {
		size_t lz_length = length - lz_base;
		if (lz_length > lz_block_size) lz_length = lz_block_size;

		upngi_init_tri_dist(tri_buf, (const char*)data + lz_base, (uint32_t)lz_length);

		for (size_t huff_base = 0; huff_base < lz_length; huff_base += huff_block_size) {
			size_t huff_length = lz_length - huff_base;
			if (huff_length > huff_block_size) huff_length = huff_block_size;

			uint16_t *syms = sym_buf;
			size_t num_syms = 0;

			upngi_huff_symbol sym_huffs[316];
			for (uint32_t i = 0; i < 316; i++) {
				sym_huffs[i].length = i >= 256 ? 6 : 8;
			}

			for (uint32_t i = 0; i < 2; i++) {
				uint32_t bits = 0;
				num_syms = upngi_encode_lz(sym_huffs, syms, tri_buf, (const char*)data + lz_base, (uint32_t)huff_base, (uint32_t)(huff_base + huff_length), &bits);

				uint32_t sym_counts[316] = { 0 };
				for (size_t i = 0; i < num_syms; i++) {
					uint32_t sym = syms[i];
					if (sym < 316) sym_counts[sym]++;
				}
				sym_counts[256] = 1;

				upngi_build_huffman(sym_huffs + 0, sym_counts + 0, 286, 15);
				upngi_build_huffman(sym_huffs + 286, sym_counts + 286, 30, 15);
			}

			uint32_t hlit = 286, hdist = 30;
			uint16_t header_syms[316], *header_dst = header_syms;
			header_dst += upngi_encode_lengths(header_dst, sym_huffs + 0, &hlit, 257);
			header_dst += upngi_encode_lengths(header_dst, sym_huffs + 286, &hdist, 1);
			size_t header_len = (size_t)(header_dst - header_syms);

			uint32_t header_counts[19] = { 0 };
			upngi_huff_symbol header_huffs[19];
			for (size_t i = 0; i < header_len; i++) {
				uint32_t sym = header_syms[i];
				if (sym < 19) header_counts[sym]++;
			}

			upngi_build_huffman(header_huffs, header_counts, 19, 7);
			static const uint8_t lens[] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
			uint32_t hclen = 19;
			while (hclen > 4 && header_huffs[lens[hclen - 1]].length == 0) hclen--;

			bool end = huff_base + huff_length == lz_length && lz_base + lz_length == length;
			upngi_push_bits(&dp, &reg, &num, end ? 0x5 : 0x4, 3);

			upngi_push_bits(&dp, &reg, &num, hlit - 257, 5);
			upngi_push_bits(&dp, &reg, &num, hdist - 1, 5);
			upngi_push_bits(&dp, &reg, &num, hclen - 4, 4);
			for (uint32_t i = 0; i < hclen; i++) {
				upngi_push_bits(&dp, &reg, &num, header_huffs[lens[i]].length, 3);
			}

			upngi_encode_syms(&dp, &reg, &num, header_huffs, header_syms, header_len);
			upngi_encode_syms(&dp, &reg, &num, sym_huffs, syms, num_syms);

			upngi_huff_symbol end_hs = sym_huffs[256];
			upngi_push_bits(&dp, &reg, &num, end_hs.bits, end_hs.length);
		}
	}

	if (num % 8 != 0) upngi_push_bits(&dp, &reg, &num, 0, 8 - (num % 8));
	uint32_t checksum = upngi_adler32(data, length);
	for (size_t i = 0; i < 4; i++) {
		upngi_push_bits(&dp, &reg, &num, (checksum >> (24 - i * 8)) & 0xff, 8);
	}

	upngi_flush_bits(&dp, reg, num);

	free(sym_buf);
	free(tri_buf);

	return (size_t)(dp - (char*)dst);
}

static void upngi_filter_row(uint32_t method, uint8_t *dst, const uint8_t *line, const uint8_t *prev, uint32_t width, uint32_t num_channels)
{
	int32_t dx = (int32_t)num_channels, pitch = (int32_t)(width * num_channels);
	if (method == 0) {
		for (int32_t i = 0; i < pitch; i++) dst[i] = line[i];
	} else if (method == 1) {
		for (int32_t i = 0; i < pitch; i++) dst[i] = line[i] - line[i-dx];
	} else if (method == 2) {
		for (int32_t i = 0; i < pitch; i++) dst[i] = line[i] - prev[i];
	} else if (method == 3) {
		for (int32_t i = 0; i < pitch; i++) dst[i] = line[i] - (line[i-dx] + prev[i]) / 2;
	} else if (method == 4) {
		for (int32_t i = 0; i < pitch; i++) {
			int32_t s = line[i], a = line[i-dx], b = prev[i], c = prev[i-dx], p = a+b-c;
			int32_t pa = abs(p-a), pb = abs(p-b), pc = abs(p-c);
			if (pa <= pb && pa <= pc) dst[i] = (uint8_t)(s - a);
			else if (pb <= pc) dst[i] = (uint8_t)(s - b);
			else dst[i] = (uint8_t)(s - c);
		}
	}
}

static void upngi_crc32_init(uint32_t *crc_table)
{
	for (uint32_t i = 0; i < 256; i++) {
		uint32_t r = i;
		for(uint32_t k = 0; k < 8; ++k) {
			r = ((r & 1) ? UINT32_C(0xEDB88320) : 0) ^ (r >> 1);
		}
		crc_table[i] = r;
	}
}

static uint32_t upngi_crc32(uint32_t *crc_table, const void *data, size_t size, uint32_t seed)
{
	uint32_t crc = ~seed;
	const uint8_t *src = (const uint8_t*)data;
	for (size_t i = 0; i < size; i++) {
		crc = crc_table[(crc ^ src[i]) & 0xff] ^ (crc >> 8);
	}
	return ~crc;
}

typedef struct {
	char *data;
	size_t offset;
	size_t size;
} upngi_stream;

static bool upngi_stream_setup(upngi_stream *stream)
{
	if (stream->data) {
		upngi_assert(stream->offset == stream->size);
		return false;
	}

	stream->data = (char*)malloc(stream->size);
	if (!stream->data) {
		stream->size = 0;
		return false;
	}

	return true;
}

static void upngi_stream_write(upngi_stream *stream, const void *data, size_t size)
{
	if (stream->data) {
		upngi_assert((size_t)(stream->size - stream->offset) >= size);
		memcpy(stream->data + stream->offset, data, size);
		stream->offset += size;
	} else {
		stream->size += size;
	}
}

static void upngi_add_chunk(uint32_t *crc_table, upngi_stream *stream, const char *tag, const void *data, size_t size)
{
	uint8_t be_size[] = { (uint8_t)(size>>24), (uint8_t)(size>>16), (uint8_t)(size>>8), (uint8_t)size };
	upngi_stream_write(stream, be_size, 4);
	upngi_stream_write(stream, tag, 4);
	upngi_stream_write(stream, data, size);
	uint32_t crc = 0;
	if (stream->data) {
		crc = upngi_crc32(crc_table, stream->data + stream->offset - (size + 4), size + 4, 0);
	}
	uint8_t be_crc[] = { (uint8_t)(crc>>24), (uint8_t)(crc>>16), (uint8_t)(crc>>8), (uint8_t)crc };
	upngi_stream_write(stream, be_crc, 4);
}

static FILE *upngi_fopen(const char *path, size_t path_len)
{
#if defined(_WIN32)
	wchar_t wpath_buf[256];
	wchar_t *wpath = NULL;

	if (path_len == SIZE_MAX) {
		path_len = strlen(path);
	}
	if (path_len < upngi_arraycount(wpath_buf) - 1) {
		wpath = wpath_buf;
	} else {
		wpath = malloc((path_len + 1) * sizeof(wchar_t));
		if (!wpath) return NULL;
	}

	// Convert UTF-8 to UTF-16 but allow stray surrogate pairs as the Windows
	// file system encoding allows them as well..
	size_t wlen = 0;
	for (size_t i = 0; i < path_len; ) {
		uint32_t code = UINT32_MAX;
		char c = path[i++];
		if ((c & 0x80) == 0) {
			code = (uint32_t)c;
		} else if ((c & 0xe0) == 0xc0) {
			code = (uint32_t)(c & 0x1f);
			if (i < path_len) code = code << 6 | (uint32_t)(path[i++] & 0x3f);
		} else if ((c & 0xf0) == 0xe0) {
			code = (uint32_t)(c & 0x0f);
			if (i < path_len) code = code << 6 | (uint32_t)(path[i++] & 0x3f);
			if (i < path_len) code = code << 6 | (uint32_t)(path[i++] & 0x3f);
		} else if ((c & 0xf8) == 0xf0) {
			code = (uint32_t)(c & 0x07);
			if (i < path_len) code = code << 6 | (uint32_t)(path[i++] & 0x3f);
			if (i < path_len) code = code << 6 | (uint32_t)(path[i++] & 0x3f);
			if (i < path_len) code = code << 6 | (uint32_t)(path[i++] & 0x3f);
		}
		if (code < 0x10000) {
			wpath[wlen++] = (wchar_t)code;
		} else {
			code -= 0x10000;
			wpath[wlen++] = (wchar_t)(0xd800 + (code >> 10));
			wpath[wlen++] = (wchar_t)(0xdc00 + (code & 0x3ff));
		}
	}
	wpath[wlen] = 0;

	FILE *file = NULL;
#if _MSC_VER >= 1400
	if (_wfopen_s(&file, wpath, L"wb") != 0) {
		file = NULL;
	}
#else
	file = _wfopen(wpath, L"wb");
#endif

	if (wpath != wpath_buf) {
		free(wpath);
	}

	return file;
#else
	if (path_len == SIZE_MAX) {
		return fopen(path, "wb");
	}

	char copy_buf[256];
	char *copy = NULL;

	if (path_len < upngi_arraycount(copy_buf) - 1) {
		copy = copy_buf;
	} else {
		copy = malloc((path_len + 1) * sizeof(char));
		if (!copy) return NULL;
	}
	memcpy(copy, path, path_len);
	copy[path_len] = '\0';

	FILE *file = fopen(copy, "wb");
	if (copy != copy_buf) {
		free(copy);
	}

	return file;
#endif
}

upng_buffer upng_write_memory(const void *data, uint32_t width, uint32_t height)
{
	upng_buffer result = { 0 };

	const uint8_t *pixels = (const uint8_t*)data;
	size_t num_pixels = (size_t)width * (size_t)height;

	uint32_t num_channels = 3;
	for (size_t i = 0; i < num_pixels; i++) {
		if (pixels[i * 4 + 3] < 255) {
			num_channels = 4;
			break;
		}
	}

	size_t line_size = (width + 1) * num_channels;
	uint8_t *line_buf = (uint8_t*)calloc(line_size * 2, sizeof(uint8_t));
	size_t idat_size = (width * num_channels + 1) * height;
	uint8_t *idat = (uint8_t*)malloc(idat_size);
	if (!line_buf || !idat) {
		free(line_buf);
		free(idat);
		return result;
	}
	uint8_t *prev = line_buf + num_channels;
	uint8_t *line = line_buf + line_size + num_channels;

	uint32_t pitch = width * num_channels;
	uint8_t *dst = idat;
	for (uint32_t y = 0; y < height; y++) {
		const uint8_t *src = pixels + y * width * 4;
		for (uint32_t c = 0; c < num_channels; c++) {
			for (uint32_t x = 0; x < width; x++) {
				line[x*num_channels + c] = src[x*4 + c];
			}
		}

		uint32_t best_filter = 0, best_entropy = UINT32_MAX;
		for (uint32_t f = 0; f <= 4; f++) {
			upngi_filter_row(f, dst + 1, line, prev, width, num_channels);
			uint32_t entropy = 0;
			for (uint32_t x = 0; x < pitch; x++) {
				entropy += abs((int8_t)dst[1 + x]);
			}
			if (entropy < best_entropy) {
				best_filter = f;
				best_entropy = entropy;
			}
		}
		if (best_filter != 4) {
			upngi_filter_row(best_filter, dst + 1, line, prev, width, num_channels);
		}
		dst[0] = (uint8_t)best_filter;

		dst += width * num_channels + 1;

		uint8_t *tmp_line = line;
		line = prev;
		prev = tmp_line;
	}

	// TODO: Proper bound
	size_t idat_deflate_size = idat_size + idat_size / 16 + 64;
	uint8_t *idat_deflate = (uint8_t*)malloc(idat_deflate_size);
	size_t idat_length = upngi_deflate(idat_deflate, idat, idat_size);
	if (idat_length > 0) {
		uint8_t ihdr[] = {
			(uint8_t)(width>>24), (uint8_t)(width>>16), (uint8_t)(width>>8), (uint8_t)width,
			(uint8_t)(height>>24), (uint8_t)(height>>16), (uint8_t)(height>>8), (uint8_t)height,
			8, (uint8_t)(num_channels == 4 ? 6 : 2), 0, 0, 0,
		};

		uint32_t crc_table[256];
		upngi_crc32_init(crc_table);

		upngi_stream stream = { 0 };
		do {
			const char magic[] = "\x89PNG\r\n\x1a\n";
			upngi_stream_write(&stream, magic, 8);
			upngi_add_chunk(crc_table, &stream, "IHDR", ihdr, sizeof(ihdr));
			upngi_add_chunk(crc_table, &stream, "IDAT", idat_deflate, idat_length);
			upngi_add_chunk(crc_table, &stream, "IEND", NULL, 0);
		} while (upngi_stream_setup(&stream));

		result.data = stream.data;
		result.size = stream.size;
	}

	free(line_buf);
	free(idat);
	free(idat_deflate);

	return result;
}

static bool upngi_write_stdio(FILE *f, const void *data, uint32_t width, uint32_t height)
{
	if (!f) return false;

	bool ok = false;
	upng_buffer buffer = upng_write_memory(data, width, height);
	if (buffer.size > 0) {
		ok = fwrite(buffer.data, 1, buffer.size, f) == buffer.size;
	}
	fclose(f);
	return ok;
}

bool upng_write_file(const char *filename, const void *data, uint32_t width, uint32_t height)
{
	FILE *f = upngi_fopen(filename, SIZE_MAX);
	return upngi_write_stdio(f, data, width, height);
}

bool upng_write_file_len(const char *filename, size_t filename_len, const void *data, uint32_t width, uint32_t height)
{
	FILE *f = upngi_fopen(filename, filename_len);
	return upngi_write_stdio(f, data, width, height);
}

#endif
#endif
