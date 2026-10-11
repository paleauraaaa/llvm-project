#ifdef __cplusplus
extern "C" {
#endif
void __emit(unsigned int);
unsigned char __loadbyteupdate(int, void *);
unsigned short __loadshortupdate(int, void *);
unsigned long __loadwordupdate(int, void *);
float __loadfloatupdate(int, void *);
double __loaddoubleupdate(int, void *);
unsigned long long __loaddoublewordupdate(int, void *);
void __storebyteupdate(unsigned char, int, void *);
void __storeshortupdate(unsigned short, int, void *);
void __storewordupdate(unsigned long, int, void *);
void __storefloatupdate(float, int, void *);
void __storedoubleupdate(double, int, void *);
void __storedoublewordupdate(unsigned long long, int, void *);
void __dcbz128(int, void *);
void __dcbz(int, void *);
void __dcbt(int, const void *);
void __dcbst(int, const void *);
void __dcbf(int, const void *);

#define DECLARE_REVERSE(SUFFIX) \
unsigned short __loadshortbytereverse##SUFFIX(int, const void *); \
unsigned long __loadwordbytereverse##SUFFIX(int, const void *); \
unsigned long long __loaddoublewordbytereverse##SUFFIX(int, const void *); \
void __storeshortbytereverse##SUFFIX(unsigned short, int, void *); \
void __storewordbytereverse##SUFFIX(unsigned int, int, void *); \
void __storedoublewordbytereverse##SUFFIX(unsigned long long, int, void *); \
unsigned short __loadvolatileshortbytereverse##SUFFIX(int, const volatile void *); \
unsigned long __loadvolatilewordbytereverse##SUFFIX(int, const volatile void *); \
unsigned long long __loadvolatiledoublewordbytereverse##SUFFIX(int, const void *); \
void __storevolatileshortbytereverse##SUFFIX(unsigned short, int, volatile void *); \
void __storevolatilewordbytereverse##SUFFIX(unsigned int, int, volatile void *); \
void __storevolatiledoublewordbytereverse##SUFFIX(unsigned long long, int, void *);
DECLARE_REVERSE()
DECLARE_REVERSE(G)
#undef DECLARE_REVERSE

unsigned short _byteswap_ushort(unsigned short);
unsigned long _byteswap_ulong(unsigned long);
unsigned long long _byteswap_uint64(unsigned long long);
long long __mulh(long long, long long);
unsigned long long __umulh(unsigned long long, unsigned long long);
double __fsqrt(double);
float __fsqrts(float);
double __frsqrte(double);
float __fres(double);
double __fsel(double, double, double);
float __fself(float, float, float);
double __fctiwz(double);
double __fctidz(double);
double __fctiw(double);
double __fctid(double);
double __fcfid(double);
void __stfiwx(double, int, void *);
double __frnd(double);
unsigned int _CountLeadingZeros64(long long);
unsigned int _CountLeadingZeros(long);
unsigned long long __mftb(void);
void __SetHWThreadPriorityHigh(void);
void __SetHWThreadPriorityMed(void);
void __SetHWThreadPriorityLow(void);
void __tw(unsigned int, int, int);
void __twi(unsigned int, int, int);
void __td(unsigned int, long long, long long);
void __tdi(unsigned int, long long, long long);
#ifdef __cplusplus
}
#endif
