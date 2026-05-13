#include <stdlib.h>
#include "api.h"

#pragma code-name(push, "LC")

void post_free(post_t *post) {
  if (IS_NULL(post)) {
    return;
  }
  free(post->id);
  free(post->image_url);
  free(post->description);
  free(post->author);
  free(post->date);
  free(post);
}
