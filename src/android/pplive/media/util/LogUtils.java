/**
 * Copyright (C) 2013 PPTV
 *
 */
package android.pplive.media.util;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.RandomAccessFile;
import java.text.SimpleDateFormat;
import java.util.Calendar;

import android.os.Environment;
import android.os.Process;
import android.pplive.media.MeetSDK;
import android.text.TextUtils;
import android.util.Log;

/**
 * <播放器日志类>
 * 
 * @author johnxie
 * @version [版本号, 2013-7-17]
 * @see [相关类/方法]
 * @since [产品/模块版本]
 */
public class LogUtils
{
    /**
     * 日志级别
     */
    public static int LOG_LEVEL = Log.ERROR + 1;

    /**
     * 异常栈位移
     */
    public static final int EXCEPTION_STACK_INDEX = 3;

    private static final String TAG = "MeetSDK_LogUtils";

    private final static String PPTV_HOME = Environment.getExternalStorageDirectory() + "/pptv/";

    private final static String TEMP_PATH = PPTV_HOME + "tmp/";

    private final static String LOG_PATH = TEMP_PATH + "player.log";

    private final static String INFO_PATH = TEMP_PATH + "deviceinfo";

    private static String outputpath;

    private static int BUFFER_LENGTH = 0;

    private static int FILE_LENGTH = 100 * 1024;

    private static StringBuilder buffer = new StringBuilder();

    private static long offset = 0;

    private static RandomAccessFile file = null;

    private static final SimpleDateFormat FORMAT = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");

    /**
     * <构造函数>
     */
    private LogUtils()
    {
    }

    public static boolean init(String path)
    {
        outputpath = path;
        boolean hasLogPath = makeParentPath(outputpath);
        boolean hasTempPath = makePath(TEMP_PATH);
        return hasLogPath && hasTempPath;
    }

    private static void logDeviceInfo()
    {
        try
        {
            BufferedWriter bw = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(INFO_PATH)));
            StringBuffer sb = new StringBuffer();
            sb.append("MeetSDK version:").append(MeetSDK.getVersion()).append("\n");
            sb.append("Android version:").append(DeviceInfoUtil.getSystemVersion()).append("\n");
            sb.append("CPU cores:").append(DeviceInfoUtil.getCpuCoresNum()).append("\n");
            sb.append("CPU Freq:").append(DeviceInfoUtil.getCpuFreq()).append("\n");
            sb.append("Memory:").append(DeviceInfoUtil.getTotalMemory()).append("\n");
            bw.write(sb.toString());
            bw.close();
        }
        catch (IOException e)
        {
            e.printStackTrace();
        }
    }

    public static void makePlayerlog()
    {
        logDeviceInfo();
        try
        {
            BufferedWriter bw = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(outputpath)));
            BufferedReader br = new BufferedReader(new InputStreamReader(new FileInputStream(INFO_PATH)));
            String line = "";
            while ((line = br.readLine()) != null)
            {
                bw.write(line);
                bw.write('\n');
            }
            br.close();
            File logfile = new File(LOG_PATH);
            if (logfile.exists())
            {
                bw.write("-----------------\n");
                br = new BufferedReader(new InputStreamReader(new FileInputStream(logfile)));
                while ((line = br.readLine()) != null)
                {
                    bw.write(line);
                    bw.write('\n');
                }
                br.close();
            }
            bw.close();
        }
        catch (IOException e)
        {
            e.printStackTrace();
        }
    }

    private static boolean makePath(String path)
    {
        if (TextUtils.isEmpty(path))
        {
            return false;
        }
        File dir = new File(path);
        if (!dir.exists())
        {
            return dir.mkdirs();
        }
        else
            return true;
    }

    private static boolean makeParentPath(String filename)
    {
        if (TextUtils.isEmpty(filename))
        {
            return false;
        }
        File file = new File(filename);
        return makePath(file.getParentFile().getAbsolutePath());
    }

    /**
     * verbose级别的日志
     * 
     * @param msg 打印内容
     * @see [类、类#方法、类#成员]
     */
    public static void verbose(String msg)
    {
        if (Log.VERBOSE >= LOG_LEVEL)
        {
            Log.v(getTag(), msg);
        }
    }

    /**
     * debug级别的日志
     * 
     * @param msg 打印内容
     * @see [类、类#方法、类#成员]
     */
    public static void debug(String msg)
    {
        if (Log.DEBUG >= LOG_LEVEL)
        {
            Log.d(getTag(), msg);
        }
    }

    /**
     * info级别的日志
     * 
     * @param msg 打印内容
     * @see [类、类#方法、类#成员]
     */
    public static void info(String msg)
    {
        String tag = getTag();
        writeFile(FORMAT.format(Calendar.getInstance().getTime()) + " " + Process.myPid() + " " + Process.myTid()
                + " I " + tag + ": " + msg, offset);
        if (Log.INFO >= LOG_LEVEL)
        {
            Log.i(tag, msg);
        }
    }
    
    /**
     * warn级别的日志
     * 
     * @param msg 打印内容
     * @see [类、类#方法、类#成员]
     */
    public static void warn(String msg)
    {
        String tag = getTag();
        writeFile(FORMAT.format(Calendar.getInstance().getTime()) + " " + Process.myPid() + " " + Process.myTid()
                + " W " + tag + ": " + msg, offset);
        if (Log.WARN >= LOG_LEVEL)
        {
            Log.w(tag, msg);
        }
    }

    /**
     * error级别的日志
     * 
     * @param msg 打印内容
     * @see [类、类#方法、类#成员]
     */
    public static void error(String msg)
    {
        String tag = getTag();
        writeFile(FORMAT.format(Calendar.getInstance().getTime()) + " " + Process.myPid() + " " + Process.myTid()
                + " E " + tag + ": " + msg, offset);
        if (Log.ERROR >= LOG_LEVEL)
        {
            Log.e(tag, msg);
        }
    }

    public static void error(String msg, Throwable tr)
    {
        String tag = getTag();
        writeFile(FORMAT.format(Calendar.getInstance().getTime()) + " " + Process.myPid() + " " + Process.myTid()
                + " E " + tag + ":" + msg + ":" + Log.getStackTraceString(tr), offset);
        if (Log.ERROR >= LOG_LEVEL)
        {
            Log.e(tag, msg, tr);
        }
    }

    public static String getTag()
    {
        return getTag(EXCEPTION_STACK_INDEX);
    }

    /**
     * 获取日志的标签 格式：类名_方法名_行号 （需要权限：android.permission.GET_TASKS）
     * 
     * @return tag
     * @see [类、类#方法、类#成员]
     */
    public static String getTag(int stackLevel)
    {
        try
        {
            Exception exception = new LogException();
            if (exception.getStackTrace() == null || exception.getStackTrace().length <= stackLevel)
            {
                return "***";
            }
            StackTraceElement element = exception.getStackTrace()[stackLevel];

            String className = element.getClassName();

            int index = className.lastIndexOf(".");
            if (index > 0)
            {
                className = className.substring(index + 1);
            }

            return className + "_" + element.getMethodName() + "_" + element.getLineNumber();

        }
        catch (Throwable e)
        {
            e.printStackTrace();
            return "***";
        }
    }

    /**
     * 取日志标签用的的异常类，只是用于取得日志标签
     */
    private static class LogException extends Exception
    {
        /**
         * 注释内容
         */
        private static final long serialVersionUID = 1L;
    }

    public static synchronized void writeFile(String msg, long off)
    {
        try
        {
            if (file == null)
            {
                file = new RandomAccessFile(LOG_PATH, "rw");
            }

            buffer.append(msg);
            if (buffer.length() > BUFFER_LENGTH)
            {
                try
                {
                    if (buffer.length() + off > FILE_LENGTH)
                    {
                        String subBufferEnd = buffer.substring(0, (int) (FILE_LENGTH - off));
                        subBufferEnd = subBufferEnd + "\n";
                        String subBufferStart = buffer.substring((int) (FILE_LENGTH - off), buffer.length() - 1);
                        file.seek(subBufferStart.length());
                        byte[] b = new byte[(int) (FILE_LENGTH - buffer.length())];
                        file.read(b);
                        file.seek(0);
                        file.setLength(0);
                        file.write(subBufferStart.getBytes());
                        String exist = new String(b);
                        int pos = exist.indexOf("\n");
                        exist = exist.substring(pos);
                        file.write(exist.getBytes());
                        file.seek(off);
                        file.write(subBufferEnd.getBytes());
                        offset = subBufferStart.length();
                    }
                    else
                    {
                        file.seek(off);
                        file.write(buffer.toString().getBytes());
                        offset = off + buffer.length();
                    }
                }
                catch (IOException e)
                {
                    Log.e(TAG, e.toString());
                }
                finally
                {
                    buffer.delete(0, buffer.length() - 1);
                    buffer.append("\n");
                }
            }
            else
            {
                buffer.append("\n");
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    public static void nativeLog(int level, String text)
    {
        SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS");
        String tag = null;
        switch (level)
        {
            case Log.ERROR:
                tag = " E ";
                break;
            case Log.WARN:
                tag = " W ";
                break;
            case Log.INFO:
                tag = " I ";
                break;
        }

        if (!TextUtils.isEmpty(tag))
        {
            writeFile(format.format(Calendar.getInstance().getTime()) + " " + Process.myPid() + " " + Process.myTid()
                    + tag + TAG + ": " + text, offset);
        }
    }
}
