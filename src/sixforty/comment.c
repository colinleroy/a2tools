#include <stdlib.h>
#include "api.h"

void comment_free(comment_t *comment) {
  if (IS_NULL(comment)) {
    return;
  }
  free(comment->text);
  free(comment->author);
  free(comment->date);
  free(comment);
}
