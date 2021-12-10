#include "ScreenRecorder.h"

using namespace std;

/* initialize the resources*/
ScreenRecorder::ScreenRecorder()
{

    //av_register_all(); //Funzione di inzizializzazione deprecata. Può essere tranquillamente omessa.
    //avcodec_register_all(); //Funzione di inzizializzazione deprecata. Può essere tranquillamente omessa.
    avdevice_register_all(); //Inizializza libavdevice e registra tutti i dispositivi di input e output.
    cout << "\nAll required functions are registered successfully";
}
//Definiamo il distruttore
/* uninitialize the resources */
ScreenRecorder::~ScreenRecorder()
{

    avformat_close_input(&pAVFormatContext); //Chiude un input AVFormatContext aperto: libera tutto e mette  a NULL il contenuto del parametro ricevuto
    if (!pAVFormatContext) //Verifichiamo che avformat_close_input abbia dato i suoi frutti
    {
        cout << "\nFile closed sucessfully";
    }
    else
    {
        cout << "\nUnable to close the file";
        exit(1);
    }

    avformat_free_context(pAVFormatContext); //Libera pAVFormatContext e tutti i suoi stream.
    if (!pAVFormatContext) //Verifichiamo che avformat_free_context abbia dato i suoi frutti
    {
        cout << "\nAvformat free successfully";
    }
    else
    {
        cout << "\nUnable to free avformat context";
        exit(1);
    }

}

/* establishing the connection between camera or screen through its respective folder */
int ScreenRecorder::openCamera()
{

    value = 0;
    options = NULL;
    pAVFormatContext = NULL;

    //X11 video input device.
    //To enable this input device during configuration you need libxcb installed on your system.
    //It will be automatically detected during configuration.
    //This device allows one to capture a region of an X11 display.
    //refer : https://www.ffmpeg.org/ffmpeg-devices.html#x11grab


    /* current below is for screen recording. To connect with camera use v4l2 as a input parameter for av_find_input_format */
    //pAVInputFormat = av_find_input_format("x11grab");//Errore: x11grab non da problemi su Linux (o, perlomeno, non dovrebbe dare problemi), ma su Windows 11 sì
    pAVInputFormat = av_find_input_format("gdigrab");
    //av_find_input_format trova  un AVInputFormat in base al nome breve del formato di input.
    //cout << "\npAVInputFormat->codec_tag: " << pAVInputFormat->codec_tag;
    //value = avformat_open_input(&pAVFormatContext, ":0.0+10,250", pAVInputFormat, NULL);

    /*
    * Con av_dict_set passo determinati parametri a options che mi servirà, dopo, per settare alcuni parametri di
    * pAVFormatContext con avformat_open_input. av_dict_set ritorna un alore maggiore di zero in caso di successo
    * minore di zero in caso di fallimento.
    */
    value = av_dict_set(&options, "framerate", "30", 0);
    if (value < 0) //Controllo che non ci siano stati errori con av_dict_set
    {
        cout << "\nError in setting dictionary value";
        exit(1);
    }
    value = av_dict_set(&options, "preset", "medium", 0);
    if (value < 0)
    {
        cout << "\nError in setting preset values";
        exit(1);
    }
    value = av_dict_set(&options, "video_size", "1920x1080", 0);
    if (value < 0)
    {
        cout << "\nError in setting preset values";
        exit(1);
    }
    value = av_dict_set(&options, "probesize", "20M", 0);
    if (value < 0)
    {
        cout << "\nError in setting preset values";
        exit(1);
    }

    pAVFormatContext = avformat_alloc_context();//Allocate an AVFormatContext

    value = avformat_open_input(&pAVFormatContext, "desktop", pAVInputFormat, &options);
    if (value != 0) //Controllo che non ci siano stati errori con avformat_open_input
    {
        cout << "\nError in opening input device\n";
        exit(1);
    }

    //cout << "\Framerate: " << pAVFormatContext; 


    // avformat_open_input apre uno stream di input e legge l'header.
    // NB: I codec non vengono aperti. Lo stream, inoltre,  deve essere chiuso con avformat_close_input().
    // Ritorna 0 in caso di successo, un valore <0 in caso di fallimento.

    value = avformat_find_stream_info(pAVFormatContext, &options); //Read packets of a media file to get stream information.
    if (value < 0)
    {
        cout << "\nunable to find the stream information";
        exit(1);
    }

    VideoStreamIndx = -1;

    /* find the first video stream index . Also there is an API available to do the below operations */
    for (int i = 0; i < pAVFormatContext->nb_streams; i++) // find video stream posistion/index
    {
        if (pAVFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            VideoStreamIndx = i;
            pCodecParameters = pAVFormatContext->streams[i]->codecpar;
            //cout << "\nHeight: " << pAVFormatContext->streams[i]->codecpar->height;
            //cout << "\nWidth: " << pAVFormatContext->streams[i]->codecpar->width;
            //cout << "\npAVFormatContext->streams[i]->codecpar->codec_id: " << pAVFormatContext->streams[i]->codecpar->codec_id << "\n";
            pAVCodec = avcodec_find_decoder(pAVFormatContext->streams[i]->codecpar->codec_id);
            //cout << "\npAVCodec->name: " << pAVCodec->name << "\n";

            break;
        }

    }

    if (VideoStreamIndx == -1)
    {
        cout << "\nUnable to find the video stream index. (-1)";
        exit(1);
    }

    if (pAVCodec == NULL)
    {
        cout << "\nUnable to find the decoder";
        exit(1);
    }
    //Link per capire meglio sta parte: https://awesomeopensource.com/project/leandromoreira/ffmpeg-libav-tutorial
    // assign pAVFormatContext to VideoStreamIndx
    //pAVCodecContext = pAVFormatContext->streams[VideoStreamIndx]->codec; //Errore perche in streams[VideoStreamIndx] non c'è nessun codec
    pAVCodecContext = avcodec_alloc_context3(pAVCodec);
    // Alloca un AVCodecContext e imposta i suoi campi sui valori predefiniti.
    // Ritorna un AVCodecContext riempito con valori predefiniti o NULL in caso di errore.
    value = avcodec_parameters_to_context(pAVCodecContext, pCodecParameters);
    //Riempie il CodecContext in base ai valori dei parametri forniti.
    //Ritorna un valore >=0 in caso di successo.

    if (value < 0)
    {
        cout << "\nUnable to set the parameter of the codec";
        exit(1);
    }

    //pAVCodec = avcodec_find_decoder(pAVCodecContext->codec_id);// avcodec_find_decoderha un ritorno di tipo const
    //pAVCodec = const_cast<AVCodec *>(avcodec_find_decoder(pAVCodecContext->codec_id));
    /*if( pAVCodec == NULL )
    {
        cout<<"\nunable to find the decoder";
        exit(1);
    }*/

    value = avcodec_open2(pAVCodecContext, pAVCodec, NULL);
    //Initialize the AVCodecContext to use the given AVCodec.
    // Prima di utilizzare questa funzione, il contesto deve essere allocato con avcodec_alloc_context3().
    if (value < 0)
    {
        cout << "\nUnable to open the av codec";
        exit(1);
    }
    //cout << "\npAVCodecContext->width: "<< pAVCodecContext->width;
    return 0;
}

/* initialize the video output file and its properties  */
int ScreenRecorder::init_outputfile()
{
    outAVFormatContext = NULL;
    value = 0;
    output_file = "C:/Users/chris/Desktop/output.mp4";

    avformat_alloc_output_context2(&outAVFormatContext, NULL, NULL, output_file);
    // Assegna un AVFormatContext per un formato di output.
    // Il primo parametro è settato sul format context creato o su NULL in caso di errore
    // L'ultimo parametro indica il nome del filename da usare per allocare il context
    if (!outAVFormatContext) //Effettuo un check
    {
        cout << "\nError in allocating av format output context";
        exit(1);
    }

    output_format = av_guess_format(NULL, output_file, NULL);
    //cout << "\nav_guess_format: "<< av_guess_format<<"\n";
    /* Ritorna il formato di output nell'elenco dei formati di output registrati
     * che matcha meglio con i parametri forniti. Se non c'è alcun match ritorna NULL
     */
    if (!output_format) //Effettuo un check
    {
        cout << "\nError in guessing the video format. try with correct format";
        exit(1);
    }
    pLocalCodec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
    //Trova un codificatore (encoder) che matcha con l'ID_codec indicato.
    //Ritorna l'encoder in caso di successo, NULL in caso di errore
    if (pLocalCodec == NULL) // Eseguo check
    {
        cout << "\nUnable to find the encoder";
        exit(1);
    }

    outAVCodecContext = avcodec_alloc_context3(pLocalCodec);
    // Alloca un AVCodecContext e imposta i suoi campi sui valori predefiniti.
    // Ritorna un AVCodecContext riempito con valori predefiniti o NULL in caso di errore.
    if (!outAVCodecContext)
    {
        cout << "\nError in allocating the codec contexts";
        exit(1);
    }

    outAVCodecContext->codec_id = AV_CODEC_ID_MPEG4;// AV_CODEC_ID_MPEG4; // AV_CODEC_ID_H264 // AV_CODEC_ID_MPEG1VIDEO
    outAVCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
    outAVCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    outAVCodecContext->width = 1920;
    outAVCodecContext->height = 1080;
    outAVCodecContext->gop_size = 3;
    outAVCodecContext->max_b_frames = 2;
    outAVCodecContext->time_base.num = 1;
    outAVCodecContext->time_base.den = 30; // 15fps


    value = avcodec_open2(outAVCodecContext, pLocalCodec, NULL);
    //Initialize the AVCodecContext to use the given AVCodec.
    // Prima di utilizzare questa funzione, il contesto deve essere allocato con avcodec_alloc_context3().
    if (value < 0) //Effettuo un check
    {
        cout << "\nError in opening the avcodec";
        exit(1);
    }

    video_st = avformat_new_stream(outAVFormatContext, pLocalCodec);
    // Aggiunge un nuovo stream al file media
    // Ritorna lo stream appena creato
    if (!video_st) //Effettuo un check
    {
        cout << "\nError in creating a av format new stream";
        exit(1);
    }

    /* set property of the video file */
    // outAVCodecContext = video_st->codec; //video_st non ha più un campo codec
    // Risolvo il problema con le seguenti righe di codice    


    //video_st->time_base = { 1, 30 };
    //video_st->codecpar->codec_id = AV_CODEC_ID_MPEG4;

    if (codec_id == AV_CODEC_ID_H264)
    {
        av_opt_set(outAVCodecContext->priv_data, "preset", "slow", 0);
        // This function set the field of obj with the given name to value.
    }
    value = avcodec_parameters_from_context(video_st->codecpar, outAVCodecContext);
    if (value < 0) // Eseguo check
    {
        cout << "\nUnable to set the parameter of the codec";
        exit(1);
    }

    video_st->time_base = { 1, 30 };
    /*
    outAVCodec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
    //Trova un codificatore (encoder) che matcha con l'ID_codec indicato.
    //Ritorna l'encoder in caso di successo, NULL in caso di errore
    if( !outAVCodec ) //Effettuo un check
    {
        cout<<"\nError in finding the av codecs. try again with correct codec";
        exit(1);
    }
    */
    /* Some container formats (like MP4) require global headers to be present
       Mark the encoder so that it behaves accordingly. */
    if (outAVFormatContext->oformat->flags & AVFMT_GLOBALHEADER) //Effettuo un check sui flag
    {
        outAVCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    //cout << outAVCodecContext;

    /* create empty video file */
    if (!(outAVFormatContext->flags & AVFMT_NOFILE)) // Mi assicuro che i flag siano settati secondo i valori indicati da AVFMT_NOFILE
    {
        if (avio_open2(&outAVFormatContext->pb, output_file, AVIO_FLAG_WRITE, NULL, NULL) < 0)
            /*Crea e inizializza un AVIOContext (di cui si salva poi il puntatore nel primo parametro) per accedere alle risorse indicate da "output_file".
             * In caso di fallimento la funzione ritorna un valore <0.
             * NB: Quando le risorse indicate da "output_file" sono aperte in read+write, l'AVIOContext può essere usato solo in scrittura.
             */
        {
            cout << "\nError in creating the video file";
            exit(1);
        }
    }

    if (!outAVFormatContext->nb_streams) //Effettuo un check sul numero di stream
    {
        cout << "\nOutput file dose not contain any stream";
        exit(1);
    }

    /* imp: mp4 container or some advanced container file required header information*/
    value = avformat_write_header(outAVFormatContext, &options);
    //Alloca i dati privati dello stream e scrive l'header dello stream in un file multimediale di output.
    if (value < 0) //Controllo che avformat_write_header abbia avuto successo
    {
        cout << "\nError in writing the header context";
        exit(1);
    }
    /*
    // uncomment here to view the complete video file informations
    cout<<"\n\nOutput file information :\n\n";
    av_dump_format(outAVFormatContext , 0 ,output_file ,1);
    */
    return 0;
}

/* funzione per acquisire e memorizzare i dati in frame allocando la memoria richiesta e rilasciando automaticamente la memoria */
int ScreenRecorder::CaptureVideoFrames()
{
    //int flag;
    //int frameFinished;

    /*Quando decodifichi un singolo pacchetto, non hai ancora informazioni sufficienti per avere un frame
     * (a seconda del tipo di codec). Quando decodifichi un GRUPPO di pacchetti che rappresenta un frame,
     * solo allora hai un'immagine! Ecco perché frameFinished ti farà sapere che hai decodificato abbastanza
     * per avere un frame.
     * */

     //int frame_index = 0;
    value = 0;
    pAVPacket = av_packet_alloc();
    //av_packet_alloc alloca un AVPacket e imposta i suoi campi sui valori predefiniti.
    if (!pAVPacket)
        exit(1);
    //cout<<""<<pAVPacket->
    //Le successive 2 righe di codice son deprecate commentate perchè av_init_packet è stato deprecato
    //pAVPacket = (AVPacket *)av_malloc(sizeof(AVPacket));
    /*av_malloc alloca un blocco di dimenzione pari a "sizeof(AVPacket)" Byte e ritorna un puntatore al blocco
     * allocato oppure NULL se il blocco non può essere allocato.
     * NB: pAVPacket è di tipo AVPacket *. Non a caso si esegue un cast a "AVPacket *" del ritorno di av-malloc
     * Di fatto, pAVPacket è un puntatore ad un pacchetto.*/
     //av_init_packet(pAVPacket);
     /*Inizializza i campi facoltativi di un pacchetto con valori predefiniti.
      *NB: questa funzione non tocca i membri "data" e "size", che devono essere inizializzati separatamente.*/

    pAVFrame = av_frame_alloc();
    /*av_frame_alloc() alloca un AVFrame e imposta i suoi campi sui valori predefiniti.
     *La struttura risultante deve essere liberata utilizzando av_frame_free().
     * Ritorna un AVFrame riempito con valori predefiniti o NULL in caso di errore.
     * NB: Alloca solo l'AVFrame e non il buffer di dati. Questo deve essere allocato con altri mezzi,
     * ad es. con av_frame_get_buffer() o manualmente.*/

    if (!pAVFrame) // Verifichiamo che l'operazione svolta da "av_frame_alloc()" abbia avuto successo
    {
        cout << "\nUnable to release the avframe resources";
        exit(1);
    }

    outFrame = av_frame_alloc();
    if (!outFrame)
    {
        cout << "\nUnable to release the avframe resources for outframe";
        exit(1);
    }

    int video_outbuf_size;
    int nbytes = av_image_get_buffer_size(outAVCodecContext->pix_fmt, outAVCodecContext->width, outAVCodecContext->height, 32);
    /*"av_image_get_buffer_size" restituisce il numero di byte necessari per memorizzare un'immagine.
     * NB: Le specifiche dell'immagine sono indicati tra parentesi*/
    uint8_t* video_outbuf = (uint8_t*)av_malloc(nbytes);
    if (video_outbuf == NULL)
    {
        cout << "\nUnable to allocate memory";
        exit(1);
    }

    // Setup the data pointers and linesizes based on the specified image parameters and the provided array.
    value = av_image_fill_arrays(outFrame->data, outFrame->linesize, video_outbuf, AV_PIX_FMT_YUV420P, outAVCodecContext->width, outAVCodecContext->height, 1);
    /* Si effettua un setup dei data pointers e delle linesizes (??) in base alle specifiche dell'immagine e
     * ed all'array fornito.
     * La funzione ritorna la dimensione in byte richiesta per video_outbuf oppure un valore minore di zero in caso di errore.
     * SIGNIFICATO PARAMETRI:
     * "outFrame->data" indica i data pointers da compilare
     * "outFrame->linesize" indica le linesize per l'immagine in outFrame->data da compilare
     * "video_outbuf" indica il buffer che contiene o conterrà i dati dell'immagine effettivi. Può valere NULL
     * "AV_PIX_FMT_YUV420P" indica il formato pixel dell'immagine
     * "outAVCodecContext->width" e "outAVCodecContext->height" indicano rispettivamente la larghezza e l'altezza dell'immagine in pixel
     * L'ultimo parametro indica il valore usato per allineare le linesizes
     */
    if (value < 0) //Verifico che non ci siano errori
    {
        cout << "\nError in filling image array";
        exit(1);
    }

    struct SwsContext* swsCtx_;

    if (!(swsCtx_ = sws_alloc_context()))
    {
        cout << "\nError nell'allocazione del SwsContext";
        exit(1);
    }

    // Allocate and return swsContext.
    // a pointer to an allocated context, or NULL in case of error
    // Deprecated : Use sws_getCachedContext() instead.
    /*swsCtx_ = sws_getContext(pAVCodecContext->width,
        pAVCodecContext->height,
        pAVCodecContext->pix_fmt,
        outAVCodecContext->width,
        outAVCodecContext->height,
        outAVCodecContext->pix_fmt,
        SWS_BICUBIC, NULL, NULL, NULL );*/
        /*La funzione sws_getContext alloca un SwsContext e ne ritorna il puntatore (o NULL in caso di errore).
         *I primi 3 parametri sono riferiti all'immagine sorgente.
         *I secondi 3 parametri sono riferiti all'immagine di destinazione.
         * Il settimo parametri specifica quale algoritmo utilizzare per ri-scalare
         * I restanti parametri sono flag di cui ce ne possiamo sbattere il cazzo
         */

    value = sws_init_context(swsCtx_, NULL, NULL);
    if (value < 0)
    {
        cout << "\nError nell'inizializzazione del SwsContext";
        exit(1);
    }

    swsCtx_ = sws_getCachedContext(swsCtx_,
        pAVCodecContext->width,
        pAVCodecContext->height,
        pAVCodecContext->pix_fmt,
        outAVCodecContext->width,
        outAVCodecContext->height,
        outAVCodecContext->pix_fmt,
        SWS_BICUBIC, NULL, NULL, NULL);

    //cout << "\nswsCtx_: " << swsCtx_ <<"\n";

    int ii = 0;
    int no_frames = 100;
    cout << "\nEnter No. of frames to capture : ";
    cin >> no_frames;

    AVPacket* outPacket;
    int j = 0;

    int got_picture;
    while (av_read_frame(pAVFormatContext, pAVPacket) >= 0)
    {
        //cout << "\npAVPacket->buf: " << pAVPacket->buf;
        /*av_read_frame è una funzione che ad ogni chiamata trasmette un frame preso da uno stream.
        * In caso di successo il paccheto sarà reference-counted (pAVPacket->buf viene settato) e sarà disponibile a tempo indeterminato.
        * Il pacchetto deve essere lberato con av_packet_unref() quando non è più utile.
        * Per il video, il pacchetto contiene esattamente un fotogramma.
        * Per l'audio, invece, dipende:
        * - se ogni frame ha una dimensione nota (ad es. dati PCM o ADPCM), allora il pacchetto coterrà un numero intero di frame;
        * - se ogni frame ha una dimensione variabile (ad es. audio MPEG), allora il pacchetto coterrà un solo frame.
        *  pAVPacket->pts può valere AV_NOPTS_VALUE se il formato video contiene B-frame, quindi è meglio fare affidamento a
        *  pAVPacket->dts (pAVPacket->dts e pAVPacket->pts sono due timestamp).
        *  av_read_frame ritorna 0 se tutto è ok, un valore negativo in caso di errore o un EOF.
        *  In caso di errore, pAVPacket sarà vuoto (come se provenisse da av_packet_alloc()).
        *  NB:pAVPacket verrà inizializzato, quindi potrebbe essere necessario terminarlo anche se non contiene dati.
        */
        if (ii++ == no_frames)
        {
            //value = AVERROR_EOF;
            break;
        }
        if (pAVPacket->stream_index == VideoStreamIndx)
        {
            //char buf[1024];
            //FUNZIONE DEPRECATA
            //value = avcodec_decode_video2( pAVCodecContext , pAVFrame , &frameFinished , pAVPacket );
            value = avcodec_send_packet(pAVCodecContext, pAVPacket);
            /* avcodec_send_packet fornisce dati compressi grezzi in un AVPacket come input al decodificatore.
             * Internamente questa chiamata copierà i campi rilevanti di pAVCodecContext che possono influenzare
             * la decodifica per-packet e li applicherà quando il pacchetto verrà effettivamente decodificato.
             * Ritorna 0 in caso di successo, altrimenti ritorna un valore negativo.
             */
            if (value < 0) //Verifichiamo che non ci siano stati errori
            {
                cout << "\nProblem with avcodec_send_packet";
                //exit(1);
            }
            /*
            cout << "\npAVCodecContext->width: " << pAVCodecContext->width;
            cout << "\npAVCodecContext->height: " << pAVCodecContext->height;
            cout << "\npAVCodecContext->coded_width: " << pAVCodecContext->coded_width;
            cout << "\npAVCodecContext->coded_height: " << pAVCodecContext->coded_height;
            cout << "\npAVCodecContext->pix_fmt: " << pAVCodecContext->pix_fmt;
            cout << "\npAVCodecContext->codec->id: " << pAVCodecContext->codec->id;
            */
            value = avcodec_receive_frame(pAVCodecContext, pAVFrame);
            cout << "\nFrame: " << pAVCodecContext->frame_number << "\n";
            /* avcodec_receive_frame restituisce i dati decodificati da un decodificatore.
            * Ritorna 0 in caso di successo.
            */
            //cout << "\npAVFrame->data[0]: " << pAVFrame->data[0];
            //cout << "\nvalue AVERROR_EOF: " << AVERROR_EOF;
            if (value == AVERROR(EAGAIN) || value == AVERROR_EOF) {
                cout << "\nOutput not available in this state.  Try to send new input. ";
                //break;
                //exit(1);
            }
            else if (value < 0)
            {
                cout << "\nError during decoding";
                exit(1);
            }
            //snprintf(buf, sizeof(buf), "%s-%d", filename, dec_ctx->frame_number);
            //pgm_save(frame->data[0], frame->linesize[0], frame->width, frame->height, buf);    

            value = sws_scale(swsCtx_, pAVFrame->data, pAVFrame->linesize, 0, pAVCodecContext->height, outFrame->data, outFrame->linesize);
            /* Funzione utile per riscalare lo slice dell'immagine (presa da pAVFrame->data) e salvare il risultato in outFrame->data.
            * Ritorna l'altezza dell'immagine di output
            */
            if (value < 0) {
                cout << "\nProblem with sws_scale ";
                //break;
                exit(1);
            }
            //av_init_packet(&outPacket); funzione deprecata
            outPacket = av_packet_alloc();
            outPacket->data = nullptr;    // i dati del pacchetto verranno allocati dall'encoder
            outPacket->size = 0;
            //FUNZIONE DEPRECATA
            //avcodec_encode_video2(outAVCodecContext , &outPacket ,outFrame , &got_picture);
            outFrame->width = outAVCodecContext->width;
            outFrame->height = outAVCodecContext->height;
            outFrame->format = outAVCodecContext->pix_fmt;
            //outAVCodecContext->pix_fmt;          

            value = avcodec_send_frame(outAVCodecContext, outFrame);
            if (value < 0)
            {
                cout << "\nError sending a frame for encoding. ERROR CODE: " << value;
                continue;
                //exit(1);
            }

            value = avcodec_receive_packet(outAVCodecContext, outPacket); //Legge i dati codificati dall'encoder.
            if (value == AVERROR(EAGAIN))
            {
                cout << "\nOutput not available in this state.  Try to send new input";
                continue;
                //exit(1);
            }
            else if (value < 0 && value != AVERROR_EOF)
            {
                //cout << "\nAVERROR_EOF: " << AVERROR_EOF;
                //cout << "\nAVERROR(EAGAIN): " << AVERROR(EAGAIN);
                cout << "\nError during encoding";
                //continue;
                exit(1);
            }

            if (value >= 0) // Frame successfully encoded :)
            {
                if (outPacket->pts != AV_NOPTS_VALUE)
                    outPacket->pts = av_rescale_q(outPacket->pts, outAVCodecContext->time_base, video_st->time_base);
                //Rescales a 64-bit integer by 2 rational numbers.
                //Nel codice di Abdullah veniva usata la stessa funzione ma con un parametro diverso (che dava errore):
                // outPacket.pts = av_rescale_q(outPacket.pts, video_st->codec->time_base, video_st->time_base);
                //video_st->codec->time_base è stato deprecato.
                if (outPacket->dts != AV_NOPTS_VALUE)
                    outPacket->dts = av_rescale_q(outPacket->dts, outAVCodecContext->time_base, video_st->time_base);
                //Rescales a 64-bit integer by 2 rational numbers.
                //Nel codice di Abdullah veniva usata la stessa funzione ma con un parametro diverso (che dava errore):
                //outPacket.dts = av_rescale_q(outPacket.dts, video_st->codec->time_base, video_st->time_base);
                //video_st->codec->time_base è stato deprecato.

                printf("Write frame %3d (size= %2d)\n", j++, outPacket->size / 1000);
                if (av_write_frame(outAVFormatContext, outPacket) != 0)
                {
                    cout << "\nError in writing video frame";
                    /*
                    * "av_write_frame" serve per scrivere un pacchetto (outpacket) in un file multimediale di output.
                    * Ritorna 0 se tutto è ok, un valore <0 se ci sono errori, 1 se è stato flushato
                    */
                }
                av_packet_free(&outPacket);
                //av_packet_unref(&outPacket);
                /*
                * Pulisce il pacchetto.
                * Elimina il riferimento al buffer a cui fa riferimento il pacchetto e resetta
                * i rimanenti campi del pacchetto ai loro valori predefiniti.
                */
            } // got_picture
            av_packet_free(&outPacket);
            //av_packet_unref(&outPacket);


        }
    }// End of while-loop

    value = av_write_trailer(outAVFormatContext);
    /*
     * Scrive il trailer dello stream in un file multimediale di output e
     * libera i dati privati ​​del file. Se non ci sono stati errori, ritorna 0.
     */
    if (value < 0)
    {
        cout << "\nError in writing av trailer";
        exit(1);
    }


    //THIS WAS ADDED LATER
    av_packet_free(&pAVPacket);
    av_free(video_outbuf);
    /*
     * Libera un blocco di memoria che è stato allocato con av_malloc (z) () o av_realloc ().
     * Riceve come parametro il puntatore al blocco di memoria che deve essere liberato.
     */
    return 0;
}