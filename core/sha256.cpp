#include "sha256.h"

namespace Core
{

#ifndef PUT_UINT32_BE
#define PUT_UINT32_BE(n, b, i)				\
{							\
	(b)[(i)] = (unsigned char)((n) >> 24);		\
	(b)[(i) + 1] = (unsigned char)((n) >> 16);	\
	(b)[(i) + 2] = (unsigned char)((n) >>  8);	\
	(b)[(i) + 3] = (unsigned char)((n));		\
}
#endif

#ifndef GET_UINT32_BE
#define GET_UINT32_BE(n, b, i)			\
{						\
	(n) = ((unsigned int)(b)[(i)] << 24)		\
	| ((unsigned int)(b)[(i) + 1] << 16)		\
	| ((unsigned int)(b)[(i) + 2] <<  8)		\
	| ((unsigned int)(b)[(i) + 3]);			\
}
#endif

static unsigned char Sha256Padding[64] = {
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void Sha256::Process(const unsigned char data[64])
{
	unsigned int temp1, temp2, W[64];
	unsigned int A, B, C, D, E, F, G, H;

	GET_UINT32_BE(W[0], data, 0);
	GET_UINT32_BE(W[1], data, 4);
	GET_UINT32_BE(W[2], data, 8);
	GET_UINT32_BE(W[3], data, 12);
	GET_UINT32_BE(W[4], data, 16);
	GET_UINT32_BE(W[5], data, 20);
	GET_UINT32_BE(W[6], data, 24);
	GET_UINT32_BE(W[7], data, 28);
	GET_UINT32_BE(W[8], data, 32);
	GET_UINT32_BE(W[9], data, 36);
	GET_UINT32_BE(W[10], data, 40);
	GET_UINT32_BE(W[11], data, 44);
	GET_UINT32_BE(W[12], data, 48);
	GET_UINT32_BE(W[13], data, 52);
	GET_UINT32_BE(W[14], data, 56);
	GET_UINT32_BE(W[15], data, 60);

#define	SHR(x, n) ((x & 0xFFFFFFFF) >> n)
#define ROTR(x, n) (SHR(x, n) | (x << (32 - n)))

#define S0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^  SHR(x, 3))
#define S1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^  SHR(x, 10))

#define S2(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define S3(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))

#define F0(x, y, z) ((x & y) | (z & (x | y)))
#define F1(x, y, z) (z ^ (x & (y ^ z)))

#define R(t)					\
(						\
	W[t] = S1(W[t -  2]) + W[t -  7] +	\
		S0(W[t - 15]) + W[t - 16]	\
)

#define P(a, b, c, d, e, f, g, h, x, K)			\
{							\
	temp1 = h + S3(e) + F1(e, f, g) + K + x;	\
	temp2 = S2(a) + F0(a, b, c);			\
	d += temp1; h = temp1 + temp2;			\
}

	A = State[0];
	B = State[1];
	C = State[2];
	D = State[3];
	E = State[4];
	F = State[5];
	G = State[6];
	H = State[7];

	P(A, B, C, D, E, F, G, H, W[0], 0x428A2F98);
	P(H, A, B, C, D, E, F, G, W[1], 0x71374491);
	P(G, H, A, B, C, D, E, F, W[2], 0xB5C0FBCF);
	P(F, G, H, A, B, C, D, E, W[3], 0xE9B5DBA5);
	P(E, F, G, H, A, B, C, D, W[4], 0x3956C25B);
	P(D, E, F, G, H, A, B, C, W[5], 0x59F111F1);
	P(C, D, E, F, G, H, A, B, W[6], 0x923F82A4);
	P(B, C, D, E, F, G, H, A, W[7], 0xAB1C5ED5);
	P(A, B, C, D, E, F, G, H, W[8], 0xD807AA98);
	P(H, A, B, C, D, E, F, G, W[9], 0x12835B01);
	P(G, H, A, B, C, D, E, F, W[10], 0x243185BE);
	P(F, G, H, A, B, C, D, E, W[11], 0x550C7DC3);
	P(E, F, G, H, A, B, C, D, W[12], 0x72BE5D74);
	P(D, E, F, G, H, A, B, C, W[13], 0x80DEB1FE);
	P(C, D, E, F, G, H, A, B, W[14], 0x9BDC06A7);
	P(B, C, D, E, F, G, H, A, W[15], 0xC19BF174);
	P(A, B, C, D, E, F, G, H, R(16), 0xE49B69C1);
	P(H, A, B, C, D, E, F, G, R(17), 0xEFBE4786);
	P(G, H, A, B, C, D, E, F, R(18), 0x0FC19DC6);
	P(F, G, H, A, B, C, D, E, R(19), 0x240CA1CC);
	P(E, F, G, H, A, B, C, D, R(20), 0x2DE92C6F);
	P(D, E, F, G, H, A, B, C, R(21), 0x4A7484AA);
	P(C, D, E, F, G, H, A, B, R(22), 0x5CB0A9DC);
	P(B, C, D, E, F, G, H, A, R(23), 0x76F988DA);
	P(A, B, C, D, E, F, G, H, R(24), 0x983E5152);
	P(H, A, B, C, D, E, F, G, R(25), 0xA831C66D);
	P(G, H, A, B, C, D, E, F, R(26), 0xB00327C8);
	P(F, G, H, A, B, C, D, E, R(27), 0xBF597FC7);
	P(E, F, G, H, A, B, C, D, R(28), 0xC6E00BF3);
	P(D, E, F, G, H, A, B, C, R(29), 0xD5A79147);
	P(C, D, E, F, G, H, A, B, R(30), 0x06CA6351);
	P(B, C, D, E, F, G, H, A, R(31), 0x14292967);
	P(A, B, C, D, E, F, G, H, R(32), 0x27B70A85);
	P(H, A, B, C, D, E, F, G, R(33), 0x2E1B2138);
	P(G, H, A, B, C, D, E, F, R(34), 0x4D2C6DFC);
	P(F, G, H, A, B, C, D, E, R(35), 0x53380D13);
	P(E, F, G, H, A, B, C, D, R(36), 0x650A7354);
	P(D, E, F, G, H, A, B, C, R(37), 0x766A0ABB);
	P(C, D, E, F, G, H, A, B, R(38), 0x81C2C92E);
	P(B, C, D, E, F, G, H, A, R(39), 0x92722C85);
	P(A, B, C, D, E, F, G, H, R(40), 0xA2BFE8A1);
	P(H, A, B, C, D, E, F, G, R(41), 0xA81A664B);
	P(G, H, A, B, C, D, E, F, R(42), 0xC24B8B70);
	P(F, G, H, A, B, C, D, E, R(43), 0xC76C51A3);
	P(E, F, G, H, A, B, C, D, R(44), 0xD192E819);
	P(D, E, F, G, H, A, B, C, R(45), 0xD6990624);
	P(C, D, E, F, G, H, A, B, R(46), 0xF40E3585);
	P(B, C, D, E, F, G, H, A, R(47), 0x106AA070);
	P(A, B, C, D, E, F, G, H, R(48), 0x19A4C116);
	P(H, A, B, C, D, E, F, G, R(49), 0x1E376C08);
	P(G, H, A, B, C, D, E, F, R(50), 0x2748774C);
	P(F, G, H, A, B, C, D, E, R(51), 0x34B0BCB5);
	P(E, F, G, H, A, B, C, D, R(52), 0x391C0CB3);
	P(D, E, F, G, H, A, B, C, R(53), 0x4ED8AA4A);
	P(C, D, E, F, G, H, A, B, R(54), 0x5B9CCA4F);
	P(B, C, D, E, F, G, H, A, R(55), 0x682E6FF3);
	P(A, B, C, D, E, F, G, H, R(56), 0x748F82EE);
	P(H, A, B, C, D, E, F, G, R(57), 0x78A5636F);
	P(G, H, A, B, C, D, E, F, R(58), 0x84C87814);
	P(F, G, H, A, B, C, D, E, R(59), 0x8CC70208);
	P(E, F, G, H, A, B, C, D, R(60), 0x90BEFFFA);
	P(D, E, F, G, H, A, B, C, R(61), 0xA4506CEB);
	P(C, D, E, F, G, H, A, B, R(62), 0xBEF9A3F7);
	P(B, C, D, E, F, G, H, A, R(63), 0xC67178F2);

	State[0] += A;
	State[1] += B;
	State[2] += C;
	State[3] += D;
	State[4] += E;
	State[5] += F;
	State[6] += G;
	State[7] += H;
}

void Sha256::Update(const void *buf, unsigned int len)
{
	size_t fill;
	const unsigned char *input = reinterpret_cast<const unsigned char *>(buf);
	unsigned int left;

	if (len == 0)
		return;

	left = Total[0] & 0x3F;
	fill = 64 - left;
	Total[0] += len;
	Total[0] &= 0xFFFFFFFF;
	if (Total[0] < len)
		Total[1]++;

	if (left && len >= fill) {
		Memory::MemCpy(Memory::MemAdd(Buffer, left), input, fill);
		Process(Buffer);
		input += fill;
		len -= fill;
		left = 0;
	}

	while (len >= 64) {
		Process(input);
		input += 64;
		len -= 64;
	}

	if (len > 0)
		Memory::MemCpy(Memory::MemAdd(Buffer, left), input, len);
}

void Sha256::Clear()
{
    Core::Memory::MemSet(Total, 0, sizeof(Total));
    Core::Memory::MemSet(State, 0, sizeof(State));
    Core::Memory::MemSet(Buffer, 0, sizeof(Buffer));
    Core::Memory::MemSet(Ipad, 0, sizeof(Ipad));
    Core::Memory::MemSet(Opad, 0, sizeof(Opad));
}

Sha256::Sha256()
{
    Clear();

    Total[0] = 0;
	Total[1] = 0;

	State[0] = 0x6A09E667;
	State[1] = 0xBB67AE85;
	State[2] = 0x3C6EF372;
	State[3] = 0xA54FF53A;
    State[4] = 0x510E527F;
	State[5] = 0x9B05688C;
	State[6] = 0x1F83D9AB;
	State[7] = 0x5BE0CD19;
}

Sha256::~Sha256()
{
    Clear();
}

void Sha256::Finish(unsigned char output[32])
{
	unsigned int last, padn;
	unsigned int high, low;
	unsigned char msglen[8];

	high = (Total[0] >> 29) | (Total[1] <<  3);
	low  = (Total[0] <<  3);

	PUT_UINT32_BE(high, msglen, 0);
	PUT_UINT32_BE(low,  msglen, 4);

	last = Total[0] & 0x3F;
	padn = (last < 56) ? (56 - last) : (120 - last);

	Update(Sha256Padding, padn);
	Update(msglen, 8);

	PUT_UINT32_BE(State[0], output, 0);
	PUT_UINT32_BE(State[1], output, 4);
	PUT_UINT32_BE(State[2], output, 8);
	PUT_UINT32_BE(State[3], output, 12);
	PUT_UINT32_BE(State[4], output, 16);
	PUT_UINT32_BE(State[5], output, 20);
	PUT_UINT32_BE(State[6], output, 24);

	PUT_UINT32_BE(State[7], output, 28);
}


}