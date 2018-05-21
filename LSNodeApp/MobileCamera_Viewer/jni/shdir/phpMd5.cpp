// phpMd5.cpp : 定义控制台应用程序的入口点。
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "phpMd5.h"


/*
 * Convert an array of little-endian words to a hex string.
 */
static int binl2hex(int *src, char *dst, int nsize)
{
  if( src == 0 )
    return 1;

  if( dst == 0 )
    return 1;

  if( nsize < 32 )
    return 1;


  char hex_tab[]  = "0123456789abcdef";

  int j = 0;
  for(int i = 0; i < 16; i++)
  {
#if _SXDEF_LITTLE_ENDIAN
    dst[j++] = hex_tab[(src[i>>2] >> ((i%4)*8+4)) & 0xF];

//syslog(LOG_DEBUG,"end:%c-%d-%d-%d",dst[j-1],(src[i>>2] >> ((i%4)*8+4)),((i%4)*8+4),src[i>>2]);


    dst[j++] = hex_tab[(src[i>>2] >> ((i%4)*8  )) & 0xF];
//syslog(LOG_DEBUG,"end:%c-%d-%d-%d",dst[j-1],(src[i>>2] >> ((i%4)*8+4)),((i%4)*8+4),src[i>>2]);
#else
#error ERROR: Please check here!
#endif
  }
  dst[j] = 0;
  return 0;
}

/*
 * Convert a string to an array of little-endian words
 * If chrsz is ASCII, characters >255 have their hi-byte silently ignored.
 */
static int str2binl(const char * src,int *pndst, int nsize)
{
  int i;

  if(src == 0)
    return 1;

  if(pndst == 0)
    return 1;

  int nloop = strlen(src) * 8;
  if( (nloop >> 5) >= nsize )
    return 1;

  for(i=0; i<nsize; i++ )
    pndst[i] = 0;

   int mask = (1 << 8) - 1;

  for(i = 0; i < nloop; i += 8)
  {
#if _SXDEF_LITTLE_ENDIAN
    pndst[i>>5] |= ( src[i / 8] & mask) << (i%32);
#else
#error ERROR: Please check here!
#endif
  }

  return 0;
}

/*
 * Add integers, wrapping at 2^32.
 */
static int safe_add(int x, int y)
{
  int lsw = (x & 0xFFFF) + (y & 0xFFFF);
  int msw = (x >> 16) + (y >> 16) + (lsw >> 16);
  return (msw << 16) | (lsw & 0xFFFF);
}

/*
 * Bitwise rotate a 32-bit number to the left.
 */
static int bit_rol(unsigned int num, unsigned int cnt)
{
  //old
  // return (num << cnt) | (num >>> (32 - cnt));
  return (num << cnt) | (num >> (32 - cnt));
}

/*
 * These functions implement the four basic operations the algorithm uses.
 */
static int md5_cmn(int q, int a, int b, int x, int s, int t)
{
  return safe_add(bit_rol(safe_add(safe_add(a, q), safe_add(x, t)), s),b);
}
static int md5_ff( int a,  int b,  int c,  int d,  int x,  int s,  int t)
{
  return md5_cmn((b & c) | ((~b) & d), a, b, x, s, t);
}
static int md5_gg( int a,  int b,  int c,  int d,  int x, int s,  int t)
{
  return md5_cmn((b & d) | (c & (~d)), a, b, x, s, t);
}
static int md5_hh( int a,  int b,  int c,  int d,  int x,  int s,  int t)
{
  return md5_cmn(b ^ c ^ d, a, b, x, s, t);
}
static int md5_ii( int a,  int b,  int c,  int d,  int x,  int s,  int t)
{
  return md5_cmn(c ^ (b | (~d)), a, b, x, s, t);
}

/*
 * Calculate the MD5 of an array of little-endian words, and a bit length
 */
static int core_md5( int *x,  int len,  int * dst)
{
  if( x == 0)
    return 1;

  if( len <= 0 )
    return 1;

  if (dst == 0)
    return 1;

  /* append padding */
  x[len >> 5] |= 0x80 << ((len) % 32);

//syslog(LOG_DEBUG,"x[len >> 5]:%d",x[len >> 5]);
//old
//--x[(((len + 64) >>> 9) << 4) + 14] = len;

  unsigned int ntemp = len + 64;
  x[(((ntemp) >> 9) << 4) + 14] = len;
//syslog(LOG_DEBUG,"x[(((ntemp) >> 9) << 4) + 14]:%d",x[(((ntemp) >> 9) << 4) + 14]);

  int a =  1732584193;
  int b = -271733879;
  int c = -1732584194;
  int d =  271733878;
  
  int ncurrlen = (((ntemp) >> 9) << 4) + 14 + 1;
  for(int i = 0; i < ncurrlen; i += 16)
  {
    int olda = a;
    int oldb = b;
    int oldc = c;
    int oldd = d;

    a = md5_ff(a, b, c, d, x[i+ 0], 7 , -680876936);
    d = md5_ff(d, a, b, c, x[i+ 1], 12, -389564586);
    c = md5_ff(c, d, a, b, x[i+ 2], 17,  606105819);
    b = md5_ff(b, c, d, a, x[i+ 3], 22, -1044525330);

    a = md5_ff(a, b, c, d, x[i+ 4], 7 , -176418897);
    d = md5_ff(d, a, b, c, x[i+ 5], 12,  1200080426);
    c = md5_ff(c, d, a, b, x[i+ 6], 17, -1473231341);
    b = md5_ff(b, c, d, a, x[i+ 7], 22, -45705983);


    a = md5_ff(a, b, c, d, x[i+ 8], 7 ,  1770035416);
    d = md5_ff(d, a, b, c, x[i+ 9], 12, -1958414417);
    c = md5_ff(c, d, a, b, x[i+10], 17, -42063);
    b = md5_ff(b, c, d, a, x[i+11], 22, -1990404162);
    

    a = md5_ff(a, b, c, d, x[i+12], 7 ,  1804603682);
    d = md5_ff(d, a, b, c, x[i+13], 12, -40341101);
    c = md5_ff(c, d, a, b, x[i+14], 17, -1502002290);
    b = md5_ff(b, c, d, a, x[i+15], 22,  1236535329);


    a = md5_gg(a, b, c, d, x[i+ 1], 5 , -165796510);
    d = md5_gg(d, a, b, c, x[i+ 6], 9 , -1069501632);
    c = md5_gg(c, d, a, b, x[i+11], 14,  643717713);
    b = md5_gg(b, c, d, a, x[i+ 0], 20, -373897302);

    a = md5_gg(a, b, c, d, x[i+ 5], 5 , -701558691);
    d = md5_gg(d, a, b, c, x[i+10], 9 ,  38016083);
    c = md5_gg(c, d, a, b, x[i+15], 14, -660478335);
    b = md5_gg(b, c, d, a, x[i+ 4], 20, -405537848);

    a = md5_gg(a, b, c, d, x[i+ 9], 5 ,  568446438);
    d = md5_gg(d, a, b, c, x[i+14], 9 , -1019803690);
    c = md5_gg(c, d, a, b, x[i+ 3], 14, -187363961);
    b = md5_gg(b, c, d, a, x[i+ 8], 20,  1163531501);

    a = md5_gg(a, b, c, d, x[i+13], 5 , -1444681467);
    d = md5_gg(d, a, b, c, x[i+ 2], 9 , -51403784);
    c = md5_gg(c, d, a, b, x[i+ 7], 14,  1735328473);
    b = md5_gg(b, c, d, a, x[i+12], 20, -1926607734);

    a = md5_hh(a, b, c, d, x[i+ 5], 4 , -378558);
    d = md5_hh(d, a, b, c, x[i+ 8], 11, -2022574463);
    c = md5_hh(c, d, a, b, x[i+11], 16,  1839030562);
    b = md5_hh(b, c, d, a, x[i+14], 23, -35309556);

    a = md5_hh(a, b, c, d, x[i+ 1], 4 , -1530992060);
    d = md5_hh(d, a, b, c, x[i+ 4], 11,  1272893353);
    c = md5_hh(c, d, a, b, x[i+ 7], 16, -155497632);
    b = md5_hh(b, c, d, a, x[i+10], 23, -1094730640);

    a = md5_hh(a, b, c, d, x[i+13], 4 ,  681279174);
    d = md5_hh(d, a, b, c, x[i+ 0], 11, -358537222);
    c = md5_hh(c, d, a, b, x[i+ 3], 16, -722521979);
    b = md5_hh(b, c, d, a, x[i+ 6], 23,  76029189);

    a = md5_hh(a, b, c, d, x[i+ 9], 4 , -640364487);
    d = md5_hh(d, a, b, c, x[i+12], 11, -421815835);
    c = md5_hh(c, d, a, b, x[i+15], 16,  530742520);
    b = md5_hh(b, c, d, a, x[i+ 2], 23, -995338651);

    a = md5_ii(a, b, c, d, x[i+ 0], 6 , -198630844);
    d = md5_ii(d, a, b, c, x[i+ 7], 10,  1126891415);
    c = md5_ii(c, d, a, b, x[i+14], 15, -1416354905);
    b = md5_ii(b, c, d, a, x[i+ 5], 21, -57434055);

    a = md5_ii(a, b, c, d, x[i+12], 6 ,  1700485571);
    d = md5_ii(d, a, b, c, x[i+ 3], 10, -1894986606);
    c = md5_ii(c, d, a, b, x[i+10], 15, -1051523);
    b = md5_ii(b, c, d, a, x[i+ 1], 21, -2054922799);

    a = md5_ii(a, b, c, d, x[i+ 8], 6 ,  1873313359);
    d = md5_ii(d, a, b, c, x[i+15], 10, -30611744);
    c = md5_ii(c, d, a, b, x[i+ 6], 15, -1560198380);
    b = md5_ii(b, c, d, a, x[i+13], 21,  1309151649);

    a = md5_ii(a, b, c, d, x[i+ 4], 6 , -145523070);
    d = md5_ii(d, a, b, c, x[i+11], 10, -1120210379);
    c = md5_ii(c, d, a, b, x[i+ 2], 15,  718787259);
    b = md5_ii(b, c, d, a, x[i+ 9], 21, -343485551);

    a = safe_add(a, olda);
    b = safe_add(b, oldb);
    c = safe_add(c, oldc);
    d = safe_add(d, oldd);
  }

  dst[0] =a;
  dst[1] =b;
  dst[2] =c;
  dst[3] =d;


  return 0;

}

/*
 * This is  the function you'll usually want to call
 * They take string arguments and return either hex or base-64 encoded strings
 */
int php_md5(const char *src, char *dst,  int nsize)
{
  if( dst == 0 )
    return 1;

  if( nsize < 32 )
    return 1;

  int nbuffsize1 = ((strlen(src) * 8) >> 5) + 1;
  int nbuffsize2 = (((strlen(src) * 8 + 64) >> 9) << 4) + 15;


  int nbuffsize = nbuffsize1 > nbuffsize2 ?  nbuffsize1 : nbuffsize2;

  if (nbuffsize < strlen(src) * 8) {/* Added by GaoHua */
	nbuffsize = strlen(src) * 8;
  }

  int* buff = new int[nbuffsize];
  if( buff == 0 )
    return 1;

  memset(buff, 0, nbuffsize*sizeof(int));/* Added by GaoHua */

  int ptemp[4];
  for(int i=0; i<4; i++)
  {
    ptemp[i] = 0;
  }

  if( str2binl(src,buff,nbuffsize) == 1 )
  {
    delete buff;
    return 1;
  }

  if( core_md5(buff,strlen(src) * 8, ptemp ) == 1)
  {
    delete buff;
    return 1;
  }

  delete buff;

  if( binl2hex(ptemp, dst, 32) == 1)
  {
    //syslog(LOG_DEBUG,"error");
    return 1;
  }

  return 0;
}

//for example:input a string and get a md5 string.
/*
int main()
{
    char src[1024];
    char md5[32] = {0};

    strcpy(src,"Cenusdesign");

    php_md5(src, md5, sizeof(md5));

    printf("\nmd5:%s\n",md5);

    return 0;
}
*/