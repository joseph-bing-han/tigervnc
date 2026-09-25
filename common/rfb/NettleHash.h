#ifndef RFB_NETTLEHASH_H
#define RFB_NETTLEHASH_H

#include <assert.h>
#include <string.h>

#include <nettle/version.h>
#include <nettle/md5.h>
#include <nettle/sha1.h>
#include <nettle/sha2.h>

namespace rfb {

inline void nettleMd5Digest(md5_ctx* ctx, size_t length, uint8_t* digest)
{
#if NETTLE_VERSION_MAJOR >= 4
  assert(length <= MD5_DIGEST_SIZE);
  uint8_t full[MD5_DIGEST_SIZE];
  md5_digest(ctx, full);
  memcpy(digest, full, length);
#else
  md5_digest(ctx, length, digest);
#endif
}

inline void nettleSha1Digest(sha1_ctx* ctx, size_t length, uint8_t* digest)
{
#if NETTLE_VERSION_MAJOR >= 4
  assert(length <= SHA1_DIGEST_SIZE);
  uint8_t full[SHA1_DIGEST_SIZE];
  sha1_digest(ctx, full);
  memcpy(digest, full, length);
#else
  sha1_digest(ctx, length, digest);
#endif
}

inline void nettleSha256Digest(sha256_ctx* ctx, size_t length, uint8_t* digest)
{
#if NETTLE_VERSION_MAJOR >= 4
  assert(length <= SHA256_DIGEST_SIZE);
  uint8_t full[SHA256_DIGEST_SIZE];
  sha256_digest(ctx, full);
  memcpy(digest, full, length);
#else
  sha256_digest(ctx, length, digest);
#endif
}

}

#endif
