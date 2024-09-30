#ifndef __VIDEO_PROVIDERS_H__
#define __VIDEO_PROVIDERS_H__

typedef enum {                  /* Warning: order instances type so that the api check succeeds correctly */
  PEERTUBE,                     /* when instance is checked against type 0..N. For example, if SEPIASEARCH */
  SEPIASEARCH,                  /* was before PEERTUBE, all Peertube servers would be recognized as SEPIASEARCH */
  INVIDIOUS,                    /* because Peertube servers also have an /api/v1/config endpoint. */
  N_INSTANCE_TYPES
} InstanceTypeId;

typedef enum {                  /* Indexes of fields in VIDEO_DETAILS_JSON_SELECTOR */
  VIDEO_NAME,
  VIDEO_ID,
  VIDEO_AUTHOR,
  VIDEO_THUMB,
  VIDEO_LENGTH,
  VIDEO_HOST,
  N_VIDEO_DETAILS
} VideoDetailFieldId;

typedef enum {
  API_CHECK_ENDPOINT,           /* Verify whether a server is of a given type; format %s, user-defined host */
  VIDEO_SEARCH_ENDPOINT,        /* Search; format %s %s, user-defined host and user search string */
  VIDEO_DETAILS_ENDPOINT,       /* Details; format %s %s, video host and video ID */
  VIDEO_CAPTIONS_ENDPOINT,      /* Captions; format %s %s, video host and video ID */

  VIDEO_DETAILS_JSON_SELECTOR,  /* Selector for video details to fetch. We expect to get N_VIDEO_DETAILS
                                 * fields, in the following order:
                                 * Video Name (example: "Bad Apple"),
                                 * Video Id (example: FtutLA63Cp8),
                                 * Video Author (example: kasidid2),
                                 * Video thumbnail (example: /vi/FtutLA63Cp8/mqdefault.jpg),
                                 * Video length in seconds (example: 220),
                                 * Video host, either '-' if the video is on the same host the search 
                                 *             was performed on, or https://another-host.example.com.
                                 *             This field is used for SepiaSearch. This host will be
                                 *             used to send the VIDEO_DETAILS and CAPTIONS requests. */

  VIDEO_URL_JSON_SELECTOR,       /* Selector for video data URL to stream. A JSON query to return one
                                  * single string, an absolute URL with or without a host part (if
                                  * there is no host, the one from VIDEO_DETAILS_JSON_SELECTOR will
                                  * be used.) */

  CAPTIONS_JSON_SELECTOR,        /* Selector for captions URL. A JSON query to return one
                                  * single string, an absolute URL with or without a host part (if
                                  * there is no host, the one from VIDEO_DETAILS_JSON_SELECTOR will
                                  * be used.) */
  N_PROTOCOL_STRINGS
} ProtocolStringId;

char *video_provider_get_protocol_string(InstanceTypeId instance_type, ProtocolStringId str_type);
  
#endif
