#include <cstring>
#include <initializer_list>

#include <rfb/NettleHash.h>

int main()
{
  const uint8_t input[] = {'a', 'b', 'c'};
  const uint8_t md5Expected[] = {
    0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0,
    0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72
  };
  const uint8_t sha1Expected[] = {
    0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a,
    0xba, 0x3e, 0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c,
    0x9c, 0xd0, 0xd8, 0x9d
  };
  const uint8_t sha256Expected[] = {
    0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
    0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
    0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
    0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
  };
  uint8_t output[SHA256_DIGEST_SIZE];

  md5_ctx md5;
  md5_init(&md5);
  md5_update(&md5, sizeof(input), input);
  rfb::nettleMd5Digest(&md5, sizeof(md5Expected), output);
  if (memcmp(output, md5Expected, sizeof(md5Expected)) != 0)
    return 1;

  for (size_t length : {size_t(8), size_t(16), sizeof(sha1Expected)}) {
    sha1_ctx sha1;
    sha1_init(&sha1);
    sha1_update(&sha1, sizeof(input), input);
    rfb::nettleSha1Digest(&sha1, length, output);
    if (memcmp(output, sha1Expected, length) != 0)
      return 1;
  }

  sha256_ctx sha256;
  sha256_init(&sha256);
  sha256_update(&sha256, sizeof(input), input);
  rfb::nettleSha256Digest(&sha256, sizeof(sha256Expected), output);
  return memcmp(output, sha256Expected, sizeof(sha256Expected)) != 0;
}
