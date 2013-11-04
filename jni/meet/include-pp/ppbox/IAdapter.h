// IAdapter.h

#ifndef _PPBOX_PPBOX_I_ADAPTER_H_
#define _PPBOX_PPBOX_I_ADAPTER_H_

#include "IPpbox.h"

#if __cplusplus
extern "C" {
#endif // __cplusplus

    //��һ��������Ƶ
    PPBOX_DECL PP_int32 Adapter_Open(
                           PP_char const * playlink,
                           PP_char const * format);

    typedef void (*Adapter_Open_Callback)(PP_int32);

    //�첽��һ��������Ƶ
    PPBOX_DECL void Adapter_AsyncOpen(
                         PP_char const * playlink, 
                         PP_char const * format,
                         Adapter_Open_Callback callback);

    //������ͣ
    PPBOX_DECL PP_int32 Adapter_Pause();

    //����ָ�
    PPBOX_DECL PP_int32 Adapter_Resume();

    //ǿ���������
    PPBOX_DECL void Adapter_Close();

    //����ĳ��ʱ�̿�ʼ����
    PPBOX_DECL PP_int32 Adapter_Seek(
                          PP_uint32 start_time);

    //���»�ȡͷ������
    PPBOX_DECL void Adapter_Reset();

    //��ȡ����ӰƬ����
    PPBOX_DECL PP_int32 Adapter_Read(
                         unsigned char * buffer,
                         PP_uint32 buffer_size,
                         PP_uint32 & read_size);

    enum Adapter_PlayStatusEnum
    {
        ppbox_adapter_closed = 0, 
        ppbox_adapter_playing, 
        ppbox_adapter_buffering, 
        ppbox_adapter_paused, 
    };

    typedef struct tag_Apapter_PlayStatistic
    {
        PP_uint32 length;           //���ṹ��ĳ���
        PP_int32  play_status;       //����״̬ 0-δ���� 1-playing̬ 2-buffering̬ 3-Pausing̬
        PP_uint32 buffering_present;//���Ż���ٷֱ� 10 ��ʾ 10%
        PP_uint32 buffer_time;      //���ػ��������ݵ���ʱ��
    } Adapter_PlayStatistic;

    //��ò�����Ϣ
    //����ֵ: ������
    //    ppbox_success      ��ʾ�ɹ�
    //    ������ֵ��ʾʧ��
    PPBOX_DECL PP_int32 Adapter_GetPlayMsg(
        Adapter_PlayStatistic * statistic_Msg);

    typedef struct MediaFileInfo
    {
        boost::uint32_t duration;
        // video
        boost::uint32_t frame_rate;
        boost::uint32_t width;
        boost::uint32_t height;
        // audio
        boost::uint32_t   channel_count;
        PP_uint32         sample_size;
        PP_uint32         sample_rate;
    }Adapter_Mediainfo;

    PPBOX_DECL PP_int32 Adapter_GetMediaInfo(
        Adapter_Mediainfo * media_info);

    PPBOX_DECL PP_int32 Adapter_GetPlayTime(
        PP_uint64 * time);

#if __cplusplus
}
#endif // __cplusplus

#endif // _PPBOX_PPBOX_I_ADAPTER_H_
