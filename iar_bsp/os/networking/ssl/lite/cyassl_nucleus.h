#ifndef CYASSL_NUCLEUS_H
#define CYASSL_NUCLEUS_H

SSL_CTX * 	CYASSL_nucleus_init(void);
void 		CYASSL_load_buffer(SSL_CTX* ctx, const char** buff, int type, int size);

#endif
