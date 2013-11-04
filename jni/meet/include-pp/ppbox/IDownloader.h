// IDownloader.h

#ifndef _PPBOX_PPBOX_I_DOWNLOADER_H_
#define _PPBOX_PPBOX_I_DOWNLOADER_H_

#include "IPpbox.h"

#if __cplusplus
extern "C" {
#endif // __cplusplus

    // refine
    typedef void * PPBOX_Download_Hander;

    //打开一个下载用例
    PPBOX_DECL PP_int32 FileDownloadOpenItem(
                char const * playlink,
                char const * format,
                char const * save_filename,
                PPBOX_Download_Hander * handle);

    //关闭指定的下载用例
    PPBOX_DECL void FileDownloadCloseItem(PPBOX_Download_Hander hander);

    //获取当前下载队列中下载实例的个数
    PPBOX_DECL PP_uint32 GetFileDownloadItemCount();

    typedef struct PPboxDownloadStatistic
    {
        PP_uint64 total_size;
        PP_uint64 finish_size;
        PP_uint32 speed; // B/s
    }Download_Statistic;

    // 获取指定下载用例的实时统计信息
    PPBOX_DECL PP_int32 GetFileDownloadItemInfo(
        PPBOX_Download_Hander hander,
        Download_Statistic * stat);

    // 设置下载参数，name表式要设置项，value表示值
    PPBOX_DECL PP_int32 SetParamenter(char const * name, char const * value);

    // 获取错误码
    PPBOX_DECL PP_int32 DownloadLastError(PPBOX_Download_Hander hander);

#if __cplusplus
}
#endif // __cplusplus

#endif // _PPBOX_PPBOX_I_DOWNLOADER_H_
