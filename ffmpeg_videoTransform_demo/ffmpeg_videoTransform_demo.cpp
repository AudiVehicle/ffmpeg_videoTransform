//////////////////////////////////////////////////////////////////////////
// ffmpeg_videoTransform_demo.cpp : �������̨Ӧ�ó������ڵ㡣
//
//����ʵ����Ƶ���ݵĲɼ�������h.264����ѹ������ͨ��RTMPЭ����д��䲢��ʾ
//by����ȫ��
//audi.car@qq.com
//2015��2��7��16:27:18
//
//SDLˢ���߳��ڲ��ٽ���һ������ѹ�������߳�
//////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "init.h"
#include <ctime>
#include <Windows.h>
#include <fstream>


//Output YUV420P 
#define OUTPUT_YUV420P 1
//'1' Use Dshow 
//'0' Use VFW
#define USE_DSHOW 1		//ʹ��direct show��Ϊ��Ƶ�豸����

#define YUV420P_H264 1	//ѡ���Ƿ���Ҫ����ת�����

//Refresh Event

SYSTEMTIME sys_time;

int main(int argc, char* argv[])
{
	AVFormatContext	*pFormatCtx;
	AVFormatContext	*pFormatCtx_o;		//_o��ʾ������й���
	AVFormatContext *pFormatCtx_test;	//_test��ʾ��ȡ�����ļ�������ʱ��Ҫ��ȡ��ͷ��Ϣ
	unsigned int				videoindex_test;
	unsigned int				i, videoindex;
	unsigned int				videoindex_o;
	AVCodecContext	*pCodecCtx;
	AVCodecContext	*pCodecCtx_o;
	AVCodec			*pCodec;
	AVCodec			*pCodec_o;
	AVPacket		pkt_o;
	AVStream		*video_st_o;
	unsigned int				frame_cnt;
	AVOutputFormat	*fmt_o;
	AVFrame			*pFrame_o;
	unsigned int				picture_size;
	uint8_t			*picture_buf;

	ofstream error_log("error_log.log");

	unsigned int				start=0;
	unsigned int				start1=0;
	unsigned int			j=1;	//j��¼����pkt��pts��Ϣ

	HANDLE Handle_encode=NULL;		//����h.264�߳�
	HANDLE Handle_flushing=NULL;		//����ڴ���Ƶ���������߳�
	HANDLE Handle_dis=NULL;			//ˢ����ʾ�߳�


	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	//Open File
	//char filepath[]="src01_480x272_22.h265";
	//avformat_open_input(&pFormatCtx,filepath,NULL,NULL)

	//Register Device
	avdevice_register_all();	//ע��libavdevice

	//Windows
#ifdef _WIN32

	//Show Dshow Device
	show_dshow_device();
	//Show Device Options
	show_dshow_device_option();
	//Show VFW Options
	show_vfw_device();

#if USE_DSHOW
	AVInputFormat *ifmt=av_find_input_format("dshow");
	AVDictionary* options = NULL;
	av_dict_set(&options,"video_size","320x240",0);		//���ø��ķֱ���
	//av_set_options_string();
	//Set own video device's name
	if(avformat_open_input(&pFormatCtx,"video=QuickCam Orbit/Sphere AF",ifmt,&options)!=0){
		printf("Couldn't open input stream.���޷�����������\n");
		return -1;
	}
#else
	AVInputFormat *ifmt=av_find_input_format("vfwcap");
	if(avformat_open_input(&pFormatCtx,"0",ifmt,NULL)!=0){
		printf("Couldn't open input stream.���޷�����������\n");
		return -1;
	}
#endif
#endif


	//Linux
#ifdef linux
	AVInputFormat *ifmt=av_find_input_format("video4linux2");
	if(avformat_open_input(&pFormatCtx,"/dev/video0",ifmt,NULL)!=0){
		printf("Couldn't open input stream.���޷�����������\n");
		return -1;
	}
#endif


	if(avformat_find_stream_info(pFormatCtx,NULL)<0)
	{
		printf("Couldn't find stream information.���޷���ȡ����Ϣ��\n");
		return -1;
	}
	videoindex=-1;
	for(i=0; i<pFormatCtx->nb_streams; i++) 
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
		{
			videoindex=i;
			break;
		}
		if(videoindex==-1)
		{
			printf("Couldn't find a video stream.��û���ҵ���Ƶ����\n");
			return -1;
		}
		pCodecCtx=pFormatCtx->streams[videoindex]->codec;
		pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
		if(pCodec==NULL)
		{
			printf("Codec not found.��û���ҵ���������\n");
			return -1;
		}
		if(avcodec_open2(pCodecCtx, pCodec,NULL)<0)
		{
			printf("Could not open codec.���޷��򿪽�������\n");
			return -1;
		}
		AVFrame	*pFrame,*pFrameYUV;
		pFrame=avcodec_alloc_frame();
		pFrameYUV=avcodec_alloc_frame();
		uint8_t *out_buffer=(uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
		avpicture_fill((AVPicture *)pFrameYUV, out_buffer, PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);


		//SDL----------------------------
		if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {  
			printf( "Could not initialize SDL - %s\n", SDL_GetError()); 
			return -1;
		} 
		int screen_w=0;
		int	screen_h=0;
		SDL_Surface *screen; 
		

		//////////////////////////////////////////////////////////////////////////
		//��ȫ��  2015��5��29��19:46:56  ������YUV����ʾʱ��Ĺ���
		SDL_Surface *pText;
		TTF_Font	*font;
		char szTime[32]={"hello world"};
		//��ʼ�������
		if (TTF_Init()==-1)
		{
			cout<<"��ʼ���ֿ�ʧ�ܣ�"<<endl;
			return 0;
		}
		  /* ��simfang.ttf �ֿ⣬������Ϊ20�� */  
		font=TTF_OpenFont("C:\\Windows\\Fonts\\Arial.ttf",20);
		if (font==NULL)
		{
			cout<<"error load C:\\Windows\Fonts\\Arial.ttf,please make sure it is existed"<<endl;
			return 0;
		}
		SDL_Color color_red;
		color_red.r=255;
		color_red.g=0;
		color_red.b=0;
		pText=TTF_RenderText_Solid(font,szTime,color_red);
		//////////////////////////////////////////////////////////////////////////

		cout<<"��ȣ�"<<pCodecCtx->width<<endl;
		screen_w = pCodecCtx->width;
		cout<<"�߶ȣ�"<<pCodecCtx->height<<endl;
		screen_h = pCodecCtx->height;
		screen = SDL_SetVideoMode(screen_w, screen_h, 0,0);

		if(!screen) {  
			printf("SDL: could not set video mode - exiting:%s\n",SDL_GetError());  
			return -1;
		}
		SDL_Overlay *bmp; 
		bmp = SDL_CreateYUVOverlay(pCodecCtx->width, pCodecCtx->height,SDL_YV12_OVERLAY, screen); 
		SDL_Rect rect;
		rect.x = 0;    
		rect.y = 0;    
		rect.w = screen_w;    
		rect.h = screen_h; 

		//����rect��������ʾ��rect1��
		SDL_Rect rect1;
		rect1.x=360;
		rect1.y=0;
		rect1.w=screen_w;
		rect1.h=screen_h;

		SDL_Rect rect2;
		rect2.x=0;
		rect2.y=300;
		rect2.w=screen_w;
		rect2.h=screen_h;

		SDL_Rect rect3;
		rect3.x=360;
		rect3.y=300;
		rect3.w=screen_w;
		rect3.h=screen_h;
		//SDL End------------------------
		int ret, got_picture;

		AVPacket *packet=(AVPacket *)av_malloc(sizeof(AVPacket));
		//Output Information-----------------------------
		printf("File Information���ļ���Ϣ��---------------------\n");
		av_dump_format(pFormatCtx,0,NULL,0);
		printf("-------------------------------------------------\n");

#if YUV420P_H264								//�Ƿ���Ƶ�ļ�����Ϊ����output.h264�ļ�
		const char *out_file="output.h264";
		//FILE *fp_yuv=fopen("output.h264","wb+");  
		pFormatCtx_o=avformat_alloc_context();
		fmt_o=av_guess_format(NULL,out_file,NULL);
		pFormatCtx_o->oformat=fmt_o;

		//������ļ�URL
		if (avio_open(&pFormatCtx_o->pb,out_file,AVIO_FLAG_READ_WRITE)<0)
		{
			cout<<"Failed to open output file! (����ļ���ʧ�ܣ���"<<endl;
			return -1;
		}
		video_st_o=avformat_new_stream(pFormatCtx_o,0);
		video_st_o->time_base.num=1;
		video_st_o->time_base.den=8;

		if (video_st_o==NULL)
		{
			return -1;
		}

		//Param that must set
		pCodecCtx_o				=video_st_o->codec;
		pCodecCtx_o->codec_id	=fmt_o->video_codec;
		pCodecCtx_o->codec_type	=AVMEDIA_TYPE_VIDEO;
		pCodecCtx_o->pix_fmt	=PIX_FMT_YUV420P;
		pCodecCtx_o->width		=pCodecCtx->width;
		pCodecCtx_o->height		=pCodecCtx->height;
		pCodecCtx_o->time_base.num=1;
		pCodecCtx_o->time_base.den=8;
		pCodecCtx_o->bit_rate	=400000;
		pCodecCtx_o->gop_size	=15;

		pCodecCtx_o->qmin		=10;
		pCodecCtx_o->qmax		=51;

		//Optional Param
		pCodecCtx_o->max_b_frames=3;

		//set Option
		AVDictionary *param=0;
		//H.264
		if (pCodecCtx_o->codec_type==AV_CODEC_ID_H264)
		{
			av_dict_set(&param,"preset","superfast",0);
			av_dict_set(&param,"tune","zerolatency",0);
			av_dict_set(&param,"threads","4",0);
		}
		//H.265
		if (pCodecCtx_o->codec_type==AV_CODEC_ID_H265)
		{
			av_dict_set(&param,"preset","ultrafast",0);
			av_dict_set(&param,"tune","zero-latency",0);
		}

		//show some Information
		av_dump_format(pFormatCtx_o,0,out_file,1);
		pCodec_o=avcodec_find_encoder(pCodecCtx_o->codec_id);
		if (!pCodec_o)
		{
			cout<<"Can not find encoder! (û���ҵ����ʵı�������)"<<endl;
			return -1;
		}
		if (avcodec_open2(pCodecCtx_o,pCodec_o,&param)<0)
		{
			cout<<"Failed to open encoder! (��������ʧ�ܣ�)"<<endl;
			return -1;
		}

		pFrame_o=avcodec_alloc_frame();
		picture_size=avpicture_get_size(pCodecCtx_o->pix_fmt,pCodecCtx_o->width,pCodecCtx_o->height);
		picture_buf=(uint8_t *)av_malloc(picture_size);
		avpicture_fill((AVPicture *)pFrame_o,picture_buf,pCodecCtx_o->pix_fmt,pCodecCtx_o->width,pCodecCtx_o->height);

		//Write File Header
		avformat_write_header(pFormatCtx_o,NULL);
		av_new_packet(&pkt_o,picture_size);
#endif  
#if OUTPUT_YUV420P 
		FILE *fp_yuv=fopen("output.yuv","wb+");  //�Ƿ���Ƶ�ļ�����Ϊ����output.yuv�ļ�
#endif 

		struct SwsContext *img_convert_ctx;
		img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, (pCodecCtx->width), (pCodecCtx->height), PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL); 
		//------------------------------
		SDL_Thread *video_tid = SDL_CreateThread(sfp_refresh_thread,NULL);	//�����߳�
		//
		SDL_WM_SetCaption("Simple FFmpeg Read Camera",NULL);
		//Event Loop
		SDL_Event event;

		i=0;
		int y_size=pCodecCtx->width*pCodecCtx->height; 

		//////////////////////////////////////////////////////////////////////////
		//�����������
		//��ȫ��  2015��3��25��16:02:09
		//////////////////////////////////////////////////////////////////////////
		AVOutputFormat *ofmt=NULL;
		//�����Ӧһ��AVFormatContext�������Ӧһ��AVFormatContext
		AVFormatContext *ofmt_ctx=NULL;
		const char *out_filename;
		out_filename="rtmp://ble.ossrs.net/test";	//RTMP��ַ
		//����������
		videoindex_test=-1;
		for (i=0;i<pFormatCtx_o->nb_streams;i++)
		{
			if (pFormatCtx_o->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO);
			{
				videoindex_test=i;
				break;
			}
		}

		//���������
		avformat_alloc_output_context2(&ofmt_ctx,NULL,"flv",out_filename);
		if (!ofmt_ctx)
		{
			cout<<"Could not create RTMP output context"<<endl;
			ret=AVERROR_UNKNOWN;
			goto end;
		}
		ofmt=ofmt_ctx->oformat;
		const char *test_file="output_test.h264";
		pFormatCtx_test=avformat_alloc_context();
		avformat_open_input(&pFormatCtx_test,test_file,0,0);
		avformat_find_stream_info(pFormatCtx_test,0);
		av_dump_format(pFormatCtx_test,0,test_file,0);
		for (i=0;i<pFormatCtx_o->nb_streams;i++)
		{
			//���������������������Create output AVStream according to input AVStream
			AVStream *in_stream=pFormatCtx_o->streams[i];
			AVStream *out_stream=avformat_new_stream(ofmt_ctx,in_stream->codec->codec);
			out_stream->time_base.num=1;
			out_stream->time_base.den=9000;		//д�ļ�ͷʱ�ᱻǿ���޸ĳ�1000
			if (!out_stream)
			{
				cout<<"Failed allocating output stream"<<endl;
				ret=AVERROR_UNKNOWN;
				goto end;
			}
			//����AVCodecContext�����ã�Copy the settings of AVCodecContext��
			ret=avcodec_copy_context(out_stream->codec,pFormatCtx_test->streams[0]->codec);
			//ret=avcodec_copy_context(out_stream->codec,in_stream->codec);
			if (ret<0)
			{
				cout<<"Failed to copy context from input to output stream codec context"<<endl;
				goto end;
			}
			out_stream->codec->codec_tag=0;
			if (ofmt_ctx->oformat->flags&AVFMT_GLOBALHEADER)  
			{
				out_stream->codec->flags|=CODEC_FLAG_GLOBAL_HEADER;
			}
		}

		av_dump_format(ofmt_ctx,0,out_filename,1);
		//�����URL��Open output URL��
		if (!(ofmt->flags&AVFMT_NOFILE))
		{
			ret=avio_open(&ofmt_ctx->pb,out_filename,AVIO_FLAG_WRITE);
			if (ret<0)
			{
				cout<<"Could not open output URL "<<out_filename<<endl;
				goto end;
			}
		}
		//д�ļ�ͷ��Write file header��
		ret=avformat_write_header(ofmt_ctx,NULL);
		if (ret<0)
		{
			cout<<"Error occurred when opening output URL"<<endl;
			goto end;
		}

		unsigned int frame_index=0;
		
		

		for (;;)	//ѭ������ˢ��
		{
			//Wait
			SDL_WaitEvent(&event);
			
			if(event.type==SFM_REFRESH_EVENT)
			{
				int64_t start_time=av_gettime(); 
				start1=clock();
				//------------------------------
				if(av_read_frame(pFormatCtx, packet)>=0)
				{
					if(packet->stream_index==videoindex)
					{
						int decode_time=clock();
						ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
						if(ret < 0)
						{
							printf("Decode Error.���������\n");
							return -1;
						}
						cout<<"��������ʱ�䣺"<<clock()-decode_time<<"����"<<endl;
						if(got_picture)
						{
							sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

#if YUV420P_H264  
							//start=clock();
							 
							pFrame_o->data[0]=pFrameYUV->data[0];
							pFrame_o->data[1]=pFrameYUV->data[1];
							pFrame_o->data[2]=pFrameYUV->data[2];
							//PTS
							pFrame_o->pts=i;
							i++;

							//Encode ��ʼ����
							int encode_time=clock();
							int ret=avcodec_encode_video2(pCodecCtx_o,&pkt_o,pFrame_o,&got_picture);   //��pkt_o����Ӧ�õ���������
							if(ret < 0)
							{
								printf("Failed to encode! \n");
								GetLocalTime(&sys_time);
								error_log<<sys_time.wYear<<"/"<<sys_time.wMonth<<"/"<<sys_time.wDay<<"\t"<<
										sys_time.wHour<<":"<<sys_time.wMinute<<":"<<sys_time.wSecond<<":"<<
										sys_time.wMilliseconds<<"\t"<<"Failed to encode!"<<endl;
								return -1;
							}
							cout<<"�����ʱ��"<<clock()-encode_time<<"����"<<endl;
							if (got_picture==1)
							{
								printf("Succeed to encode frame: %5d\tsize:%5d\n",i,pkt_o.size);
								pkt_o.stream_index = video_st_o->index;
								//ret = av_write_frame(pFormatCtx_o, &pkt_o);
								
								pkt_o.pts=j;
								pkt_o.dts=pkt_o.pts;
								j++;
								/*if (pkt_o.stream_index==videoindex_test)
								{
									AVRational time_base=pFormatCtx_o->streams[videoindex_test]->time_base;
									AVRational time_base_q={1,AV_TIME_BASE};
									int64_t pts_time=av_rescale_q(pkt_o.pts,time_base,time_base_q);
									int64_t pkt_duration=av_rescale_q(pkt_o.duration,time_base,time_base_q);
									cout<<"pkt_durationΪ��"<<pkt_duration<<endl;
									int64_t now_time=av_gettime()-start_time;
									cout<<"now_timeΪ��"<<now_time<<endl;
									cout<<clock()-start1<<endl;
									if (pkt_duration>now_time)
									{
										cout<<"����ʱ�䣺"<<pkt_duration-now_time<<endl;
										av_usleep(pkt_duration-now_time-20000);
									}
								}*/
								
								AVStream *in_stream=pFormatCtx_o->streams[pkt_o.stream_index];
								AVStream *out_stream=ofmt_ctx->streams[pkt_o.stream_index];
								/* copy packet */
								//ת��PTS/DTS��Convert PTS/DTS��
								pkt_o.pts=av_rescale_q_rnd(pkt_o.pts,in_stream->time_base,out_stream->time_base,(AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
								cout<<"pkt_o.ptsΪ"<<pkt_o.pts<<endl;
								pkt_o.dts=pkt_o.pts;
								pkt_o.duration=av_rescale_q(pkt_o.duration,in_stream->time_base,out_stream->time_base);
								cout<<"pkt_o.duration"<<pkt_o.duration<<endl;
								pkt_o.pos=-1;
								//Print to Screen
								if(pkt_o.stream_index==videoindex_test)
								{
									printf("Send %8d video frames to output URL\n",frame_index);
									frame_index++;
								}
								cout<<"����ǰ��pkt��СΪ"<<pkt_o.size<<endl;
								//pkt_size_write<<"����ǰ��pkt��СΪ"<<pkt_o.size<<endl;
								ret = av_interleaved_write_frame(ofmt_ctx, &pkt_o);
								cout<<"����ǰ��pkt��СΪ"<<pkt_o.size<<endl;
								if (ret < 0) {
									printf( "Error muxing packet\n");
									GetLocalTime(&sys_time);
									error_log<<sys_time.wYear<<"/"<<sys_time.wMonth<<"/"<<sys_time.wDay<<"\t"<<
											    sys_time.wHour<<":"<<sys_time.wMinute<<":"<<sys_time.wSecond<<":"<<
												sys_time.wMilliseconds<<"\t"<<"Error muxing packet"<<endl;
									break;
								}

								av_free_packet(&pkt_o);
							}

							int test_time=clock();

							ret=flush_encoder(pFormatCtx_o,0);
							if (ret<0)
							{
								cout<<"Flushing encoder failed"<<endl;
								return -1;
							}
							//cout<<"�����ʱ��"<<clock()-start<<"����"<<endl;
							//fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y   
							//fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U  
							//fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V

#endif  

							//start=clock();
							SDL_LockYUVOverlay(bmp);
							bmp->pixels[0]=pFrameYUV->data[0];
							bmp->pixels[2]=pFrameYUV->data[1];
							bmp->pixels[1]=pFrameYUV->data[2];     
							bmp->pitches[0]=pFrameYUV->linesize[0];
							bmp->pitches[2]=pFrameYUV->linesize[1];   
							bmp->pitches[1]=pFrameYUV->linesize[2];
							SDL_UnlockYUVOverlay(bmp); 
							//�����Լ��������----------------
							SDL_DisplayYUVOverlay(bmp, &rect); 
							if (pText!=NULL)
							{
								ApplySurface(rect.x+rect.w/2,rect.y,pText,screen);
								//SDL_FreeSurface(pText);
							}
							SDL_Flip(screen);
							cout<<"SDL��ʾ��ʱΪ��"<<clock()-test_time<<"����"<<endl;
						}
					}
					av_free_packet(packet);
				}
				else
				{
					//Exit Thread
					av_write_trailer(pFormatCtx_o);

					if (video_st_o)
					{
						avcodec_close(video_st_o->codec);
						av_free(pFrame_o);
						av_free(picture_buf);
					}
					avio_close(pFormatCtx_o->pb);
					avformat_free_context(pFormatCtx_o);
					thread_exit=1;
					break;
				}
				cout<<"������ʾ�ͱ�����������̺�ʱ��"<<clock()-start1<<"����"<<endl<<endl;
			}
			else if(event.type==SDL_QUIT)
			{
				thread_exit=1;
				break;
			}
			
		}
		sws_freeContext(img_convert_ctx);


#if YUV420P_H264
		//write file trailer
		av_write_trailer(pFormatCtx_o);
		//Clean
		if (video_st_o)
		{
			avcodec_close(video_st_o->codec);
			av_free(pFrame_o);
			av_free(picture_buf);
		}
		avio_close(pFormatCtx_o->pb);
#endif 

#if OUTPUT_YUV420P 
		fclose(fp_yuv);
#endif 


		SDL_Quit();

		av_free(out_buffer);
		av_free(pFrameYUV);
		av_free(pFrame);
		avcodec_close(pCodecCtx);
		avcodec_close(pCodecCtx_o);
		avformat_close_input(&pFormatCtx);
		avformat_close_input(&pFormatCtx_o);

		error_log.close();

end:
		//avformat_close_input(&pFormatCtx);
		/* close output */
		if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
			avio_close(ofmt_ctx->pb);
		avformat_free_context(ofmt_ctx);
		if (ret < 0 && ret != AVERROR_EOF) 
		{
			printf( "Error occurred.\n");
			return -1;
		}

		return 0;
}