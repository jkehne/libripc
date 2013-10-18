#define _GNU_SOURCE
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "base64.h"

/* Based on an implementation by Barry Steyn, available under
 * http://doctrina.org/Base64-With-OpenSSL
 */

void base64(
	char *destination, const uint8_t *source,
	size_t l_dest, size_t l_src)
{
	BIO *bio, *b64;
	FILE* stream;

	stream = fmemopen(destination, l_dest, "w");
	b64 = BIO_new(BIO_f_base64());
	bio = BIO_new_fp(stream, BIO_NOCLOSE);
	bio = BIO_push(b64, bio);
	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
	BIO_write(bio, source, l_src);
	BIO_flush(bio);
	BIO_free_all(bio);
	fclose(stream);
}
