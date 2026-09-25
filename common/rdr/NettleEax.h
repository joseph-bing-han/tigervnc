#ifndef RDR_NETTLEEAX_H
#define RDR_NETTLEEAX_H

#include <nettle/version.h>
#include <nettle/eax.h>

#if NETTLE_VERSION_MAJOR >= 4
#define RDR_EAX_DIGEST(ctx, encrypt, digest) EAX_DIGEST(ctx, encrypt, digest)
#else
#define RDR_EAX_DIGEST(ctx, encrypt, digest) EAX_DIGEST(ctx, encrypt, 16, digest)
#endif

#endif
