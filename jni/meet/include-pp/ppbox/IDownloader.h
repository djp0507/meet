// IDownloader.h

#ifndef _PPBOX_PPBOX_I_DOWNLOADER_H_
#define _PPBOX_PPBOX_I_DOWNLOADER_H_

#include "IPpbox.h"

#if __cplusplus
extern "C" {
#endif // __cplusplus

    // refine
    typedef void * PPBOX_Download_Hander;

    //��һ����������
    PPBOX_DECL PP_int32 FileDownloadOpenItem(
                char const * playlink,
                char const * format,
                char const * save_filename,
                PPBOX_Download_Hander * handle);

    //�ر�ָ������������
    PPBOX_DECL void FileDownloadCloseItem(PPBOX_Download_Hander hander);

    //��ȡ��ǰ���ض���������ʵ���ĸ���
    PPBOX_DECL PP_uint32 GetFileDownloadItemCount();

    typedef struct PPboxDownloadStatistic
    {
        PP_uint64 total_size;
        PP_uint64 finish_size;
        PP_uint32 speed; // B/s
    }Download_Statistic;

    // ��ȡָ������������ʵʱͳ����Ϣ
    PPBOX_DECL PP_int32 GetFileDownloadItemInfo(
        PPBOX_Download_Hander hander,
        Download_Statistic * stat);

    // �������ز�����name��ʽҪ�����value��ʾֵ
    PPBOX_DECL PP_int32 SetParamenter(char const * name, char const * value);

    // ��ȡ������
    PPBOX_DECL PP_int32 DownloadLastError(PPBOX_Download_Hander hander);

#if __cplusplus
}
#endif // __cplusplus

#endif // _PPBOX_PPBOX_I_DOWNLOADER_H_
