#include "web_limits.h"

bool webPostBodyTooLarge(long contentLength, uint32_t capBytes) {
  if (contentLength < 0) {
    return true;
  }
  return (uint32_t)contentLength > capBytes;
}
