#include <string.h>
#include "platform.h"
#include "video_providers.h"

static char *protocol_strings[N_INSTANCE_TYPES][N_PROTOCOL_STRINGS] = {
  { /* PEERTUBE */
    "%s/api/v1/config/about",
    "%s/api/v1/search/videos?sort=-match&search=%s",
    "%s/api/v1/videos/%s",
    "%s/api/v1/videos/%s/captions",

    /* selector tweaked to fit both Peertube and SepiaSearch API responses:
     * .previewUrl is available from SepiaSearch and allows easy getting of thumbnail from final peertube host,
     * if not, .previewPath is available from Peertube servers.
     * .account.host is the easiest way to fetch the final peertube host for Sepiasearch, and is useless but
     * harmless for Peertube. */
    ".data[]|.name,.uuid,.account.displayName,.previewUrl//.previewPath,.duration,\"https://\"+.account.host",

    /* Concatenate HLS files and normal files that have a resolution (a video track), sort by size, get
     * the less big one. */
    "[.files+.streamingPlaylists[0].files|.[]|select(.resolution.id > 0)]|sort_by(.size)|first|.fileUrl",

    /* Get captions, first the one in the user's preferred language, then the first one.
     * It could be two, it could be the same, or it could be zero. LG is a placeholder and
     * will be replaced by the user's preferred language. fileUrl is preferred as captionPath
     * is deprecated. */
    "(.data[]|select(.language.id == \"LG\")|.fileUrl//.captionPath),.data[0].fileUrl//.data[0].captionPath"
  },

  { /* SEPIASEARCH */
    "%s/api/v1/config",
    "%s/api/v1/search/videos?count=10&search=%s",
    NULL, /* Sepiasearch aggregates results from Peertube instances, */
    NULL, /* So the rest of endpoints & json selectors are Peertube's. */

    NULL,
    NULL,
    NULL
  },

  { /* INVIDIOUS */
    "%s/api/v1/trending",
    "%s/api/v1/search?type=video&sort=relevance&q=%s",
    "%s/api/v1/videos/%s?local=true",
    "%s/api/v1/captions/%s",

    /* Fetch '-' as host so we keep using the user-defined instance. */
    ".[]|.title,.videoId,.author,(.videoThumbnails[]|select(.quality == \"medium\")|.url),.lengthSeconds,\"-\"",

    /* Do not try to get .adaptiveFormats, we would need to use a different one for video and audio,
     * which is not yet supported proxy-side. */
    ".formatStreams[]|select(.itag==\"18\").url",

    /* Get captions, first the one in the user's preferred language, then the first one.
     * It could be two, it could be the same, or it could be zero. */
    "(.captions[]|select(.lang==\"LG\")|.url),.captions[0].url"
  }
};

char *video_provider_get_protocol_string(InstanceTypeId instance_type, ProtocolStringId str_type) {
  static char *s;

  s = protocol_strings[instance_type][str_type];

  /* No endpoint, fallback to parent instance type */
  if (IS_NULL(s)) {
    switch(instance_type) {
      /* SepiaSearch fallback: Peertube */
      case SEPIASEARCH:
        s = protocol_strings[PEERTUBE][str_type];
        break;
    }
  }

  return s;
}
