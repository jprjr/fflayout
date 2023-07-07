#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>

#include <stdio.h>
#include <stddef.h>
#include <string.h>

static const char *progname = NULL;

typedef enum {
    PLAIN,
    JSON,
    CSV,
    SHELL,
} output_mode;

int usage(int e) {
    fprintf(stderr,"Usage: %s [--json | --csv | --shell] url [audio index]\n", progname);
    return e;
}

static const char *const order_names[] = {
    "unknown",
    "unspecified",
    "native",
    "custom",
    "ambisonics",
};

static const char* order_to_str(enum AVChannelOrder order) {
    switch(order) {
        case AV_CHANNEL_ORDER_UNSPEC: return order_names[1];
        case AV_CHANNEL_ORDER_NATIVE: return order_names[2];
        case AV_CHANNEL_ORDER_CUSTOM: return order_names[3];
        case AV_CHANNEL_ORDER_AMBISONIC: return order_names[4];
        default: break;
    }
    return order_names[0];
}

int main(int argc, const char* argv[]) {
    int err;
    char buf[512];
    int idx = -1;
    AVFormatContext *avFormatContext = NULL;
    const AVCodec *avCodec = NULL;
    AVChannelLayout *ch_layout = NULL;
    enum AVChannel channel;
    int i;
    int j = 0;
    int r = 1;
    output_mode omode = PLAIN;

    const char *url = NULL;
    const char *streamid = NULL;
    int flag = 1;

    progname = *argv;
    argv++;
    argc--;

    /* handle any flags */
    while(argc && flag) {
        if(strcmp(*argv,"--") == 0) {
            flag = 0;
        } else if(strcmp(*argv,"--json") == 0) {
            omode = JSON;
        } else if(strcmp(*argv,"--csv") == 0) {
            omode = CSV;
        } else if(strcmp(*argv,"--shell") == 0) {
            omode = SHELL;
        } else {
            break;
        }
        argc--;
        argv++;
    }

    while(argc) {
        if(url == NULL) {
            url = *argv;
        } else if(streamid == NULL) {
            streamid = *argv;
        } else {
            break;
        }
        argc--;
        argv++;
    }

    if(url == NULL) return usage(1);

    if(streamid != NULL) {
        idx = atoi(streamid);
    }

    if( (err = avformat_open_input(&avFormatContext, url, NULL, NULL)) < 0) goto av_error;

    if( (err = avformat_find_stream_info(avFormatContext, NULL)) < 0) goto av_error;

    if(idx < 0) {
        if( (err = av_find_best_stream(avFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &avCodec, 0)) < 0) goto av_error;
        idx = err;
        /* figure out the audio track # */

        for(i=0;i<(int)avFormatContext->nb_streams;i++) {
            if(avFormatContext->streams[i]->codecpar->codec_type != AVMEDIA_TYPE_AUDIO) continue;
            if(i == idx) break;
            j++;
        }

    } else {
        for(i=0;i<(int)avFormatContext->nb_streams;i++) {
            if(avFormatContext->streams[i]->codecpar->codec_type != AVMEDIA_TYPE_AUDIO) continue;
            if(j == idx) {
                idx = i;
                break;
            }
            j++;
        }
        if(i == (int)avFormatContext->nb_streams) {
            fprintf(stderr,"unable to find stream\n");
            goto cleanup;
        }
    }

    ch_layout = &avFormatContext->streams[idx]->codecpar->ch_layout;

    if( (err = av_channel_layout_describe(ch_layout, buf, sizeof(buf))) < 0) goto av_error;

    switch(omode) {
        case PLAIN: {
            fprintf(stdout,"channel count: %d\n", ch_layout->nb_channels);
            fprintf(stdout,"channel order: %s\n",order_to_str(ch_layout->order));
            fprintf(stdout,"channel layout: %s\n",buf);
            break;
        }
        case JSON: {
            fprintf(stdout,"{\n");
            fprintf(stdout,"  \"channel_count\":%d,\n",ch_layout->nb_channels);
            fprintf(stdout,"  \"channel_order\":\"%s\",\n",order_to_str(ch_layout->order));
            fprintf(stdout,"  \"channel_layout\":\"%s\",\n",buf);
            fprintf(stdout,"  \"channels\": [");
            break;
        }
        case CSV: {
            fprintf(stdout,"\"field\",\"value\"\n");
            fprintf(stdout,"\"channel_count\",%d\n",ch_layout->nb_channels);
            fprintf(stdout,"\"channel_order\",\"%s\"\n",order_to_str(ch_layout->order));
            fprintf(stdout,"\"channel_layout\":\"%s\"\n",buf);
            break;
        }
        case SHELL: {
            fprintf(stdout,"ffmpeg -i '%s'",url);
            fprintf(stdout," -filter_complex \"[0:a:%d]",j);
            fprintf(stdout,"channelsplit=channel_layout=%s",buf);
            break;
        }
    }


    for(i=0;i<ch_layout->nb_channels;i++) {
        channel = av_channel_layout_channel_from_index(ch_layout,i);
        if( (err = av_channel_name(buf,sizeof(buf),channel)) < 0) goto av_error;

        switch(omode) {
            case PLAIN: {
                fprintf(stdout,"channel %d: %s\n", i, buf);
                break;
            }
            case JSON: {
                if(i > 0) {
                    fprintf(stdout,",");
                }
                fprintf(stdout,"\n    \"%s\"", buf);
                break;
            }
            case CSV: {
                fprintf(stdout,"\"channel_%d\",\"%s\"\n",i,buf);
                break;
            }
            case SHELL: {
                fprintf(stdout,"[%s]",buf);
                break;
            }
        }
    }

    if(omode == JSON) {
        fprintf(stdout,"\n  ]\n}\n");
    } else if(omode == SHELL) {
        fprintf(stdout,"\"");
        for(i=0;i<ch_layout->nb_channels;i++) {
            channel = av_channel_layout_channel_from_index(ch_layout,i);
            if( (err = av_channel_name(buf,sizeof(buf),channel)) < 0) goto av_error;

            fprintf(stdout," -map '[%s]' '%s.wav'",buf,buf);
        }
        fprintf(stdout,"\n");
    }

    r = 0;
    goto cleanup;

    av_error:
    av_strerror(err,buf,sizeof(buf));
    fprintf(stderr,"error: %s\n",buf);

    cleanup:
    if(avFormatContext != NULL) avformat_close_input(&avFormatContext);

    return r;
}
