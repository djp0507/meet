package android.pplive.media.scan;

import java.io.File;
import java.io.FileFilter;
import java.util.Arrays;
import java.util.Stack;

import android.pplive.media.util.IoUtil;

public class Scanner implements OnScannedListener<File> {

	@SuppressWarnings("unused")
	private static final String TAG = "ppmedia/Scanner";

	private static Scanner sScanner = null;

	// Singleton
	static synchronized Scanner getInstance() {

		if (sScanner == null) {
			sScanner = new Scanner();
		}
		return sScanner;
	}

	private Scanner() { }

	private OnScannedListener<File> mOnScannedListener;

	public void setOnScannedListener(OnScannedListener<File> listener) {
		mOnScannedListener = listener;
	}

	public void scan(final File fileToScan) {
		scan(fileToScan, null);
	}
	
	public void scan(final File fileToScan, FileFilter filter) {
	    scan(fileToScan, filter, true);
	}

	public void scan(final File fileToScan, FileFilter filter, boolean recursively) {
	    if (IoUtil.isAccessible(fileToScan)){
	        if (fileToScan.isDirectory()){
	            scan(fileToScan.listFiles(filter), filter, recursively);
	        }
	        else {
	            scan(new File[] { fileToScan }, filter, recursively);
	        }
	    }
	}

	public void scan(final File[] filesToScan, FileFilter filter) {
		scan(filesToScan, null, true);
	}

	public void scan(final File[] files, FileFilter filter, boolean recursively) {
		if (files != null && files.length > 0) {
			Stack<File> stack = new Stack<File>();
			stack.addAll(Arrays.asList(files));
			while (!stack.isEmpty()) {
				File file = stack.pop();
				if (!IoUtil.isAccessible(file)) {
					continue;
				}
				
				if (file.isDirectory()) {
				    if (recursively){
	                    File[] subFiles = file.listFiles(filter);
	                    if (subFiles != null) {
	                        stack.addAll(Arrays.asList(subFiles));
	                    }
				    }
				} else if (file.isFile()) {
					onScanned(file);
				}
			}
		}
	}
	
	@Override
	public void onScanned(File f) {
		if (mOnScannedListener != null) {
			mOnScannedListener.onScanned(f);
		}
	}
}
