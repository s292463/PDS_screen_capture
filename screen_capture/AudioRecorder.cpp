#include "AudioRecorder.h"
#include <thread>
#include <mutex>
#include <condition_variable>

#ifdef __linux__
#include <assert.h>
#endif

const AVSampleFormat requireAudioFmt = AV_SAMPLE_FMT_FLTP;

void AudioRecorder::Open()
{
    // input context
    AVDictionary *options = nullptr;
    audioInFormatCtx = nullptr;
    int ret;

    //ref: https://ffmpeg.org/ffmpeg-devices.html
#ifdef _WIN32
    if (deviceName == "") {
        deviceName = DS_GetDefaultDevice("a");
            //"@device_cm_{33D9A762-90C8-11D0-BD43-00A0C911CE86}\\wave_{9FF96969-7CB9-458F-8407-EB85A9265599}";
        if (deviceName == "") {
            throw std::runtime_error("Fail to get default audio device, maybe no microphone.");
        }
    }
    deviceName = "audio=" + deviceName;
    const AVInputFormat *inputFormat = av_find_input_format("dshow");
/*#elif MACOS
    if(deviceName == "") deviceName = ":0";
    AVInputFormat *inputFormat = av_find_input_format("avfoundation");
    //"[[VIDEO]:[AUDIO]]"
#elif UNIX
    if(deviceName == "") deviceName = "default";
    AVInputFormat *inputFormat=av_find_input_format("pulse");
*/
#endif


    ret = avformat_open_input(&audioInFormatCtx, deviceName.c_str(), inputFormat, &options);
    if (ret != 0) {
        throw std::runtime_error("Couldn't open input audio stream.");
    }

    if (avformat_find_stream_info(audioInFormatCtx, nullptr) < 0) {
        throw std::runtime_error("Couldn't find audio stream information.");
    }

    audioInStream = nullptr;
    for (unsigned int i = 0; i < audioInFormatCtx->nb_streams; i++) {
        if (audioInFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioInStream = audioInFormatCtx->streams[i];
            break;
        }
    }
    if (!audioInStream) {
        throw std::runtime_error("Couldn't find a audio stream.");
    }

    const AVCodec *audioInCodec = avcodec_find_decoder(audioInStream->codecpar->codec_id);
    audioInCodecCtx = avcodec_alloc_context3(audioInCodec);
    avcodec_parameters_to_context(audioInCodecCtx, audioInStream->codecpar);

    if (avcodec_open2(audioInCodecCtx, audioInCodec, nullptr) < 0) throw std::runtime_error("Could not open video codec.");

    // audio converter, convert other fmt to requireAudioFmt
    audioConverter = swr_alloc_set_opts(nullptr,
                                        av_get_default_channel_layout(audioInCodecCtx->channels),
                                        requireAudioFmt,  // aac encoder only receive this format
                                        audioInCodecCtx->sample_rate,
                                        av_get_default_channel_layout(audioInCodecCtx->channels),
                                        (AVSampleFormat)audioInStream->codecpar->format,
                                        audioInStream->codecpar->sample_rate,
                                        0, nullptr);
    swr_init(audioConverter);

    // 2 seconds FIFO buffer
    audioFifo = av_audio_fifo_alloc(requireAudioFmt, audioInCodecCtx->channels,
                                    audioInCodecCtx->sample_rate * 2);

}

void AudioRecorder::initializeEncoder(AVFormatContext* outputFormatContext) {

    this->audioOutFormatCtx = outputFormatContext;

    const AVCodec* audioOutCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!audioOutCodec) {
        throw std::runtime_error("Fail to find aac encoder. Please check your DLL.");
    }

    audioOutCodecCtx = avcodec_alloc_context3(audioOutCodec);
    audioOutCodecCtx->channels = audioInStream->codecpar->channels;
    audioOutCodecCtx->channel_layout = av_get_default_channel_layout(audioInStream->codecpar->channels);
    audioOutCodecCtx->sample_rate = audioInStream->codecpar->sample_rate;
    audioOutCodecCtx->sample_fmt = audioOutCodec->sample_fmts[0];  //for aac , there is AV_SAMPLE_FMT_FLTP =8
    audioOutCodecCtx->bit_rate = 32000;
    audioOutCodecCtx->time_base.num = 1;
    audioOutCodecCtx->time_base.den = audioOutCodecCtx->sample_rate;

    if (audioOutFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
        audioOutCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (avcodec_open2(audioOutCodecCtx, audioOutCodec, NULL) < 0)
    {
        throw std::runtime_error("Fail to open ouput audio encoder.");
    }

    //Add a new stream to output,should be called by the user before avformat_write_header() for muxing
    audioOutStream = avformat_new_stream(audioOutFormatCtx, audioOutCodec);

    if (audioOutStream == NULL)
    {
        throw std::runtime_error("Fail to new a audio stream.");
    }
    avcodec_parameters_from_context(audioOutStream->codecpar, audioOutCodecCtx);

}

void AudioRecorder::StartEncode(std::mutex& w_m, std::mutex& s_m, std::condition_variable& s_cv, std::atomic_bool& isStopped)
{
    AVFrame *inputFrame = av_frame_alloc();
    AVPacket *inputPacket = av_packet_alloc();

    AVPacket *outputPacket = av_packet_alloc();
    uint64_t  frameCount = 0;

    int ret;
    std::mutex test_mutex;
    while (isRun->load()) {
        std::unique_lock s_ul{ test_mutex };
        s_cv.wait(s_ul, [&isStopped] { return !isStopped.load(); });

        //  decoding
        ret = av_read_frame(audioInFormatCtx, inputPacket);
        if (ret < 0) {
            throw std::runtime_error("can not read frame");
        }
        ret = avcodec_send_packet(audioInCodecCtx, inputPacket);
        if (ret < 0) {
            throw std::runtime_error("can not send pkt in decoding");
        }
        ret = avcodec_receive_frame(audioInCodecCtx, inputFrame);
        if (ret < 0) {
            throw std::runtime_error("can not receive frame in decoding");
        }
        //--------------------------------
        // encoding

        uint8_t **cSamples = nullptr;
        ret = av_samples_alloc_array_and_samples(&cSamples, NULL, audioOutCodecCtx->channels, inputFrame->nb_samples, requireAudioFmt, 0);
        if (ret < 0) {
            throw std::runtime_error("Fail to alloc samples by av_samples_alloc_array_and_samples.");
        }
        ret = swr_convert(audioConverter, cSamples, inputFrame->nb_samples, (const uint8_t**)inputFrame->extended_data, inputFrame->nb_samples);
        if (ret < 0) {
            throw std::runtime_error("Fail to swr_convert.");
        }
        if (av_audio_fifo_space(audioFifo) < inputFrame->nb_samples) {
            throw std::runtime_error("audio buffer is too small.");
        }

        ret = av_audio_fifo_write(audioFifo, (void**)cSamples, inputFrame->nb_samples);
        if (ret < 0) {
            throw std::runtime_error("Fail to write fifo");
        }

        av_freep(&cSamples[0]);

        av_frame_unref(inputFrame);
        av_packet_unref(inputPacket);

        // frame_size -> n di sample in un frame audio
        while (av_audio_fifo_size(audioFifo) >= audioOutCodecCtx->frame_size) {
            AVFrame* outputFrame = av_frame_alloc();
            outputFrame->nb_samples = audioOutCodecCtx->frame_size;
            outputFrame->channels = audioInCodecCtx->channels;
            outputFrame->channel_layout = av_get_default_channel_layout(audioInCodecCtx->channels);
            outputFrame->format = requireAudioFmt;
            outputFrame->sample_rate = audioOutCodecCtx->sample_rate;

            ret = av_frame_get_buffer(outputFrame, 0);
            /*assert(ret >= 0);*/
            ret = av_audio_fifo_read(audioFifo, (void**)outputFrame->data, audioOutCodecCtx->frame_size);
            /*assert(ret >= 0);*/

            outputFrame->pts = frameCount * audioOutStream->time_base.den * 1024 / audioOutCodecCtx->sample_rate;

            ret = avcodec_send_frame(audioOutCodecCtx, outputFrame);
            if (ret < 0) {
                throw std::runtime_error("Fail to send frame in encoding");
            }
            av_frame_free(&outputFrame);
            ret = avcodec_receive_packet(audioOutCodecCtx, outputPacket);
            if (ret == AVERROR(EAGAIN)) {
                continue;
            }
            else if (ret < 0) {
                throw std::runtime_error("Fail to receive packet in encoding");
            }


            outputPacket->stream_index = audioOutStream->index;
            outputPacket->duration = audioOutStream->time_base.den * 1024 / audioOutCodecCtx->sample_rate;
            outputPacket->dts = outputPacket->pts =frameCount * audioOutStream->time_base.den * 1024 / audioOutCodecCtx->sample_rate;

            frameCount++;


            // Critic section
            std::unique_lock ul(w_m);

            ret = av_interleaved_write_frame(audioOutFormatCtx, outputPacket);

            ul.unlock();
            // End of Critic section
            av_packet_unref(outputPacket);

        }
    }

    av_packet_free(&inputPacket);
    av_packet_free(&outputPacket);
    av_frame_free(&inputFrame);
    printf("encode %llu audio packets in total.\n", frameCount);
}
