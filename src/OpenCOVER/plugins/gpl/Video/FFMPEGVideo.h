#include <osgViewer/GraphicsWindow>

extern "C" {
#ifdef HAVE_FFMPEG_SEPARATE_INCLUDES
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>
#include <libswscale/swscale.h>
#define HAVE_SWSCALE_H
#else
#include <ffmpeg/avcodec.h>
#include <ffmpeg/avformat.h>
#include <ffmpeg/avutil.h>
#define AV_VERSION_INT(a, b, c) (a << 16 | b << 8 | c)
#ifdef LIBAVCODEC_VERSION_INT
#if (LIBAVCODEC_VERSION_INT > AV_VERSION_INT(51, 9, 0))
#include <ffmpeg/swscale.h>
#define HAVE_SWSCALE_H
#endif
#endif
#endif
};

#include <xercesc/dom/DOM.hpp>

#include "Video.h"

#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_PIX_FMT PIX_FMT_YUV420P /* default pix_fmt */

typedef std::list<AVCodec *> AVCodecList;

typedef struct
{
    std::string name;
    std::string fps;
    int constFrames;
    int avgBitrate;
    int maxBitrate;
    int width;
    int height;
} VideoParameter;

class FFMPEGPlugin : public coVRPlugin, public coTUIListener
{
public:
    FFMPEGPlugin();
    ~FFMPEGPlugin();

    VideoPlugin *myPlugin;
    coTUIComboBox *selectCodec;
    coTUIComboBox *selectParams;

    friend class VideoPlugin;

private:
    std::string filterList;
    std::map<AVOutputFormat *, AVCodecList> formatList;
    std::list<VideoParameter> VPList;

    void tabletEvent(coTUIElement *);
    void Menu(int row);
    void ParamMenu(int row);
    void hideParamMenu(bool hide);
    void ListFormatsAndCodecs(const std::string &);
    void FillComboBoxSetExtension(int selection, int row);
    void changeFormat(coTUIElement *, int row);
    void checkFileFormat(const string &name);
    bool videoCaptureInit(const string &filename, int format, int RGBFormat);
    void videoWrite(int format = 0);
    bool FFMPEGInit(AVOutputFormat *outfmt, AVCodec *codec, const string &filename, bool test_codecs = false);
    void FFMPEGFormat(bool registerFormat);
    void init_GLbuffers();
    bool open_codec(AVCodec *codec);
    bool open_video(AVCodec *codec);
    void unInitialize();
    void close_video();
    void close_all(bool stream = false, int format = 0);
    AVFrame *alloc_picture(PixelFormat pix_fmt, int width, int height);
    bool add_video_stream(AVCodec *codec, int w, int h, int frame_base, int bitrate, int maxBitrate);
    void video_tag(const char *cname);
    int SwConvertScale(int width, int height);
    int ImgConvertScale(int width, int height);
    int readParams();
    void loadParams(int);
    void saveParams();
    void addParams();
    void fillParamComboBox(int);
    void sendParams();
    int getParams();

    AVOutputFormat *fmt;
    AVFormatContext *oc;
    AVStream *video_st;
    AVCodecContext *codecCtx;
    AVPacket pkt;

    AVFrame *picture, *inPicture, *outPicture;
#ifdef HAVE_SWSCALE_H
    SwsContext *swsconvertctx;
#else
    ImgReSampleContext *imgresamplectx;
#endif
    uint8_t *video_outbuf;
    int video_outbuf_size;
    uint8_t *mirroredpixels;
    int linesize;

    coTUIEditField *paramNameField;
    coTUILabel *paramErrorLabel;
    coTUILabel *paramLabel;
    coTUILabel *bitrateLabel;
    coTUILabel *maxBitrateLabel;
    coTUIEditIntField *bitrateField;
    coTUIEditIntField *maxBitrateField;
    coTUIToggleButton *saveButton;

    xercesc::DOMImplementation *impl;
};
