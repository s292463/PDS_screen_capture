#include "VideoRecorder.h"
#include <atomic>
#include <utility>


// CONSTRUCTOR AND DISTRUCTOR =================================================
VideoRecorder::VideoRecorder(std::string outputFileName, std::string framerate, std::pair<std::string, std::string> offset, std::pair<std::string, std::string> resolution, std::atomic_bool* isRun):
	outputFileName(outputFileName),
	inputFileName("gdigrab"),
	audioOn(true),
	framerate(framerate),
	res(resolution),
	offset{ offset },
	stream_index{ 0,0 },
	failReason(""),
	isRun(isRun)
{
	avdevice_register_all(); // It's not thread safe
}

VideoRecorder::~VideoRecorder() {


	// Close format context input
	if (inputFormatContext)
		avformat_close_input(&inputFormatContext);

	// Close codec Context
	if (videoEncoderContext) {
		avcodec_close(videoEncoderContext);
		avcodec_free_context(&videoEncoderContext);
	}
	if (videoDecoderContext) {
		avcodec_close(videoDecoderContext);
		avcodec_free_context(&videoDecoderContext);
	}
}

// FUNCTIONS ===============================================================

void VideoRecorder::Open() {
	intilizeDecoder();
}


void VideoRecorder::intilizeDecoder() {
    this->inputFormat = nullptr;
#ifdef _WIN32
    this->inputFormat = av_find_input_format("gdigrab");
	if (!this->inputFormat) {
		throw std::runtime_error("Error in opening input device");
	}
#elif linux
    this->inputFormat = av_find_input_format("x11grab");
    if (!this->inputFormat) {
        throw std::runtime_error("Error in opening input device");
    }
#endif


	// Option dictionary
	av_dict_set(&options, "framerate", this->framerate.c_str(), 0);
	av_dict_set(&options, "preset", "medium", 0);
	av_dict_set(&options, "offset_x", this->offset.first.c_str(), 0);
	av_dict_set(&options, "offset_y", this->offset.second.c_str(), 0);
	av_dict_set(&options, "video_size", (this->res.first + "x" + this->res.second).c_str(), 0);
	av_dict_set(&options, "probesize", "20M", 0);


	// alloc AVFormatContext input
	inputFormatContext = avformat_alloc_context();
	if (!avformat_alloc_context()) {
		throw std::runtime_error("Can't allocate input context");
	}
#ifdef _WIN32
	if (avformat_open_input(&inputFormatContext, "desktop", const_cast<AVInputFormat*>(this->inputFormat), &options) < 0)
    {
		throw std::runtime_error("Can't open input context");
	}
#elif linux
    if(avformat_open_input(&inputFormatContext, ":0.0+10,250", const_cast<AVInputFormat*>(this->inputFormat), &options) != 0)
    {
        throw std::runtime_error("Can't open input context");
    }
#endif

    //Read packets of a media file to get stream information.
	if (avformat_find_stream_info(inputFormatContext, &options) < 0)
	{
		throw std::runtime_error("\nunable to find the stream information");
		
	}

	for (unsigned int i = 0; i < inputFormatContext->nb_streams; i++)
	{
		// Estraiamo i parametri del codec dello stream dati (audio/video)
		AVMediaType type = inputFormatContext->streams[i]->codecpar->codec_type;

		// codec_type definisce il tipo di stream su cui viene applicato il codec
		// codec_id definisce il tipo di codec in cui e'codificato lo stream 
		if (type == AVMEDIA_TYPE_VIDEO)
		{
			// Salvo l'indice del tipo di stream 
			stream_index[StreamType::VIDEO] = i;

			/* Decoder setup */
			// Estraiamo i parametri del codec dello stream dati (audio/video)
			// Usiamo l' ID del codec per trovare il decoder che ci serve
			videoDecoder = avcodec_find_decoder(inputFormatContext->streams[i]->codecpar->codec_id);
			if (!videoDecoder) // Eseguo check
			{
				throw std::runtime_error("\nUnable to find the Decoder");
	
			}

			inStream = inputFormatContext->streams[stream_index[StreamType::VIDEO]];

			break;

		}
	}

	// Creazione del codec context a partire dal decoder e riempito con i parametri del codec
	videoDecoderContext = avcodec_alloc_context3(videoDecoder);
	if (!videoDecoderContext)
	{
		throw std::runtime_error("\nUnable to allocate the decoder context");
	}

	if (avcodec_parameters_to_context(videoDecoderContext, inputFormatContext->streams[stream_index[0]]->codecpar) < 0)
	{
		throw std::runtime_error("\nUnable to set the parameter of the codec");
	}

	// Inizializziamo il Decoder Context con il codec aprendolo
	if (avcodec_open2(videoDecoderContext, videoDecoder, NULL) < 0)
	{
		throw std::runtime_error("\nUnable to open the av codec");
	}
    // TODO eliminare "inputFileName"
	//Stampo le informazioni dell' input format context
	av_dump_format(inputFormatContext, 0, this->inputFileName.c_str(), 0);

}

void VideoRecorder::Reopen() {
// Option dictionary
    av_dict_set(&options, "framerate", this->framerate.c_str(), 0);
    av_dict_set(&options, "preset", "medium", 0);
    av_dict_set(&options, "offset_x", this->offset.first.c_str(), 0);
    av_dict_set(&options, "offset_y", this->offset.second.c_str(), 0);
    av_dict_set(&options, "video_size", (this->res.first + "x" + this->res.second).c_str(), 0);
    av_dict_set(&options, "probesize", "20M", 0);
#ifdef _WIN32
    if (avformat_open_input(&inputFormatContext, "desktop", const_cast<AVInputFormat*>(this->inputFormat), &options) < 0) {
		throw std::runtime_error("Can't open input context");
	}
#elif linux
    if(avformat_open_input(&inputFormatContext, ":0.0+10,250", const_cast<AVInputFormat*>(this->inputFormat), &options) < 0){
        throw std::runtime_error("Can't open input context");
    }

#endif
}

void VideoRecorder::initializeEncoder(AVFormatContext* outputFormatContext)
{
	this->outputFormatContext = outputFormatContext;
	
	//-------------------encoder------------------------------
	videoEncoder = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
	if (!videoEncoder) // Eseguo check
	{
		throw std::runtime_error("\nUnable to find the encoder");
	}

	videoEncoderContext = avcodec_alloc_context3(videoEncoder);
	if (!videoEncoderContext) // Eseguo check
	{
		throw std::runtime_error("\nUnable to allocate the encoder context");
	}

	// Encoder Settings
	//videoEncoderContext->codec_id = AV_CODEC_ID_MPEG4;
	videoEncoderContext->pix_fmt = AV_PIX_FMT_YUV420P;
	//videoEncoderContext->sample_aspect_ratio = videoDecoderContext->sample_aspect_ratio;
	videoEncoderContext->height = std::stoi(this->res.second);// videoDecoderContext->height;
	videoEncoderContext->width = std::stoi(this->res.first);// videoDecoderContext->width;
	videoEncoderContext->gop_size = 3;
	videoEncoderContext->max_b_frames = 2;
	videoEncoderContext->time_base.num = 1;
	videoEncoderContext->time_base.den = 15; // 15 fps
	//videoEncoderContext->time_base = inStream->time_base;
	videoEncoderContext->bit_rate = 4000000;


	if (avcodec_open2(videoEncoderContext, videoEncoder, nullptr) < 0) //Effettuo un check
	{
		throw std::runtime_error("\nError in opening the Encoder");
	}

	//------------------------------------------------------------------------------------
	
	// associo uno stream video all'outputFormatContext
	outStream = avformat_new_stream(outputFormatContext, videoEncoder);
	if (!outStream) {
		throw std::runtime_error("Failed allocating output stream\n");
	}

	if (videoEncoderContext->codec_id == AV_CODEC_ID_H264)
	{
		av_opt_set(videoEncoderContext->priv_data, "preset", "slow", 0);
	}

	if (avcodec_parameters_from_context(outStream->codecpar, videoEncoderContext) < 0) // Eseguo check
	{
		throw std::runtime_error("\nUnable to set the parameter of the codec");
	}

	//outStream->time_base = { 1, 30 };
	outStream->time_base = videoEncoderContext->time_base;

	//We set the flag AV_CODEC_FLAG_GLOBAL_HEADER which tells the encoder that it can use the global headers
	if (outputFormatContext->oformat->flags & AVFMT_GLOBALHEADER) {
		videoEncoderContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	//Effettuo un check sul numero di stream
	if (!outputFormatContext->nb_streams)
	{
		throw std::runtime_error("\nOutput file dose not contain any stream");
	}

}

void VideoRecorder::startCapturing(std::mutex& write_mutex, std::condition_variable& s_cv, std::atomic_bool& isStopped) {

	AVPacket* inPacket = av_packet_alloc();
	if (!inPacket) { std::cout << "Error on allocating input Packet"; exit(1); }
	AVPacket* outPacket = av_packet_alloc();
	if (!outPacket) { std::cout << "Error on allocating output Packet"; exit(1); }
	AVFrame* inFrame = av_frame_alloc();
	if (!inFrame) { std::cout << "Error on allocating input Frame"; exit(1); }
	AVFrame* outFrame = av_frame_alloc();
	if (!outFrame) { std::cout << "Error on allocating output Frame"; exit(1); }


	int nbytes = av_image_get_buffer_size(videoEncoderContext->pix_fmt, videoEncoderContext->width, videoEncoderContext->height, 32);

	uint8_t* video_outbuf = (uint8_t*)av_malloc(nbytes);
	if (!video_outbuf)
	{
		throw std::runtime_error("\nUnable to allocate memory");
	}

	if (av_image_fill_arrays(outFrame->data, outFrame->linesize, video_outbuf, AV_PIX_FMT_YUV420P, videoEncoderContext->width, videoEncoderContext->height, 1) < 0) //Verifico che non ci siano errori
	{
		throw std::runtime_error("\nError in filling image array");
	}


	struct SwsContext* swsCtx_;

	if (!(swsCtx_ = sws_alloc_context()))
	{
		throw std::runtime_error("\nError nell'allocazione del SwsContext");
	}


	if (sws_init_context(swsCtx_, NULL, NULL) < 0)
	{
		throw std::runtime_error("\nError nell'inizializzazione del SwsContext");
	}

	swsCtx_ = sws_getCachedContext(swsCtx_,
		// Immagine sorgente
		videoDecoderContext->width,
		videoDecoderContext->height,
		videoDecoderContext->pix_fmt,
		// Immagine destinazione
		videoEncoderContext->width,
		videoEncoderContext->height,
		videoEncoderContext->pix_fmt,
		SWS_BICUBIC, NULL, NULL, NULL);


	int fn = 0;

	std::mutex stop_mutex;

	while (isRun->load()) {
        // av_read_frame legge i pacchetti del formatContext sequenzialmente come una readFile
        if (av_read_frame(inputFormatContext, inPacket) < 0) {
            throw std::runtime_error("Error on reading packet");
        }

        // Poiche' il formatContext e' uno solo dobbiamo distinguere noi da quale
        // stream arriva il paccketto in modo da processarlo correttamente
        if (inPacket->stream_index == stream_index[StreamType::VIDEO]) {
            // Letto il pacchetto(=frame compresso) lo mandiamo al decoder attraverso
            // il codecContext
            if (avcodec_send_packet(videoDecoderContext, inPacket) < 0) {
                throw std::runtime_error("Error on sending packet");
            }
            //std::cout << "In packet size: " << inPacket->size << std::endl;
            int value = avcodec_receive_frame(videoDecoderContext, inFrame);

            if (value == AVERROR(EAGAIN) || value == AVERROR_EOF) {
                throw std::runtime_error("\nOutput not available in this state.  Try to send new input.");
            } else if (value < 0) {
                throw std::runtime_error("\nError during decoding");
            }
            //std::cout << "Input frame Number: " << videoDecoderContext->frame_number << std::endl;
            //std::cout << "In frame linesize: " << inFrame->linesize << std::endl;

            value = sws_scale(swsCtx_, inFrame->data, inFrame->linesize, 0, videoDecoderContext->height, outFrame->data,
                              outFrame->linesize);
            if (value < 0) {
                throw std::runtime_error("\nProblem with sws_scale");
            }


            outPacket->data = nullptr;    // i dati del pacchetto verranno allocati dall'encoder
            outPacket->size = 0;

            outFrame->width = videoEncoderContext->width;
            outFrame->height = videoEncoderContext->height;
            outFrame->format = videoEncoderContext->pix_fmt;


            value = avcodec_send_frame(videoEncoderContext, outFrame);
            if (value < 0) {
                throw std::runtime_error("Error in sending frame");
            }
            value = avcodec_receive_packet(videoEncoderContext, outPacket);
            if (value == AVERROR(EAGAIN)) {
                std::cout << "output is not available right now" << std::endl;
                continue;
            } else if (value < 0 && value != AVERROR_EOF) {
                throw std::runtime_error("\nError during encoding");
            }
            //std::cout << "Out packet size: " << outPacket->size << std::endl;


            if (value >= 0) {

                //std::cout << "\rFrame number " << videoDecoderContext->frame_number << ", successfully encoded" << std::endl;

                //outPacket->stream_index = video_stream_index;
                //outPacket->duration = outStream->time_base.den / outStream->time_base.num / inStream->avg_frame_rate.num * inStream->avg_frame_rate.den;
                //av_packet_rescale_ts(outPacket, in_stream->time_base, out_stream->time_base);
                /*outPacket->dts = av_rescale_q_rnd(outPacket->dts, videoEncoderContext->time_base, outStream->time_base, AV_ROUND_NEAR_INF);
                outPacket->pts = av_rescale_q_rnd(outPacket->pts, videoEncoderContext->time_base, outStream->time_base, AV_ROUND_NEAR_INF);
                outPacket->duration = av_rescale_q(outPacket->duration, videoEncoderContext->time_base, outStream->time_base);*/

                if (outPacket->pts != AV_NOPTS_VALUE)
                    outPacket->pts = av_rescale_q(outPacket->pts, videoEncoderContext->time_base, outStream->time_base);

                if (outPacket->dts != AV_NOPTS_VALUE)
                    outPacket->dts = av_rescale_q(outPacket->dts, videoEncoderContext->time_base, outStream->time_base);


                // Critic section
                std::unique_lock ul(write_mutex);

                if (av_interleaved_write_frame(outputFormatContext, outPacket) < 0) {
                    std::cout << "Error muxing packet" << std::endl;
                    break;
                }

                ul.unlock();

            }

        }
        av_packet_unref(outPacket);


        if (isStopped.load()) {
            avformat_close_input(&this->inputFormatContext);
        }

        std::unique_lock s_ul{stop_mutex};
        s_cv.wait(s_ul, [&isStopped] { return !isStopped.load(); });

    }

	av_free(video_outbuf);
	av_packet_free(&outPacket);
	av_packet_free(&inPacket);
	av_frame_free(&inFrame);

}
