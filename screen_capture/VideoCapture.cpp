#include "VideoCapture.h"


// CONSTRUCTOR AND DISTRUCTOR ==================================================================================
VideoCapture::VideoCapture(std::string outputFileName, Resolution res, std::string offset_x, std::string offset_y):
	outputFileName(outputFileName),
	inputFileName("gdigrab"),
	audioOn(true),
	framerate("30"),
	res(res),
	offset_x{ offset_x },
	offset_y{ offset_y },
	stream_index{ 0,0 }
{
	avdevice_register_all(); // It's not thread safe
}


VideoCapture::~VideoCapture() {

	// Close format context input
	if (inputFormatContext)
		avformat_close_input(&inputFormatContext);

	if (outputFormatContext && !(outputFormatContext->oformat->flags & AVFMT_NOFILE))
	{
		avio_close(outputFormatContext->pb);
	}

	if (outputFormatContext)
		avformat_free_context(outputFormatContext);


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
int VideoCapture::intilizeDecoder() {

	const AVInputFormat* pAVInputFormat = av_find_input_format("gdigrab");
	if (!pAVInputFormat) {
		std::cout << "Error in opening input device";
		return(-1);
	}

	// Option dictionary
	av_dict_set(&options, "framerate", this->framerate.c_str(), 0);
	av_dict_set(&options, "preset", "medium", 0);
	av_dict_set(&options, "offset_x", this->offset_x.c_str(), 0);
	av_dict_set(&options, "offset_y", this->offset_y.c_str(), 0);
	av_dict_set(&options, "video_size", (this->res.width + "x" + this->res.height).c_str(), 0);
	av_dict_set(&options, "probesize", "20M", 0);


	// alloc AVFormatContext input
	inputFormatContext = avformat_alloc_context();
	if (!avformat_alloc_context()) {
		std::cout << "Can't allocate input context" << std::endl;
		return(-1);
	}
	if (avformat_open_input(&inputFormatContext, "desktop", pAVInputFormat, &options) < 0) {
		std::cout << "Can't open input context" << std::endl;
		return(-1);
	}

	//Read packets of a media file to get stream information.
	if (avformat_find_stream_info(inputFormatContext, &options) < 0)
	{
		std::cout << "\nunable to find the stream information";
		return(-1);
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
				std::cout << "\nUnable to find the Decoder";
				return(-1);
			}

			break;

		}
	}

	// Creazione del codec context a partire dal decoder e riempito con i parametri del codec
	videoDecoderContext = avcodec_alloc_context3(videoDecoder);
	if (!videoDecoderContext)
	{
		std::cout << "\nUnable to allocate the decoder context";
		return(-1);
	}

	if (avcodec_parameters_to_context(videoDecoderContext, inputFormatContext->streams[stream_index[0]]->codecpar) < 0)
	{
		std::cout << "\nUnable to set the parameter of the codec";
		return(-1);
	}

	// Inizializziamo il Decoder Context con il codec aprendolo
	if (avcodec_open2(videoDecoderContext, videoDecoder, NULL) < 0)
	{
		std::cout << "\nUnable to open the av codec";
		return(-1);
	}

	//Stampo le informazioni dell' input format context
	av_dump_format(inputFormatContext, 0, this->inputFileName.c_str(), 0);

	return 0;
}

int VideoCapture::initializeEncoder()
{
	// Alloco il format context di output
	if (avformat_alloc_output_context2(&outputFormatContext, NULL, NULL, outputFileName.c_str()) < 0) {
		std::cout << "Can't open output context" << std::endl;
		std::exit(1);
	}

	if (!outputFormatContext) //Effettuo un check
	{
		std::cout << "\nError in allocating av format output context";
		exit(1);
	}

	// Cerca un formato che mecci il formato di output. Ritorna NULL se non ne trova uno
	if (!av_guess_format(NULL, this->outputFileName.c_str(), NULL)) //Effettuo un check
	{
		std::cout << "\nError in guessing the video format. try with correct format";
		exit(1);
	}


	videoEncoder = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
	if (!videoEncoder) // Eseguo check
	{
		std::cout << "\nUnable to find the encoder";
		exit(1);
	}

	videoEncoderContext = avcodec_alloc_context3(videoEncoder);
	if (!videoEncoderContext) // Eseguo check
	{
		std::cout << "\nUnable to allocate the encoder context";
		exit(1);
	}

	// Encoder Settings
	//videoEncoderContext->codec_id = AV_CODEC_ID_MPEG4;
	videoEncoderContext->pix_fmt = AV_PIX_FMT_YUV420P;
	//videoEncoderContext->sample_aspect_ratio = videoDecoderContext->sample_aspect_ratio;
	videoEncoderContext->height = 1080;// videoDecoderContext->height;
	videoEncoderContext->width = 1920;// videoDecoderContext->width;
	videoEncoderContext->gop_size = 3;
	videoEncoderContext->max_b_frames = 2;
	videoEncoderContext->time_base.num = 1;
	videoEncoderContext->time_base.den = 30; // 15 fps
	//videoEncoderContext->bit_rate = 400000;




	if (avcodec_open2(videoEncoderContext, videoEncoder, nullptr) < 0) //Effettuo un check
	{
		std::cout << "\nError in opening the Encoder";
		exit(1);
	}

	outStream = avformat_new_stream(outputFormatContext, videoEncoder);
	if (!outStream) {
		av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
		return AVERROR_UNKNOWN;
	}

	if (videoEncoderContext->codec_id == AV_CODEC_ID_H264)
	{
		av_opt_set(videoEncoderContext->priv_data, "preset", "slow", 0);
	}

	if (avcodec_parameters_from_context(outStream->codecpar, videoEncoderContext) < 0) // Eseguo check
	{
		std::cout << "\nUnable to set the parameter of the codec";
		exit(1);
	}

	outStream->time_base = { 1, 30 };

	//We set the flag AV_CODEC_FLAG_GLOBAL_HEADER which tells the encoder that it can use the global headers
	if (outputFormatContext->oformat->flags & AVFMT_GLOBALHEADER) {
		videoEncoderContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}



	//Creo il file di output
	if (!(outputFormatContext->flags & AVFMT_NOFILE)) {
		//PB: I/O context
		if (avio_open2(&outputFormatContext->pb, outputFileName.c_str(), AVIO_FLAG_WRITE, NULL, NULL) < 0) {
			std::cout << "Could not open output file " << outputFileName << std::endl;
			exit(2);
		}
	}

	//Effettuo un check sul numero di stream
	if (!outputFormatContext->nb_streams)
	{ 
		std::cout << "\nOutput file dose not contain any stream";
		exit(1);
	}

	// Scrivo l'header sul formatContext di output
	if (avformat_write_header(outputFormatContext, &options) < 0) {
		fprintf(stderr, "Error occurred when writing header file\n");
		exit(2);
	}

	// Stampo le informazioni del output format context
	av_dump_format(outputFormatContext, 0, this->outputFileName.c_str(), 1);
	return 0;
}


int VideoCapture::startCapturing(int n_frame) {

	AVPacket* inPacket = av_packet_alloc();
	if (!inPacket) { std::cout << "Error on allocating input Packet"; exit(1); }
	AVPacket* outPacket = av_packet_alloc();
	if (!outPacket) { std::cout << "Error on allocating output Packet"; exit(1); }
	AVFrame* inFrame = av_frame_alloc();
	if (!inFrame) { std::cout << "Error on allocating input Frame"; exit(1); }
	AVFrame* outFrame = av_frame_alloc();
	if (!outFrame) { std::cout << "Error on allocating output Frame"; exit(1); }

	int video_outbuf_size;
	int nbytes = av_image_get_buffer_size(videoEncoderContext->pix_fmt, videoEncoderContext->width, videoEncoderContext->height, 32);

	uint8_t* video_outbuf = (uint8_t*)av_malloc(nbytes);
	if (!video_outbuf)
	{
		std::cout << "\nUnable to allocate memory";
		exit(1);
	}

	if (av_image_fill_arrays(outFrame->data, outFrame->linesize, video_outbuf, AV_PIX_FMT_YUV420P, videoEncoderContext->width, videoEncoderContext->height, 1) < 0) //Verifico che non ci siano errori
	{
		std::cout << "\nError in filling image array";
		exit(1);
	}


	struct SwsContext* swsCtx_;

	if (!(swsCtx_ = sws_alloc_context()))
	{
		std::cout << "\nError nell'allocazione del SwsContext";
		exit(1);
	}


	if (sws_init_context(swsCtx_, NULL, NULL) < 0)
	{
		std::cout << "\nError nell'inizializzazione del SwsContext";
		exit(1);
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

	// av_read_frame legge i pacchetti del formatContext sequenzialmente come una readFile
	while (av_read_frame(inputFormatContext, inPacket) >= 0 && fn++ < n_frame) {

		/*if (fn == 50) {
			std::chrono::milliseconds timespan(5000);
			std::this_thread::sleep_for(timespan);
		}*/
		/*AVStream* in_stream, * out_stream;
		in_stream = inputFormatContext->streams[inPacket->stream_index];
		out_stream = outputFormatContext->streams[inPacket->stream_index];*/

		// Poiche' il formatContext e' uno solo dobbiamo distinguere noi da quale
		// stream arriva il paccketto in modo da processarlo correttamente
		if (inPacket->stream_index == stream_index[StreamType::VIDEO]) {
			// Letto il pacchetto(=frame compresso) lo mandiamo al decoder attraverso
			// il codecContext
			if (avcodec_send_packet(videoDecoderContext, inPacket) < 0) {
				std::cout << "Error on sending packet";
				exit(1);
			}
			//std::cout << "In packet size: " << inPacket->size << std::endl;
			int value = avcodec_receive_frame(videoDecoderContext, inFrame);

			if (value == AVERROR(EAGAIN) || value == AVERROR_EOF) {
				std::cout << "\nOutput not available in this state.  Try to send new input. ";
				exit(1);
			}
			else if (value < 0)
			{
				std::cout << "\nError during decoding";
				exit(1);
			}
			std::cout << "Input frame Number: " << videoDecoderContext->frame_number << std::endl;
			//std::cout << "In frame linesize: " << inFrame->linesize << std::endl;

			value = sws_scale(swsCtx_, inFrame->data, inFrame->linesize, 0, videoDecoderContext->height, outFrame->data, outFrame->linesize);
			if (value < 0) {
				std::cout << "\nProblem with sws_scale ";
				//break;
				exit(1);
			}

	
			outPacket->data = nullptr;    // i dati del pacchetto verranno allocati dall'encoder
			outPacket->size = 0;

			outFrame->width = videoEncoderContext->width;
			outFrame->height = videoEncoderContext->height;
			outFrame->format = videoEncoderContext->pix_fmt;


			value = avcodec_send_frame(videoEncoderContext, outFrame);
			if (value < 0) {
				std::cout << "Error in sending frame" << std::endl;
				return -1;
			}
			value = avcodec_receive_packet(videoEncoderContext, outPacket);
			if (value == AVERROR(EAGAIN)) {
				std::cout << "output is not available right now" << std::endl;
				continue;
			}
			else if (value < 0 && value != AVERROR_EOF)
			{
				std::cout << "\nError during encoding";
				exit(1);
			}
			//std::cout << "Out packet size: " << outPacket->size << std::endl;


			if (value >= 0) {

				std::cout << "Frame number " << videoDecoderContext->frame_number << ", successfully encoded" << std::endl;

				//outPacket->stream_index = video_stream_index;
				//outPacket->duration = out_stream->time_base.den / out_stream->time_base.num / in_stream->avg_frame_rate.num * in_stream->avg_frame_rate.den;
				//av_packet_rescale_ts(outPacket, in_stream->time_base, out_stream->time_base);
				/*outPacket->dts = av_rescale_q_rnd(outPacket->dts, videoEncoderContext->time_base, outStream->time_base, AV_ROUND_NEAR_INF);
				outPacket->pts = av_rescale_q_rnd(outPacket->pts, videoEncoderContext->time_base, outStream->time_base, AV_ROUND_NEAR_INF);
				outPacket->duration = av_rescale_q(outPacket->duration, videoEncoderContext->time_base, outStream->time_base);*/

				if (outPacket->pts != AV_NOPTS_VALUE)
					outPacket->pts = av_rescale_q(outPacket->pts, videoEncoderContext->time_base, outStream->time_base);

				if (outPacket->dts != AV_NOPTS_VALUE)
					outPacket->dts = av_rescale_q(outPacket->dts, videoEncoderContext->time_base, outStream->time_base);

				if (av_interleaved_write_frame(outputFormatContext, outPacket) < 0) {
					std::cout << "Error muxing packet" << std::endl;
					break;
				}
			}
		}
		av_packet_unref(outPacket);
	}


	if (av_write_trailer(outputFormatContext) < 0)
	{
		std::cout << "\nError in writing av trailer";
		exit(1);
	}


	av_free(video_outbuf);
	av_packet_free(&outPacket);
	av_packet_free(&inPacket);
	av_frame_free(&inFrame);

	return 0;

}

