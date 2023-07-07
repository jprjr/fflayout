# fflayout

A quick and dirty tool to inspect an audio stream's channel layout,
including assigned channels.

While working on some software I wanted to ensure I was getting
channel orders correct, I don't have a surround-sound system so
I wanted to extract each audio channel to a separate, labeled mono file.

This is pretty easy to do with ffmpeg:

```
# from https://trac.ffmpeg.org/wiki/AudioChannelManipulation#a5.16mono

ffmpeg -i in.wav \
-filter_complex "channelsplit=channel_layout=5.1[FL][FR][FC][LFE][BL][BR]" \
-map "[FL]" front_left.wav \
-map "[FR]" front_right.wav \
-map "[FC]" front_center.wav \
-map "[LFE]" lfe.wav \
-map "[BL]" back_left.wav \
-map "[BR]" back_right.wav
```

I can't remember which channels are present in each layout, though.
You can get the list of layouts with `ffmpeg -layouts` but I thought
it'd be nice to have a tool that puts it all together in one shot.

If you run:

```
fflayout in.wav
```

You'll receive output like:

```
channel count: 6
channel order: native
channel layout: 5.1
channel 0: FL
channel 1: FR
channel 2: FC
channel 3: LFE
channel 4: BL
channel 5: BR
```

You can also get JSON output by adding the `--json` flag:

```
{
  "channel_count":6,
  "channel_order":"native",
  "channel_layout":"5.1",
  "channels": [
    "FL",
    "FR",
    "FC",
    "LFE",
    "BL",
    "BR"
  ]
}
```

and CSV output with the `--csv` flag:

```
"field","value"
"channel_count",6
"channel_order","native"
"channel_layout":"5.1"
"channel_0","FL"
"channel_1","FR"
"channel_2","FC"
"channel_3","LFE"
"channel_4","BL"
"channel_5","BR"
```

and finally if you're super lazy: `--shell` will generate a shell command for splitting the
output into separate files. This assumes you're using a POSIX shell.

(I've broken this output onto multiple lines for legibility, actual output is all one line):

```
# result from running fflayout --shell test.wav
ffmpeg -i 'test.wav' \
  -filter_complex "[0:a:0]channelsplit=channel_layout=5.1[FL][FR][FC][LFE][BL][BR]" \
  -map '[FL]' 'FL.wav' \
  -map '[FR]' 'FR.wav' \
  -map '[FC]' 'FC.wav' \
  -map '[LFE]' 'LFE.wav' \
  -map '[BL]' 'BL.wav' \
  -map '[BR]' 'BR.wav'
```

Note that `--shell` doesn't check your input filename - if it contains special
characters you may have a bad time, always check the generated script before
running it!

Or if you know your filename is fine you could run something like:

```
eval $(fflayout --shell test.wav)
```

If you have a file with multiple audio tracks, you can specify which audio track
as an extra parameter after the filename:

```
fflayout test.mkv 0 # grabs the first audio track
fflayout test.mkv 1 # grabs the second audio track
```

An important thing to note is the channel order is usually going to be the order
that channels are decoded as internally by ffmpeg. If you're trying to say,
use libopus directly to decode a surround-sound Opus file, that uses a different
channel ordering. ffmpeg abstracts all that away and remaps Opus channel ordering
into ffmpeg's ordering.

The main use of this tool is to pair it with ffmpeg, the channel names and ordering
will all match up correctly as long as you do that.

# LICENSE

BSD Zero Clause, see file LICENSE.

