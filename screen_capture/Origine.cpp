/*
 * http://ffmpeg.org/doxygen/trunk/index.html
 *
 * Main components
 *
 * Format (Container) - a wrapper, providing sync, metadata and muxing for the streams.
 * Stream - a continuous stream (audio or video) of data over time.
 * Codec - defines how data are enCOded (from Frame to Packet)
 *        and DECoded (from Packet to Frame).
 * Packet - are the data (kind of slices of the stream data) to be decoded as raw frames.
 * Frame - a decoded raw frame (to be encoded or filtered).
 */

extern "C" 
{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libavdevice/avdevice.h>
}

#include <iostream>
#include <fstream>

#define __STDC_CONSTANT_MACROS

int ciccio(int argc, const char* argv[]) {

	const char *fileName = "C:/Users/chris/Desktop/VID-20161112-WA0038.mp4";
	//const char* fileName = "C:\\Users\\elia_\\OneDrive\\Desktop\\VID-20161112-WA0038.mp4";
	//const char* fileName = "C:\\Users\\elia_\\OneDrive\\Desktop\\small_bunny_1080p_60fps.mp4";
	const char* outputFilename = "C:/Users/chris/Desktop/output.mp4";


	// Allocation of the main format context where the imported media file is stored
	AVFormatContext* inFormatContext = nullptr;
	AVFormatContext* outputFormatContext = nullptr;
	const AVCodec* codecVideoDecoder = nullptr;	
	AVCodecContext* codecContextVideoDecoder = nullptr;
	AVCodecParameters* codecParmetersVideo = nullptr;
	/*
	const AVCodec* audioCodec = nullptr;
	AVCodecParameters* audioCodecPar = nullptr;
	*/

	AVDictionary* options = nullptr;

	int video_stream_index = -1;
	

	// alloc AVFormatContext input
	inFormatContext = avformat_alloc_context();

	// Serve a iscrivere la nostra aplicazione per l'utilizzo di multimedia device come x11grab, dshow, gdigrab
	avdevice_register_all();		//Warning: This function is not thread safe

	// Option on recordig video
	//av_dict_set(&options, "framerate", "30", 0);
	//av_dict_set(&options, "preset", "medium", 0);
	//av_dict_set(&options, "offset_x", "20", 0);
	//av_dict_set(&options, "offset_y", "40", 0);
	//av_dict_set(&options, "video_size", "1920x1080", 0);

	//auto *pAVInputFormat = av_find_input_format("gdigrab");
	auto* pAVInputFormat = av_find_input_format("dshow");

	int value = avformat_open_input(&inFormatContext, "video=screen-capture-recorder", pAVInputFormat, NULL);
	//int value = avformat_open_input(&inFormatContext, "desktop", pAVInputFormat, &options);

	//int value = avformat_open_input(&inFormatContext, fileName, nullptr, nullptr);

	if (value != 0)
	{
		std::cout << "Error in opening input device";
		exit(1);
	}


	std::cout << fileName << std::endl << "Format " << inFormatContext->iformat->long_name << "\nDuration: " << inFormatContext->duration * 0.000001 << " sec" << std::endl;
	std::cout << "Number of streams: "<<inFormatContext->nb_streams<<std::endl;

	for (unsigned int i = 0; i < inFormatContext->nb_streams; i++)
	{
		// Estraiamo i parametri del codec dello stream dati (audio/video)
		
		AVMediaType type = inFormatContext->streams[i]->codecpar->codec_type;
		

		// codec_type definisce il tipo di stream su cui viene applicato il codec
		// codec_id definisce il tipo di codec in cui e'codificato lo stream 
		if (type == AVMEDIA_TYPE_VIDEO)
		{
			if(video_stream_index==-1) video_stream_index = i;
			/* decoder setup */
			// Estraiamo i parametri del codec dello stream dati (audio/video)
			codecParmetersVideo = inFormatContext->streams[i]->codecpar;
			// Tra i parametri trovati usiamo l' id del codec per trovare il decoder che ci serve
			codecVideoDecoder = avcodec_find_decoder(codecParmetersVideo->codec_id);
			std::cout<< "Video Codec: resolution " <<codecParmetersVideo->width<< "x" << codecParmetersVideo->height<<std::endl;
			// Creazione del codec context a partire dal decoder trovato con la find
			codecContextVideoDecoder = avcodec_alloc_context3(codecVideoDecoder);
			avcodec_parameters_to_context(codecContextVideoDecoder, codecParmetersVideo);
			// Inizializziamo il codecContext con il codec aprendolo
			avcodec_open2(codecContextVideoDecoder, codecVideoDecoder, NULL);
		}/*
		else if (type == AVMEDIA_TYPE_AUDIO)
		{
			// Estraiamo i parametri del codec dello stream dati (audio/video)
			audioCodecPar = inFormatContext->streams[i]->codecpar;
			// Tra i parametri trovati usiamo l' id del codec per trovare il decoder che ci serve
			audioCodec = avcodec_find_decoder(videoCodecPar->codec_id);
			std::cout << "Audio Codec: "<< videoCodecPar->channels << " channels, Sample Rate: " << videoCodecPar->sample_rate<< std::endl;
		}*/

	}

	// General info about codecs
		std::cout<<"\tCodec "<<codecVideoDecoder->long_name <<", ID: "<< codecVideoDecoder->id << ", bit_rate: "<<codecParmetersVideo->bit_rate << std::endl;
		if (codecVideoDecoder->id == AV_CODEC_ID_BMP)
			std::cout << "formato corrispondente" << std::endl;
		else
			std::cout << "NON è VERO" << std::endl;
	/* 
	// Codec context audio
	AVCodecContext* audioCodecContext = avcodec_alloc_context3(audioCodec);
	avcodec_parameters_to_context(audioCodecContext,audioCodecPar);
	*/

	/* gestione output */
	//allocazione memoria per l'output file
	if (avformat_alloc_output_context2(&outputFormatContext, NULL, NULL, outputFilename) < 0) {
		std::cout << "Can't open output context" << std::endl;
		std::exit(1);
	}
	
	/* ENCODER SET UP */
	
	unsigned int tag = 0;

	// seleziono l'encoder di tipo mp4
	const AVCodec* codecVideoEncoder = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!codecVideoEncoder)
		std::cout << "codecVideoEncoder null" << std::endl;
	
	// aggiunge uno stream all'interno del format context
	AVStream* outStreamVideo = avformat_new_stream(outputFormatContext, NULL);
	AVCodecParameters* outStreamParam = outStreamVideo->codecpar;

	//avcodec_parameters_copy(outStreamVideo->codecpar, codecParmetersVideo);

	value = av_codec_get_tag2(outputFormatContext->oformat->codec_tag, codecVideoEncoder->id, &tag);
	if (value == 0) {
		std::cout << "Can't find tag for codec" << std::endl;
		std::exit(1);
	}

	// Creazione del codec context
	AVCodecContext* codecContextVideoEncoder = avcodec_alloc_context3(codecVideoEncoder);
	
	/* set property of the video file */
	codecContextVideoEncoder->codec_id = AV_CODEC_ID_H264;// AV_CODEC_ID_MPEG4; // AV_CODEC_ID_H264 // AV_CODEC_ID_MPEG1VIDEO //AV_CODEC_ID_BMP
	codecContextVideoEncoder->codec_type = AVMEDIA_TYPE_VIDEO;
	codecContextVideoEncoder->pix_fmt = AV_PIX_FMT_YUV420P;
	codecContextVideoEncoder->bit_rate = 400000; // 2500000
	codecContextVideoEncoder->width = 1920;
	codecContextVideoEncoder->codec_tag = tag;
	codecContextVideoEncoder->height = 1080;
	codecContextVideoEncoder->gop_size = 12;
	codecContextVideoEncoder->max_b_frames = 2;
	codecContextVideoEncoder->time_base.num = 1;
	codecContextVideoEncoder->time_base.den = 30; // 30fps

	//=====================================

	avcodec_parameters_from_context(outStreamVideo->codecpar, codecContextVideoEncoder);
	
	//=====================================

	if (codecContextVideoEncoder->codec_id == AV_CODEC_ID_H264)
	{
		av_opt_set(codecContextVideoEncoder->priv_data, "preset", "slow", 0);
	}


	//We set the flag AV_CODEC_FLAG_GLOBAL_HEADER which tells the encoder that it can use the global headers
	if (outputFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
		outputFormatContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	
	// Inizializziamo il codecContext con il codec aprendolo
	avcodec_open2(codecContextVideoEncoder, codecVideoEncoder, NULL);
	
	//Creo il file di output
	if (!(outputFormatContext->oformat->flags & AVFMT_NOFILE)) {
		//PB: I/O context
		if (avio_open(&outputFormatContext->pb, outputFilename, AVIO_FLAG_WRITE) < 0) {
			std::cout << "Could not open output file " << outputFilename << std::endl;
			exit(2);
		}
	}
	//outputFormatContext->video_codec = codecVideoEncoder;

	if (avformat_write_header(outputFormatContext, NULL) < 0) {			
		fprintf(stderr, "Error occurred when writing header file\n");
		exit(2);
	}
	
	AVPacket* inPacket = av_packet_alloc();
	AVFrame* inFrame = av_frame_alloc();
	AVPacket* outPacket = av_packet_alloc();

	int fn = 0;
	// av_read_frame legge i pacchetti del formatContext sequenzialmente come una readFile
	while (av_read_frame(inFormatContext, inPacket) >= 0 && fn<100) {
		AVStream* in_stream, * out_stream;
		in_stream = inFormatContext->streams[inPacket->stream_index];
		out_stream = outputFormatContext->streams[inPacket->stream_index];

		// Poiche' il formatContext e' uno solo dobbiamo distinguere noi da quale
		// stream arriva il paccketto in modo da processarlo correttamente
		if (inPacket->stream_index == video_stream_index) {
			// Letto il pacchetto(=frame compresso) lo mandiamo al decoder attraverso
			// il codecContext
			if (avcodec_send_packet(codecContextVideoDecoder, inPacket) != 0) {
				std::cout << "Error on sending packet";
				exit(1);
			}
			//std::cout << "In packet size: " << inPacket->size << std::endl;
			int response = avcodec_receive_frame(codecContextVideoDecoder, inFrame);

			
			std::cout << "In frame Number: " << codecContextVideoDecoder->frame_number << std::endl;
			//std::cout << "In frame linesize: " << inFrame->linesize << std::endl;
			
			response = avcodec_send_frame(codecContextVideoEncoder, inFrame);		//errore qui
			if (response < 0) {
				std::cout << "error in send frame" << std::endl;
				return -1;
			}
			response = avcodec_receive_packet(codecContextVideoEncoder, outPacket);
			if (response == AVERROR(EAGAIN)) {
				std::cout<<"output is not available right now" << std::endl;
				return -1;
			}
			else if (response == AVERROR(EINVAL)) {
				std::cout << "codec not opened, or it is an encoder other errors : legitimate decoding errors " << std::endl;
				return -1;
			}
			else if (response == AVERROR_EOF) {
				std::cout << "the encoder has been fully flushed, and there will be no more output packets " << std::endl;
				return -1;
			}
			//std::cout << "Out packet size: " << outPacket->size << std::endl;

			outPacket->stream_index = video_stream_index;
			outPacket->duration = out_stream->time_base.den / out_stream->time_base.num / in_stream->avg_frame_rate.num * in_stream->avg_frame_rate.den;
			av_packet_rescale_ts(outPacket, in_stream->time_base, out_stream->time_base);

			if (av_interleaved_write_frame(outputFormatContext, outPacket) < 0) {
				std::cout << "Error muxing packet" << std::endl;
				break;
			}
			av_packet_unref(outPacket);			
			av_frame_unref(inFrame);
			av_packet_unref(inPacket);
			fn++;
		}
	}

	av_write_trailer(outputFormatContext);








	// Close format context input
	avformat_close_input(&inFormatContext);
	av_packet_free(&outPacket);
	av_packet_free(&inPacket);
	av_frame_free(&inFrame);
	// Close codec Context
	avcodec_free_context(&codecContextVideoDecoder);

	return 0;
}



//int prepare_video_encoder(StreamingContext* sc, AVCodecContext* decoder_ctx, AVRational input_framerate, StreamingParams sp) {
//	sc->video_avs = avformat_new_stream(sc->avfc, NULL);
//
//	sc->video_avc = avcodec_find_encoder_by_name(sp.video_codec);
//	if (!sc->video_avc) { logging("could not find the proper codec"); return -1; }
//
//	sc->video_avcc = avcodec_alloc_context3(sc->video_avc);
//	if (!sc->video_avcc) { logging("could not allocated memory for codec context"); return -1; }
//
//	av_opt_set(sc->video_avcc->priv_data, "preset", "fast", 0);
//	if (sp.codec_priv_key && sp.codec_priv_value)
//		av_opt_set(sc->video_avcc->priv_data, sp.codec_priv_key, sp.codec_priv_value, 0);
//
//	sc->video_avcc->height = decoder_ctx->height;
//	sc->video_avcc->width = decoder_ctx->width;
//	sc->video_avcc->sample_aspect_ratio = decoder_ctx->sample_aspect_ratio;
//	if (sc->video_avc->pix_fmts)
//		sc->video_avcc->pix_fmt = sc->video_avc->pix_fmts[0];
//	else
//		sc->video_avcc->pix_fmt = decoder_ctx->pix_fmt;
//
//	sc->video_avcc->bit_rate = 2 * 1000 * 1000;
//	sc->video_avcc->rc_buffer_size = 4 * 1000 * 1000;
//	sc->video_avcc->rc_max_rate = 2 * 1000 * 1000;
//	sc->video_avcc->rc_min_rate = 2.5 * 1000 * 1000;
//
//	sc->video_avcc->time_base = av_inv_q(input_framerate);
//	sc->video_avs->time_base = sc->video_avcc->time_base;
//
//	if (avcodec_open2(sc->video_avcc, sc->video_avc, NULL) < 0) { logging("could not open the codec"); return -1; }
//	avcodec_parameters_from_context(sc->video_avs->codecpar, sc->video_avcc);
//	return 0;
//}