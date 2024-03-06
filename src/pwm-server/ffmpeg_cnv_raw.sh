ffmpeg -i $1 -ar 11520 -f u8 -acodec pcm_u8 -ac 1 output.raw
