package android.pplive.media.util;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileFilter;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.util.regex.Pattern;

import android.content.Context;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Build.VERSION;

public class DeviceInfoUtil
{
    private final static String MEM_INFO_FILE = "/proc/meminfo";

    private final static String CPU_DIR = "/sys/devices/system/cpu/";

    private final static String CPU_FREQ_FILE = CPU_DIR + "cpu0/cpufreq/cpuinfo_max_freq";

    public static String getAppVersion(Context context)
    {
        try
        {
            String pkName = context.getPackageName();
            return context.getPackageManager().getPackageInfo(pkName, 0).versionName;
        }
        catch (NameNotFoundException e)
        {
            LogUtils.error("NameNotFoundException", e);
            return "";
        }
    }

    public static String getSystemVersion()
    {
        return VERSION.RELEASE;
    }

    public static String getTotalMemory()
    {
    		BufferedReader br = null;
        try
        {
            br = new BufferedReader(new InputStreamReader(new FileInputStream(MEM_INFO_FILE)));
            String line = br.readLine();// 读取/proc/meminfo第一行
            line = line.replace(" ", "");
            String[] info = line.split(":");
            return info[info.length - 1];
        }
        catch (Exception e)
        {
            LogUtils.error("Exception", e);
            return "";
        } finally {
        		
        		if (null != br) {
        			try {
        				br.close();
        				br = null;
        			} catch (Throwable t) {
        				
        			}
        		}
        }
    }

    public static int getCpuCoresNum()
    {
        try
        {
            File dir = new File(CPU_DIR);
            File[] files = dir.listFiles(new FileFilter()
            {
                @Override
                public boolean accept(File pathname)
                {
                    if (Pattern.matches("cpu[0-9]", pathname.getName()))
                    {
                        return true;
                    }
                    return false;
                }
            });
            return files.length;
        }
        catch (Exception e)
        {
            LogUtils.error("Exception", e);
            return 1;
        }
    }

    public static String getCpuFreq()
    {
    		BufferedReader br = null;
        try
        {
            br = new BufferedReader(new InputStreamReader(new FileInputStream(CPU_FREQ_FILE)));
            String line = br.readLine();
            int freq = Integer.parseInt(line.trim()) / 1000; // MHz
            return String.format("%dMHz", freq);
        }
        catch (Exception e)
        {
            LogUtils.error("Exception", e);
            return "";
        } finally {
        		if (null != br) {
        			try {
        				br.close();
        			} catch (Throwable t) {
        				
        			}
        		}
        }
    }
}
