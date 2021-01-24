// =============================================================================
//	ImageWindow.hpp
//
//	MIT License
//
//	Copyright (c) 2007-2018 Dairoku Sekiguchi
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in all
//	copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//	SOFTWARE.
// =============================================================================
/*!
	\file		ImageWindow.hpp
	\author		Dairoku Sekiguchi
	\version	3.0 (Release.04)
	\date		2018/07/07
	\brief		Single file image displaying utility class
*/
#ifndef __IMAGE_WINDOW_H
#define __IMAGE_WINDOW_H

#pragma warning(disable:4996)
#ifdef _M_IX86
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif _M_X64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif



// -----------------------------------------------------------------------------
// 	include files
// -----------------------------------------------------------------------------
#include <windows.h>
#include <string.h>
#include <process.h>
#include <vector>
#include <commctrl.h>
#include <math.h>


// -----------------------------------------------------------------------------
// 	macros
// -----------------------------------------------------------------------------
#define	IMAGE_PALLET_SIZE_8BIT		256
#define	IMAGE_WINDOW_CLASS_NAME		TEXT("ImageWindow")
#define	IMAGE_WINDOW_DEFAULT_SIZE	160
#define	IMAGE_WINDOW_AUTO_POS		-100000
#define	IMAGE_WINDOW_DEFAULT_POS		20
#define	IMAGE_WINDOW_POS_SPACE		20
#define	IMAGE_FILE_NAME_BUF_LEN		256
#define	IMAGE_STR_BUF_SIZE			256
#define DEFAULT_WINDOW_NAME			"Untitled"
#define	MONITOR_ENUM_MAX				32

#define IMAGE_ZOOM_STEP				1
#define MOUSE_WHEEL_STEP				60


// -----------------------------------------------------------------------------
// 	typedefs
// -----------------------------------------------------------------------------
typedef struct
{
	BITMAPINFOHEADER		Header;
	RGBQUAD					RGBQuad[IMAGE_PALLET_SIZE_8BIT];
} ImageBitmapInfoMono8;


// -----------------------------------------------------------------------------
//	ImageWindow class
// -----------------------------------------------------------------------------
//!
/*!
	The class for Image Data Container and Window
*/
class ImageWindow
{
public:
	// enum --------------------------------------------------------------------
	enum CursorMode
	{
		CURSOR_MODE_SCROLL_TOOL	=	1,
		CURSOR_MODE_ZOOM_TOOL,
		CURSOR_MODE_INFO_TOOL
	};

	// -------------------------------------------------------------------------
	//	ImageWindow(...)
	// -------------------------------------------------------------------------
	//!	ImageWindow Class Constructor
	/*!
		xxxx
	*/
	ImageWindow(const char *inWindowName = NULL,
						int inPosX = IMAGE_WINDOW_AUTO_POS,
						int inPosY = IMAGE_WINDOW_DEFAULT_POS)
	{
		static int	sPrevPosX	= 0;
		static int	sPrevPosY	= 0;
		static int	sWindowNum	= 0;

		mWindowState			= WINDOW_INIT_STATE;
		mWindowTitle			= NULL;
		mFileNameIndex			= 0;
		mWindowH				= NULL;
		mToolbarH				= NULL;
		mStatusbarH				= NULL;
		mRebarH					= NULL;
		mMenuH					= NULL;
		mMutexHandle			= NULL;
		mEventHandle			= NULL;
		mThreadHandle			= NULL;
		mIsThreadRunning		= false;
		mBitmapInfo				= NULL;
		mBitmapInfoSize			= 0;
		mBitmapBits				= NULL;
		mBitmapBitsSize			= 0;
		mModuleH				= GetModuleHandle(NULL);
		mCursorMode				= CURSOR_MODE_SCROLL_TOOL;

		mIsHiDPI = false;
		mIsColorImage			= false;
		mIs16BitsImage			= false;

		mIsMouseDragging		= false;
		mIsFullScreenMode		= false;
		mIsMenubarEnabled		= true;
		mIsToolbarEnabled		= true;
		mIsStatusbarEnabled		= true;
		mIsPlotEnabled			= false;

		mFPSValue				= 0;

		mAllocatedImageBuffer	= NULL;
		mAllocated16BitsImageBuffer = NULL;
		mExternal16BitsImageBuffer = NULL;

		mDrawOverlayFunc		= NULL;
		mOverlayFuncData		= NULL;

		mImageClickNum			= 0;
		mLastImageClickX		= 0;
		mLastImageClickY		= 0;

		mIsMapModeEnabled		= false;
		mMapBottomValue			= 0;
		mMapTopValue			= 65535;
		mIsMapReverse			= false;
		mMapDirectMapLimit		= 0;

		if (inPosX == IMAGE_WINDOW_AUTO_POS ||
			inPosY == IMAGE_WINDOW_AUTO_POS)
		{
			if (sPrevPosX == 0 && sPrevPosY == 0)
			{
				mPosX = IMAGE_WINDOW_DEFAULT_POS;
				mPosY = IMAGE_WINDOW_DEFAULT_POS;
			}
			else
			{
				mPosX = sPrevPosX + IMAGE_WINDOW_POS_SPACE;
				mPosY = sPrevPosY + IMAGE_WINDOW_POS_SPACE;

				if (mPosX >= GetSystemMetrics(SM_CXMAXIMIZED) - IMAGE_WINDOW_POS_SPACE)
					mPosX = IMAGE_WINDOW_DEFAULT_POS;

				if (mPosY >= GetSystemMetrics(SM_CYMAXIMIZED) - IMAGE_WINDOW_POS_SPACE)
					mPosY = IMAGE_WINDOW_DEFAULT_POS;
			}

			sPrevPosX = mPosX;
			sPrevPosY = mPosY;
		}

		if (sWindowNum == 0)
		{
			// TODO: Add an option to skip the following call
			SetProcessDPIAware();
		}

		if (RegisterWindowClass() == 0)
		{
			printf("Error: RegisterWindowClass() failed\n");
			return;
		}

		char	windowName[IMAGE_STR_BUF_SIZE];
		if (inWindowName == NULL)
		{
			sprintf_s(windowName, IMAGE_STR_BUF_SIZE, "%s%d", DEFAULT_WINDOW_NAME, sWindowNum);
			inWindowName = windowName;
		}

		size_t	bufSize = strlen(inWindowName) + 1;
		mWindowTitle = new char[bufSize];
		if (mWindowTitle == NULL)
		{
			printf("Error: Can't allocate mWindowTitle\n");
			return;
		}
		strcpy_s(mWindowTitle, bufSize, inWindowName);

		mMutexHandle = CreateMutex(NULL, false, NULL);
		if (mMutexHandle == NULL)
		{
			printf("Error: Can't create Mutex object\n");
			delete mWindowTitle;
			return;
		}

		mEventHandle = CreateEvent(NULL, false, false, NULL);
		if (mEventHandle == NULL)
		{
			printf("Error: Can't create Event object\n");
			CloseHandle(mMutexHandle);
			delete mWindowTitle;
			return;
		}

		sWindowNum++;
	}

	virtual ~ImageWindow()
	{
		if (mWindowState == WINDOW_OPEN_STATE)
			PostMessage(mWindowH, WM_CLOSE, 0, 0);

		if (mThreadHandle != NULL)
		{
			WaitForSingleObject(mThreadHandle, INFINITE);
			CloseHandle(mThreadHandle);
		}

		if (mMutexHandle != NULL)
			CloseHandle(mMutexHandle);

		if (mEventHandle != NULL)
			CloseHandle(mEventHandle);

		if (mAllocatedImageBuffer != NULL)
			delete mAllocatedImageBuffer;

		if (mWindowTitle != NULL)
			delete mWindowTitle;
	}

	void	ShowWindow()
	{
		if (mWindowState != WINDOW_INIT_STATE)
			return;

		if (mIsThreadRunning == true)
			return;

		if (mThreadHandle != NULL)
		{
			::CloseHandle(mThreadHandle);
		}
		//	Must use _beginthreadex instead of _beginthread, CreateThread
		mIsThreadRunning = true;
		mThreadHandle = (HANDLE )_beginthreadex(
			NULL,
			0,
			ThreadFunc,
			this,
			0,
			NULL);
		if (mThreadHandle == NULL)
		{
			mIsThreadRunning = false;
			printf("Error: Can't create process thread\n");
			CloseHandle(mEventHandle);
			CloseHandle(mMutexHandle);
			delete mWindowTitle;
			return;
		}

		WaitForSingleObject(mEventHandle, INFINITE);
	}

	void	CloseWindow()
	{
		if (mWindowState == WINDOW_OPEN_STATE)
			PostMessage(mWindowH, WM_CLOSE, 0, 0);
	}

	bool	IsWindowOpen()
	{
		if (mWindowState == WINDOW_OPEN_STATE)
			return true;

		return false;
	}

	bool	IsWindowClosed()
	{
		if (mWindowState == WINDOW_CLOSED_STATE)
			return true;

		return false;
	}

	bool	WaitForWindowClose(DWORD inTimeout = INFINITE)
	{
		if (mThreadHandle == NULL)
			return false;

		if (WaitForSingleObject(mThreadHandle, inTimeout) == WAIT_OBJECT_0)
			return true;
	return false;
	}

	static bool	WaitForWindowCloseMulti(ImageWindow *inWindowArray[], int inArrayLen,
											bool inWaitAll = false, DWORD inTimeout = INFINITE)
	{
		std::vector< HANDLE >	handles;

		for (int i = 0; i < inArrayLen; i++)
			if (inWindowArray[i]->mThreadHandle != NULL)
				handles.push_back(inWindowArray[i]->mThreadHandle);

		if (WaitForMultipleObjects((DWORD )handles.size(), &(handles[0]), inWaitAll, inTimeout) == WAIT_TIMEOUT)
			return false;

		return true;
	}

	void	SetColormap(int inIndex = 0)
	{
		if (mBitmapInfo == NULL)
			return;

		if (mBitmapInfo->biBitCount != 8)
			return;

		ImageBitmapInfoMono8	*bitmapInfo = (ImageBitmapInfoMono8 *)mBitmapInfo;

		if (inIndex == 0)
		{
			for (int i = 0; i < 255; i++)
			{
				bitmapInfo->RGBQuad[i].rgbBlue		= i;
				bitmapInfo->RGBQuad[i].rgbGreen		= i;
				bitmapInfo->RGBQuad[i].rgbRed		= i;
				bitmapInfo->RGBQuad[i].rgbReserved	= 0;
			}
			bitmapInfo->RGBQuad[0].rgbRed	= 0;
			bitmapInfo->RGBQuad[0].rgbGreen	= 0;
			bitmapInfo->RGBQuad[0].rgbBlue	= 0;
			bitmapInfo->RGBQuad[1].rgbRed	= 255;
			bitmapInfo->RGBQuad[1].rgbGreen	= 255;
			bitmapInfo->RGBQuad[1].rgbBlue	= 255;
			return;
		}
		if (inIndex == 1)
		{
			for (int i = 0; i < 64; i++)
			{
				bitmapInfo->RGBQuad[i      ].rgbRed		= 0;
				bitmapInfo->RGBQuad[i      ].rgbGreen	= i * 4;
				bitmapInfo->RGBQuad[i      ].rgbBlue	= 255;

				bitmapInfo->RGBQuad[i +  64].rgbRed		= 0;
				bitmapInfo->RGBQuad[i +  64].rgbGreen	= 255;
				bitmapInfo->RGBQuad[i +  64].rgbBlue	= (255 - i * 4);

				bitmapInfo->RGBQuad[i + 128].rgbRed		= i * 4;
				bitmapInfo->RGBQuad[i + 128].rgbGreen	= 255;
				bitmapInfo->RGBQuad[i + 128].rgbBlue	= 0;

				bitmapInfo->RGBQuad[i + 192].rgbRed		= 255;
				bitmapInfo->RGBQuad[i + 192].rgbGreen	= (255 - i * 4);
				bitmapInfo->RGBQuad[i + 192].rgbBlue	= 0;
			}
			bitmapInfo->RGBQuad[0].rgbRed	= 0;
			bitmapInfo->RGBQuad[0].rgbGreen	= 0;
			bitmapInfo->RGBQuad[0].rgbBlue	= 0;
			bitmapInfo->RGBQuad[1].rgbRed	= 255;
			bitmapInfo->RGBQuad[1].rgbGreen	= 255;
			bitmapInfo->RGBQuad[1].rgbBlue	= 255;
			return;
		}
		if (inIndex == 2)
		{
			const unsigned char tableR[256] = {
				60, 61, 62, 63, 64, 66, 67, 68, 69, 70, 71, 73, 74, 75, 76, 77, 79,
				80, 81, 82, 84, 85, 86, 87, 89, 90, 91, 93, 94, 95, 96, 98, 99, 100,
				102, 103, 104, 106, 107, 108, 110, 111, 112, 114, 115, 116, 118, 119,
				120, 122, 123, 124, 126, 127, 129, 130, 131, 133, 134, 135, 137, 138,
				140, 141, 142, 144, 145, 147, 148, 149, 151, 152, 153, 155, 156, 158,
				159, 160, 162, 163, 164, 166, 167, 168, 170, 171, 172, 174, 175, 176,
				178, 179, 180, 182, 183, 184, 185, 187, 188, 189, 190, 192, 193, 194,
				195, 197, 198, 199, 200, 201, 203, 204, 205, 206, 207, 208, 209, 210,
				211, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225,
				226, 227, 228, 229, 230, 231, 232, 232, 233, 234, 235, 236, 236, 237,
				238, 238, 239, 240, 240, 241, 241, 242, 242, 243, 243, 244, 244, 245,
				245, 245, 245, 246, 246, 246, 246, 247, 247, 247, 247, 247, 247, 247,
				247, 247, 247, 247, 247, 247, 247, 247, 247, 247, 246, 246, 246, 246,
				245, 245, 245, 244, 244, 244, 243, 243, 242, 242, 241, 241, 240, 240,
				239, 238, 238, 237, 236, 236, 235, 234, 233, 233, 232, 231, 230, 229,
				228, 227, 227, 226, 225, 224, 223, 222, 221, 220, 218, 217, 216, 215,
				214, 213, 212, 210, 209, 208, 207, 205, 204, 203, 202, 200, 199, 198,
				196, 195, 193, 192, 190, 189, 188, 186, 185, 183, 181, 180
			};
			const unsigned char tableG[256] = {
				78, 80, 81, 83, 85, 87, 88, 90, 92, 93, 95, 97, 99, 100, 102, 104,
				105, 107, 109, 110, 112, 114, 115, 117, 119, 120, 122, 123, 125,
				127, 128, 130, 131, 133, 135, 136, 138, 139, 141, 142, 144, 145,
				147, 148, 150, 151, 153, 154, 156, 157, 158, 160, 161, 163, 164,
				165, 167, 168, 169, 171, 172, 173, 174, 176, 177, 178, 179, 181,
				182, 183, 184, 185, 186, 187, 188, 190, 191, 192, 193, 194, 195,
				196, 197, 198, 199, 199, 200, 201, 202, 203, 204, 205, 205, 206,
				207, 208, 208, 209, 210, 210, 211, 212, 212, 213, 213, 214, 214,
				215, 215, 216, 216, 217, 217, 217, 218, 218, 219, 219, 219, 219,
				220, 220, 220, 220, 220, 220, 221, 221, 220, 220, 219, 219, 218,
				218, 217, 216, 216, 215, 215, 214, 213, 212, 212, 211, 210, 209,
				209, 208, 207, 206, 205, 204, 203, 202, 201, 200, 199, 198, 197,
				196, 195, 194, 193, 192, 191, 190, 188, 187, 186, 185, 184, 182,
				181, 180, 178, 177, 176, 174, 173, 172, 170, 169, 167, 166, 164,
				163, 161, 160, 158, 157, 155, 154, 152, 151, 149, 147, 146, 144,
				142, 141, 139, 137, 136, 134, 132, 130, 129, 127, 125, 123, 121,
				120, 118, 116, 114, 112, 110, 108, 106, 104, 102, 100, 98, 96,
				94, 92, 90, 88, 86, 84, 82, 80, 78, 75, 73, 71, 69, 66, 64, 62,
				59, 57, 54, 51, 49, 46, 43, 40, 37, 34, 30, 26, 22, 17, 11, 4
 			};
			const unsigned char tableB[256] = {
				194, 195, 197, 198, 200, 201, 203, 204, 206, 207, 209, 210, 211, 213,
				214, 215, 217, 218, 219, 221, 222, 223, 224, 225, 226, 228, 229, 230,
				231, 232, 233, 234, 235, 236, 237, 238, 239, 239, 240, 241, 242, 243,
				243, 244, 245, 246, 246, 247, 247, 248, 249, 249, 250, 250, 251, 251,
				252, 252, 252, 253, 253, 253, 254, 254, 254, 254, 254, 255, 255, 255,
				255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 254, 254, 254,
				253, 253, 253, 253, 252, 252, 251, 251, 251, 250, 250, 249, 248, 248,
				247, 247, 246, 245, 245, 244, 243, 243, 242, 241, 240, 239, 238, 238,
				237, 236, 235, 234, 233, 232, 231, 230, 229, 228, 227, 225, 224, 223,
				222, 221, 219, 218, 216, 215, 214, 212, 211, 209, 208, 206, 205, 203,
				202, 200, 199, 197, 196, 194, 193, 191, 190, 188, 187, 185, 184, 182,
				181, 179, 178, 176, 174, 173, 171, 170, 168, 167, 165, 163, 162, 160,
				159, 157, 156, 154, 152, 151, 149, 148, 146, 145, 143, 141, 140, 138,
				137, 135, 134, 132, 131, 129, 127, 126, 124, 123, 121, 120, 118, 117,
				115, 114, 112, 111, 109, 108, 106, 105, 103, 102, 100, 99, 97, 96, 95,
				93, 92, 90, 89, 88, 86, 85, 83, 82, 81, 79, 78, 77, 75, 74, 73, 71, 70,
				69, 67, 66, 65, 64, 62, 61, 60, 59, 57, 56, 55, 54, 53, 52, 50, 49, 48,
				47, 46, 45, 44, 43, 41, 40, 39, 38
			};
			for (int i = 0; i < 256; i++)
			{
				bitmapInfo->RGBQuad[i].rgbRed		= tableR[i];
				bitmapInfo->RGBQuad[i].rgbGreen		= tableG[i];
				bitmapInfo->RGBQuad[i].rgbBlue		= tableB[i];
			}
			bitmapInfo->RGBQuad[0].rgbRed	= 0;
			bitmapInfo->RGBQuad[0].rgbGreen	= 0;
			bitmapInfo->RGBQuad[0].rgbBlue	= 0;
			bitmapInfo->RGBQuad[1].rgbRed	= 255;
			bitmapInfo->RGBQuad[1].rgbGreen	= 255;
			bitmapInfo->RGBQuad[1].rgbBlue	= 255;
			printf("I was here\n");
			return;
		}

		if (inIndex == 3)
		{
			for (int j = 0; j < 4; j++)
			{
				for (int i = 0; i < 64; i++)
				{
					bitmapInfo->RGBQuad[i+j*64].rgbRed		= i * 3 + 64;
					bitmapInfo->RGBQuad[i+j*64].rgbGreen	= i * 3 + 64;
					bitmapInfo->RGBQuad[i+j*64].rgbBlue		= i * 3 + 64;
				}
			}
			bitmapInfo->RGBQuad[0].rgbRed	= 0;
			bitmapInfo->RGBQuad[0].rgbGreen	= 0;
			bitmapInfo->RGBQuad[0].rgbBlue	= 0;
			bitmapInfo->RGBQuad[1].rgbRed	= 255;
			bitmapInfo->RGBQuad[1].rgbGreen	= 255;
			bitmapInfo->RGBQuad[1].rgbBlue	= 255;
			return;
		}
		if (inIndex == 4)
		{
			for (int j = 0; j < 8; j++)
			{
				for (int i = 0; i < 32; i++)
				{
					bitmapInfo->RGBQuad[i+j*32].rgbRed		= i * 7 + 32;
					bitmapInfo->RGBQuad[i+j*32].rgbGreen	= i * 7 + 32;
					bitmapInfo->RGBQuad[i+j*32].rgbBlue		= i * 7 + 32;
				}
			}
			bitmapInfo->RGBQuad[0].rgbRed	= 0;
			bitmapInfo->RGBQuad[0].rgbGreen	= 0;
			bitmapInfo->RGBQuad[0].rgbBlue	= 0;
			bitmapInfo->RGBQuad[1].rgbRed	= 255;
			bitmapInfo->RGBQuad[1].rgbGreen	= 255;
			bitmapInfo->RGBQuad[1].rgbBlue	= 255;
			return;
		}
		if (inIndex == 5)
		{
			for (int j = 0; j < 16; j++)
			{
				for (int i = 0; i < 16; i++)
				{
					bitmapInfo->RGBQuad[i+j*16].rgbRed		= i * 15 + 16;
					bitmapInfo->RGBQuad[i+j*16].rgbGreen	= i * 15 + 16;
					bitmapInfo->RGBQuad[i+j*16].rgbBlue		= i * 15 + 16;
				}
			}
			bitmapInfo->RGBQuad[0].rgbRed	= 0;
			bitmapInfo->RGBQuad[0].rgbGreen	= 0;
			bitmapInfo->RGBQuad[0].rgbBlue	= 0;
			bitmapInfo->RGBQuad[1].rgbRed	= 255;
			bitmapInfo->RGBQuad[1].rgbGreen	= 255;
			bitmapInfo->RGBQuad[1].rgbBlue	= 255;
			return;
		}
		if (inIndex == 6)
		{
			for (int j = 0; j < 32; j++)
			{
				for (int i = 0; i < 8; i++)
				{
					bitmapInfo->RGBQuad[i+j* 8].rgbRed		= i * 32;
					bitmapInfo->RGBQuad[i+j* 8].rgbGreen	= i * 32;
					bitmapInfo->RGBQuad[i+j* 8].rgbBlue		= i * 32;
				}
			}
			bitmapInfo->RGBQuad[0].rgbRed	= 0;
			bitmapInfo->RGBQuad[0].rgbGreen	= 0;
			bitmapInfo->RGBQuad[0].rgbBlue	= 0;
			bitmapInfo->RGBQuad[1].rgbRed	= 255;
			bitmapInfo->RGBQuad[1].rgbGreen	= 255;
			bitmapInfo->RGBQuad[1].rgbBlue	= 255;
			return;
		}
	}

	void	SetImageBufferPtr(int inWidth, int inHeight, unsigned char *inImagePtr, bool inIsColor, bool inIsBottomUp = false, bool inIs16Bits = false)
	{
		DWORD	result;
		bool	doUpdateSize;

		result = WaitForSingleObject(mMutexHandle, INFINITE);
		if (result != WAIT_OBJECT_0)
		{
			printf("Error: WaitForSingleObject failed (SetMonoImageBufferPtr)\n");
			return;
		}

		doUpdateSize = CreateBitmapInfo(inWidth, inHeight, inIsColor, inIsBottomUp, inIs16Bits);

		if (mAllocatedImageBuffer != NULL)
		{
			delete mAllocatedImageBuffer;
			mAllocatedImageBuffer = NULL;
		}
		if (mAllocated16BitsImageBuffer != NULL)
		{
			delete mAllocated16BitsImageBuffer;
			mAllocated16BitsImageBuffer = NULL;
		}

		if (inIs16Bits == false)
		{
			mBitmapBits = inImagePtr;
			mExternal16BitsImageBuffer = NULL;
		}
		else
		{
			CreateNewImageBuffer(true);
			mExternal16BitsImageBuffer = (unsigned short *)inImagePtr;
		}
		ReleaseMutex(mMutexHandle);
		UpdateFPS();

		if (doUpdateSize)
			UpdateWindowSize();
		else
			UpdateImage();
	}

	void	AllocateImageBuffer(int inWidth, int inHeight, bool inIsColor, bool inIsBottomUp = false, bool inIs16Bits = false)
	{
		DWORD	result;
		bool	doUpdateSize;

		result = WaitForSingleObject(mMutexHandle, INFINITE);
		if (result != WAIT_OBJECT_0)
		{
			printf("Error: WaitForSingleObject failed (AllocateMonoImageBuffer)\n");
			return;
		}

		doUpdateSize = PrepareImageBuffers(inWidth, inHeight, inIsColor, inIsBottomUp, inIs16Bits);

		ReleaseMutex(mMutexHandle);

		if (doUpdateSize)
			UpdateWindowSize();
	}

	void	CopyIntoImageBuffer(int inWidth, int inHeight, const unsigned char *inImage, bool inIsColor, bool inIsBottomUp = false, bool inIs16Bits = false)
	{
		DWORD	result;
		bool	doUpdateSize;

		result = WaitForSingleObject(mMutexHandle, INFINITE);
		if (result != WAIT_OBJECT_0)
		{
			printf("Error: WaitForSingleObject failed (AllocateMonoImageBuffer)\n");
			return;
		}

		doUpdateSize = PrepareImageBuffers(inWidth, inHeight, inIsColor, inIsBottomUp, inIs16Bits);
		CopyMemory(GetImageBufferPtr(), inImage, GetImageBufferSize());

		ReleaseMutex(mMutexHandle);
		UpdateFPS();

		if (doUpdateSize)
			UpdateWindowSize();
		else
			UpdateImage();
	}

	void	SetMonoImageBufferPtr(int inWidth, int inHeight, unsigned char *inImagePtr, bool inIsBottomUp = false)
	{
		SetImageBufferPtr(inWidth, inHeight, inImagePtr, false, inIsBottomUp);
	}

	void	AllocateMonoImageBuffer(int inWidth, int inHeight, bool inIsBottomUp = false)
	{
		AllocateImageBuffer(inWidth, inHeight, false, inIsBottomUp);
	}

	void	CopyIntoMonoImageBuffer(int inWidth, int inHeight, const unsigned char *inImage, bool inIsBottomUp = false)
	{
		CopyIntoImageBuffer(inWidth, inHeight, inImage, false, inIsBottomUp);
	}

	void	Set16BitsMonoImageBufferPtr(int inWidth, int inHeight, unsigned char *inImagePtr, bool inIsBottomUp = false)
	{
		SetImageBufferPtr(inWidth, inHeight, inImagePtr, false, inIsBottomUp, true);
	}

	void	Allocate16BitsMonoImageBuffer(int inWidth, int inHeight, bool inIsBottomUp = false)
	{
		AllocateImageBuffer(inWidth, inHeight, false, inIsBottomUp, true);
	}

	void	CopyInto16BitsMonoImageBuffer(int inWidth, int inHeight, const unsigned char *inImage, bool inIsBottomUp = false)
	{
		CopyIntoImageBuffer(inWidth, inHeight, inImage, false, inIsBottomUp, true);
	}

	void	SetColorImageBufferPtr(int inWidth, int inHeight, unsigned char *inImagePtr, bool inIsBottomUp = false)
	{
		SetImageBufferPtr(inWidth, inHeight, inImagePtr, true, inIsBottomUp);
	}

	void	AllocateColorImageBuffer(int inWidth, int inHeight, bool inIsBottomUp = false)
	{
		AllocateImageBuffer(inWidth, inHeight, true, inIsBottomUp);
	}
	void	CopyIntoColorImageBuffer(int inWidth, int inHeight, const unsigned char *inImage, bool inIsBottomUp = false)
	{
		CopyIntoImageBuffer(inWidth, inHeight, inImage, true, inIsBottomUp);
	}
	void	SetMapMode(unsigned short inMapBottomValue, unsigned short inMapTopValue,
						bool inIsMapReverse, unsigned short inDirectMapLimit)
	{
		if (mIs16BitsImage == false)
			return;

		mIsMapModeEnabled = true;
		mMapBottomValue = inMapBottomValue;
		mMapTopValue = inMapTopValue;
		mIsMapReverse = inIsMapReverse;
		mMapDirectMapLimit = inDirectMapLimit;

		UpdateImage();
	}
	void	DisableMapMode()
	{
		mIsMapModeEnabled = false;
	}
	void	UpdateImage()
	{
		if (IsWindowOpen() == false)
			return;

		UpdateFPS();
		UpdateMousePixelReadout();
		if (mIs16BitsImage)
			Update16BitsImageDisp();
		UpdateImageDisp();
	}
	void	DumpBitmapInfo()
	{
		printf("[Dump BitmapInfo : %s]\n", mWindowTitle);

		if (mBitmapInfo == NULL)
		{
			printf("BitmapInfo is NULL \n");
			return;
		}

		printf(" biSize: %d\n", mBitmapInfo->biSize);
		printf(" biWidth: %d\n", mBitmapInfo->biWidth);
		if (mBitmapInfo->biHeight < 0)
			printf(" biHeight: %d (Top-down DIB)\n", mBitmapInfo->biHeight);
		else
			printf(" biHeight: %d (Bottom-up DIB)\n", mBitmapInfo->biHeight);
		printf(" biPlanes: %d\n", mBitmapInfo->biPlanes);
		printf(" biBitCount: %d\n", mBitmapInfo->biBitCount);
		printf(" biCompression: %d\n", mBitmapInfo->biCompression);
		printf(" biSizeImage: %d\n", mBitmapInfo->biSizeImage);
		printf(" biXPelsPerMeter: %d\n", mBitmapInfo->biXPelsPerMeter);
		printf(" biYPelsPerMeter: %d\n", mBitmapInfo->biYPelsPerMeter);
		printf(" biClrUsed: %d\n", mBitmapInfo->biClrUsed);
		printf(" biClrImportant: %d\n", mBitmapInfo->biClrImportant);
		printf("\n");

		printf(" mBitmapInfoSize =  %d bytes\n", mBitmapInfoSize);
		printf(" RGBQUAD number  =  %d\n", (mBitmapInfoSize - mBitmapInfo->biSize) / (unsigned int )sizeof(RGBQUAD));
		printf(" mBitmapBitsSize =  %d bytes\n", mBitmapBitsSize);
	}

	unsigned char	*CreateDIB(bool inForceConvertToBottomUp = true)
	{
		if (mBitmapInfo == NULL)
			return NULL;

		unsigned char	*buf = new unsigned char[mBitmapInfoSize + mBitmapBitsSize];
		CopyMemory(buf, mBitmapInfo, mBitmapInfoSize);
		CopyMemory(&(buf[mBitmapInfoSize]), mBitmapBits, mBitmapBitsSize);

		if (inForceConvertToBottomUp == true)
			if (mBitmapInfo->biHeight < 0)
				FlipBitmap((BITMAPINFOHEADER *)buf, &(buf[mBitmapInfoSize]));

		return buf;
	}

	void	FlipImageBuffer()
	{
		FlipBitmap(mBitmapInfo, mBitmapBits);
	}

	bool	OpenBitmapFile(const char *inFileName)
	{
		FILE	*fp;
		BITMAPFILEHEADER	fileHeader;

		if (fopen_s(&fp, inFileName, "rb") != 0)
		{
			printf("Error: Can't open file %s (OpenBitmapFile)\n", inFileName);
			return false;
		}

		if (fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fp) != 1)
		{
			printf("Error: fread failed (OpenBitmapFile)\n");
			fclose(fp);
			return false;
		}


		DWORD	result = WaitForSingleObject(mMutexHandle, INFINITE);
		if (result != WAIT_OBJECT_0)
		{
			printf("Error: WaitForSingleObject failed (OpenBitmapFile)\n");
			fclose(fp);
			return false;
		}

		if (mBitmapInfo != NULL)
		{
			delete mBitmapInfo;
			if (mAllocatedImageBuffer != NULL)
				delete mAllocatedImageBuffer;
		}

		mBitmapInfoSize = fileHeader.bfOffBits - sizeof(BITMAPFILEHEADER);
		mBitmapInfo = (BITMAPINFOHEADER *)(new unsigned char[mBitmapInfoSize]);
		if (mBitmapInfo == NULL)
		{
			printf("Error: Can't allocate mBitmapInfo (OpenBitmapFile)\n");
			ReleaseMutex(mMutexHandle);
			fclose(fp);
			return false;
		}

		mBitmapBitsSize = fileHeader.bfSize - fileHeader.bfOffBits;
		mAllocatedImageBuffer = new unsigned char[mBitmapBitsSize];
		if (mAllocatedImageBuffer == NULL)
		{
			printf("Error: Can't allocate mBitmapBits (OpenBitmapFile)\n");
			delete mBitmapInfo;
			mBitmapInfo = NULL;
			ReleaseMutex(mMutexHandle);
			fclose(fp);
			return false;
		}
		mBitmapBits = mAllocatedImageBuffer;

		if (fread(mBitmapInfo, mBitmapInfoSize, 1, fp) != 1)
		{
			printf("Error: fread failed (OpenBitmapFile)\n");
			ReleaseMutex(mMutexHandle);
			fclose(fp);
			return false;
		}
		if (fread(mBitmapBits, mBitmapBitsSize, 1, fp) != 1)
		{
			printf("Error: fread failed (OpenBitmapFile)\n");
			ReleaseMutex(mMutexHandle);
			fclose(fp);
			return false;
		}
		fclose(fp);

		//if (mBitmapInfo->biHeight > 0)
		//	FlipImageBuffer();

		ReleaseMutex(mMutexHandle);

		UpdateWindowSize();
		UpdateImageDisp();

		printf("Bitmap File opened: %s\n", inFileName);

		return true;
	}

	bool	CopyToClipboard()
	{
		if (mBitmapInfo == NULL || mBitmapBits == NULL)
		{
			printf("Error: mBitmapInfo == NULL || mBitmapBits == NULL CopyToClipboard()\n");
			return false;
		}

		unsigned char	*buf = CreateDIB(true);

		OpenClipboard(mWindowH);
		EmptyClipboard();
		SetClipboardData(CF_DIB, buf);
		CloseClipboard();

		delete buf;

		return true;
	}

	bool	SaveAsBitmapFile()
	{
		const int	BUF_LEN = 512;
		OPENFILENAME	fileName;
		TCHAR	buf[BUF_LEN] = TEXT("Untitled.bmp");

		ZeroMemory(&fileName, sizeof(fileName));
		fileName.lStructSize = sizeof(fileName);
		fileName.hwndOwner = mWindowH;
		fileName.lpstrFilter = TEXT("Bitmap File (*.bmp)\0*.bmp\0");
		fileName.nFilterIndex = 1;
		fileName.lpstrFile = buf;
		fileName.nMaxFile = BUF_LEN;
		fileName.lpstrTitle = TEXT("Save Image As");
		//fileName.Flags = OFN_EXTENSIONDIFFERENT;
		fileName.lpstrDefExt = TEXT("bmp");

		if (GetSaveFileName(&fileName) == 0)
			return false;

		return SaveBitmapFile((const char *)fileName.lpstrFile, true);
	}

	bool	SaveBitmapFile(const char *inFileName = NULL, bool inIsUnicode = false)
	{
		if (mBitmapInfo == NULL || mBitmapBits == NULL)
		{
			printf("Error: mBitmapInfo == NULL || mBitmapBits == NULL SaveBitmap()\n");
			return false;
		}

//DumpBitmapInfo();

		unsigned char	*buf = CreateDIB(true);

		FILE	*fp;
		char	fileName[IMAGE_FILE_NAME_BUF_LEN];

		BITMAPFILEHEADER	fileHeader;
		fileHeader.bfType = 'MB';
		fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + mBitmapInfoSize + mBitmapBitsSize;
		fileHeader.bfReserved1 = 0;
		fileHeader.bfReserved2 = 0;
		fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + mBitmapInfoSize;

		if (inIsUnicode == false)
		{
			if (inFileName != NULL)
			{
				sprintf_s(fileName, IMAGE_FILE_NAME_BUF_LEN, "%s", inFileName);
			}
			else
			{
				sprintf_s(fileName, IMAGE_FILE_NAME_BUF_LEN, "%s%d.bmp", mWindowTitle, mFileNameIndex);
				mFileNameIndex++;
			}

			if (fopen_s(&fp, fileName, "wb") != 0)
			{
				printf("Error: Can't create file %s (SaveBitmapFile)\n", fileName);
				ReleaseMutex(mMutexHandle);
				return false;
			}
		}
		else
		{
			if (_wfopen_s(&fp, (wchar_t *)inFileName, L"wb") != 0)
			{
				printf("Error: Can't create file %s (SaveBitmapFile)\n", fileName);
				ReleaseMutex(mMutexHandle);
				return false;
			}
		}

		if (fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fp) != 1)
		{
			printf("Error: Can't write file (SaveBitmapFile)\n");
			ReleaseMutex(mMutexHandle);
			fclose(fp);
			return false;
		}
		if (fwrite(buf, mBitmapInfoSize + mBitmapBitsSize, 1, fp) != 1)
		{
			printf("Error: Can't write file (SaveBitmapFile)\n");
			ReleaseMutex(mMutexHandle);
			fclose(fp);
			return false;
		}
		fclose(fp);

//		if (inIsUnicode == false)
//			printf("Bitmap File saved: %s\n", fileName);
//		else
//			wprintf(TEXT("Bitmap File saved: %s\n"), inFileName);

		delete buf;

		return true;
	}

	int	GetImageWidth()
	{
		if (mBitmapInfo == NULL)
			return 0;
		return mBitmapInfo->biWidth;
	}

	int	GetImageHeight()
	{
		if (mBitmapInfo == NULL)
			return 0;
		return abs(mBitmapInfo->biHeight);
	}

	RECT	GetImageClientRect()
	{
		return mImageClientRect;
	}

	bool	IsBottomUpImage()
	{
		if (mBitmapInfo == NULL)
			return false;

		if (mBitmapInfo->biHeight > 0)
			return true;

		return false;
	}

	int	GetImageBitCount()
	{
		if (mBitmapInfo == NULL)
			return 0;
		return mBitmapInfo->biBitCount;
	}
	unsigned char	*GetImageBufferPtr()
	{
		if (mIs16BitsImage == false)
			return mBitmapBits;

		if (mAllocated16BitsImageBuffer == NULL)
			return (unsigned char *)mExternal16BitsImageBuffer;

		return (unsigned char *)mAllocated16BitsImageBuffer;
	}
	unsigned int	GetImageBufferSize()
	{
		if (mIs16BitsImage == false)
			return mBitmapBitsSize;

		return mBitmapBitsSize * sizeof(unsigned short);
	}
	double	GetDispScale()
	{
		return mImageDispScale;
	}

	void	SetDispScale(double inScale)
	{
		double	prevImageDispScale = mImageDispScale;

		if (inScale <= 1.0)
			inScale = 1.0;
		mImageDispScale = inScale;
		CheckImageDispOffset();
		UpdateStatusBar();

		UpdateImageDispDispRect();
		/*double	scale = mImageDispScale / 100.0;
		if (prevImageDispScale > mImageDispScale &&
			((int )(mImageSize.cx * scale) <= mImageDispRect.right - mImageDispRect.left ||
			 (int )(mImageSize.cy * scale) <= mImageDispRect.bottom - mImageDispRect.top))
			UpdateImageDisp(true);
		else
			UpdateImageDisp();*/

		UpdateMouseCursor();
	}

	double	CalcWindowSizeFitScale()
	{
		double	imageRatio = (double )mImageSize.cy / (double )mImageSize.cx;
		double width = mImageClientRect.right - mImageClientRect.left;
		double height = mImageClientRect.bottom - mImageClientRect.top;
		double	dispRatio = (double )mImageClientRect.bottom / width;
		double	scale;

		if (imageRatio > dispRatio)
			scale = height / (double )mImageSize.cy;
		else
			scale = width / (double )mImageSize.cx;

		return scale * 100.0;
	}

	void	FitDispScaleToWindowSize()
	{
		SetDispScale(CalcWindowSizeFitScale());
	}

	void	ZoomIn()
	{
		SetDispScale(CalcImageScale(IMAGE_ZOOM_STEP));
	}

	void	ZoomOut()
	{
		SetDispScale(CalcImageScale(-IMAGE_ZOOM_STEP));
	}

	bool	IsFullScreenMode()
	{
		return mIsFullScreenMode;
	}

	void	EnableFullScreenMode(bool inFitDispScaleToWindowSize = true, int inMonitorIndex = -1)
	{
		if (mIsFullScreenMode)
			return;

		RECT	monitorRect = {0, 0, 0, 0};
		int		index = 0;

		mMonitorNum = 0;
		EnumDisplayMonitors(NULL, NULL, (MONITORENUMPROC )MyMonitorEnumProc, (LPARAM )this);
		GetWindowRect(mWindowH, &mLastWindowRect);

		switch (inMonitorIndex)
		{
			case -1:
				POINT	pos;
				for (int i = 0; i < mMonitorNum; i++)
				{
					pos.x = mLastWindowRect.left;
					pos.y = mLastWindowRect.top;
					if (PtInRect(&mMonitorRect[i], pos) == TRUE)
					{
						index = i;
						break;
					}
				}
				monitorRect = mMonitorRect[index];
				break;
			case -2:
				for (int i = 0; i < mMonitorNum; i++)
				{
					if (mMonitorRect[i].left < monitorRect.left)
						monitorRect.left = mMonitorRect[i].left;
					if (mMonitorRect[i].top < monitorRect.top)
						monitorRect.top = mMonitorRect[i].top;
					if (mMonitorRect[i].right > monitorRect.right)
						monitorRect.right = mMonitorRect[i].right;
					if (mMonitorRect[i].bottom > monitorRect.bottom)
						monitorRect.bottom = mMonitorRect[i].bottom;
				}
				break;
			default:
				index = inMonitorIndex;
				if (index < 0 || index >= mMonitorNum)
					index = 0;
				monitorRect = mMonitorRect[index];
				break;
		}

		SetWindowLong(mWindowH, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		SetWindowPos(mWindowH, HWND_TOPMOST,
			monitorRect.left, monitorRect.top,
			monitorRect.right, monitorRect.bottom,
			SWP_SHOWWINDOW);
		DisableMenubar();
		DisableToolbar();
		DisableStatusbar();
		CheckMenuItem(mMenuH, IDM_FULL_SCREEN, MF_CHECKED);
		mIsFullScreenMode = true;
		UpdateImageDispDispRect();
		if (inFitDispScaleToWindowSize)
			FitDispScaleToWindowSize();
	}

	void	DisableFullScreenMode(bool inFitDispScaleToWindowSize = true)
	{
		if (!mIsFullScreenMode)
			return;

		SetWindowLong(mWindowH, GWL_STYLE,
			WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_VISIBLE | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
		SetWindowPos(mWindowH, HWND_NOTOPMOST,
			mLastWindowRect.left, mLastWindowRect.top,
			mLastWindowRect.right - mLastWindowRect.left,
			mLastWindowRect.bottom - mLastWindowRect.top,
			SWP_SHOWWINDOW);
		EnableMenubar();
		EnableToolbar();
		EnableStatusbar();
		CheckMenuItem(mMenuH, IDM_FULL_SCREEN, MF_UNCHECKED);
		mIsFullScreenMode = false;
		UpdateImageDispDispRect();
		if (inFitDispScaleToWindowSize)
			FitDispScaleToWindowSize();
	}

	bool	IsMenubarEnabled()
	{
		return mIsMenubarEnabled;
	}

	void	EnableMenubar()
	{
		if (mIsMenubarEnabled)
			return;

		::SetMenu(mWindowH, mMenuH);
		CheckMenuItem(mMenuH, IDM_MENUBAR, MF_CHECKED);
		mIsMenubarEnabled = true;
	}

	void	DisableMenubar()
	{
		if (!mIsMenubarEnabled)
			return;

		::SetMenu(mWindowH, NULL);
		CheckMenuItem(mMenuH, IDM_MENUBAR, MF_UNCHECKED);
		mIsMenubarEnabled = false;
	}

	bool	IsToolbarEnabled()
	{
		return mIsToolbarEnabled;
	}

	void	EnableToolbar()
	{
		if (mIsToolbarEnabled)
			return;

		::ShowWindow(mRebarH, SW_SHOW);
		CheckMenuItem(mMenuH, IDM_TOOLBAR, MF_CHECKED);
		mIsToolbarEnabled = true;
		UpdateImageDispDispRect();
		UpdateImageDisp(true);
	}

	void	DisableToolbar()
	{
		if (!mIsToolbarEnabled)
			return;

		::ShowWindow(mRebarH, SW_HIDE);
		CheckMenuItem(mMenuH, IDM_TOOLBAR, MF_UNCHECKED);
		mIsToolbarEnabled = false;
		UpdateImageDispDispRect();
		UpdateImageDisp(true);
	}

	bool	IsStatusbarEnabled()
	{
		return mIsStatusbarEnabled;
	}

	void	EnableStatusbar()
	{
		if (mIsStatusbarEnabled)
			return;

		::ShowWindow(mStatusbarH, SW_SHOW);
		CheckMenuItem(mMenuH, IDM_STATUSBAR, MF_CHECKED);
		mIsStatusbarEnabled = true;
	}

	void	DisableStatusbar()
	{
		if (!mIsStatusbarEnabled)
			return;

		::ShowWindow(mStatusbarH, SW_HIDE);
		CheckMenuItem(mMenuH, IDM_STATUSBAR, MF_UNCHECKED);
		mIsStatusbarEnabled = false;
	}

	bool	IsPlotEnabled()
	{
		return mIsPlotEnabled;
	}

	void	EnablePlot()
	{
		if (mIsPlotEnabled)
			return;

		CheckMenuItem(mMenuH, IDM_PLOT, MF_CHECKED);
		mIsPlotEnabled = true;
	}

	void	DisablePlot()
	{
		if (!mIsPlotEnabled)
			return;

		CheckMenuItem(mMenuH, IDM_PLOT, MF_UNCHECKED);
		mIsPlotEnabled = false;
	}


	static void	FlipBitmap(BITMAPINFOHEADER *inBitmapInfo, unsigned char *inBitmapBits)
	{
		if (inBitmapInfo == NULL)
			return;
		if (inBitmapBits == NULL)
			return;

		unsigned char	*srcPtr, *dstPtr, tmp;
		int	width = inBitmapInfo->biWidth;
		int	height = abs(inBitmapInfo->biHeight);

		if (inBitmapInfo->biBitCount == 8)
		{
			for (int y = 0; y < height / 2; y++)
			{
				srcPtr = &(inBitmapBits[width * y]);
				dstPtr = &(inBitmapBits[width * (height - y - 1)]);
				for (int x = 0; x < width; x++)
				{
					tmp = *dstPtr;
					*dstPtr = *srcPtr;
					*srcPtr = tmp;
					srcPtr++;
					dstPtr++;
				}
			}
		}
		else
		{
			for (int y = 0; y < height / 2; y++)
			{
				srcPtr = &(inBitmapBits[width * 3 * y]);
				dstPtr = &(inBitmapBits[width * 3 * (height - y - 1)]);
				for (int x = 0; x < width * 3; x++)
				{
					tmp = *dstPtr;
					*dstPtr = *srcPtr;
					*srcPtr = tmp;
					srcPtr++;
					dstPtr++;
				}
			}
		}

		inBitmapInfo->biHeight *= -1;
	}

	bool	IsScrollable()
	{
			int	imageWidth = (int )(mImageSize.cx * mImageDispScale / 100.0);
			int	imageHeight = (int )(mImageSize.cy * mImageDispScale / 100.0);

			if (imageWidth > mImageDispSize.cx)
				return true;
			if (imageHeight > mImageDispSize.cy)
				return true;

			return false;
	}

	void	SetDrawOverlayFunc(void (*inFunc)(ImageWindow *, HDC, void *), void *inFuncData)
	{
		mDrawOverlayFunc = inFunc;
		mOverlayFuncData = inFuncData;
	}


int					mImageClickNum;
int					mLastImageClickX;
int					mLastImageClickY;

//	HWND			GetWindowHandle() { return mWindowH; }
	const BITMAPINFOHEADER	*GetBitmapInfo() { return mBitmapInfo; }

protected:
	enum
	{
		IDM_SAVE	=	100,
		IDM_SAVE_AS,
		IDM_PROPERTY,
		IDM_CLOSE,
		IDM_UNDO,
		IDM_CUT,
		IDM_COPY,
		IDM_PASTE,
		IDM_MENUBAR,
		IDM_TOOLBAR,
		IDM_STATUSBAR,
		IDM_ZOOMPANE,
		IDM_FULL_SCREEN,
		IDM_FPS,
		IDM_PLOT,
		IDM_FREEZE,
		IDM_SCROLL_TOOL,
		IDM_ZOOM_TOOL,
		IDM_INFO_TOOL,
		IDM_ZOOM_IN,
		IDM_ZOOM_OUT,
		IDM_ACTUAL_SIZE,
		IDM_FIT_WINDOW,
		IDM_ADJUST_WINDOW_SIZE,
		IDM_CASCADE_WINDOW,
		IDM_TILE_WINDOW,
		IDM_ABOUT
	};

	virtual void	OnCommand(WPARAM inWParam, LPARAM inLParam)
	{
		switch (LOWORD(inWParam))
		{
			case IDM_SAVE:
				SaveBitmapFile();
				break;
			case IDM_SAVE_AS:
				SaveAsBitmapFile();
				break;
			case IDM_PROPERTY:
				DumpBitmapInfo();
				break;
			case IDM_CLOSE:
				PostQuitMessage(0);
				break;
			case IDM_COPY:
				CopyToClipboard();
				break;
			case IDM_MENUBAR:
				if (IsMenubarEnabled())
					DisableMenubar();
				else
					EnableMenubar();
				break;
			case IDM_TOOLBAR:
				if (IsToolbarEnabled())
					DisableToolbar();
				else
					EnableToolbar();
				break;
			case IDM_STATUSBAR:
				if (IsStatusbarEnabled())
					DisableStatusbar();
				else
					EnableStatusbar();
				break;
			case IDM_ZOOMPANE:
				break;
			case IDM_FULL_SCREEN:
				if (IsFullScreenMode())
					DisableFullScreenMode();
				else
					EnableFullScreenMode();
				break;
			case IDM_FPS:
				break;
			case IDM_PLOT:
				if (IsPlotEnabled())
					DisablePlot();
				else
					EnablePlot();
				break;
			case IDM_FREEZE:
				break;
			case IDM_SCROLL_TOOL:
				SetCursorMode(CURSOR_MODE_SCROLL_TOOL);
				break;
			case IDM_ZOOM_TOOL:
				SetCursorMode(CURSOR_MODE_ZOOM_TOOL);
				break;
			case IDM_INFO_TOOL:
				SetCursorMode(CURSOR_MODE_INFO_TOOL);
				break;
			case IDM_ZOOM_IN:
				ZoomIn();
				if (GetKeyState(VK_CONTROL) < 0)
					UpdateWindowSize(true);
				break;
			case IDM_ZOOM_OUT:
				ZoomOut();
				if (GetKeyState(VK_CONTROL) < 0)
					UpdateWindowSize(true);
				break;
			case IDM_ACTUAL_SIZE:
				if (GetKeyState(VK_CONTROL) < 0 || IsFullScreenMode() == true)
					SetDispScale(100);
				else
				{
					UpdateWindowSize();
					UpdateImageDisp();
				}
				break;
			case IDM_FIT_WINDOW:
				FitDispScaleToWindowSize();
				break;
			case IDM_ADJUST_WINDOW_SIZE:
				if (IsFullScreenMode() == false)
					UpdateWindowSize(true);
				break;
			case IDM_CASCADE_WINDOW:
				break;
			case IDM_TILE_WINDOW:
				break;
			case IDM_ABOUT:
				::MessageBox(mWindowH,
					TEXT("ImageWindow Ver.1.0.0\nDairoku Sekiguchi (2007/10/13)"),
					TEXT("About ImageWindow"), MB_OK);
				break;
		}
	}

	virtual void	OnKeyDown(WPARAM ininWParam, LPARAM inLParam)
	{
		switch (ininWParam)
		{
			case VK_SHIFT:
				UpdateMouseCursor();
				break;
		}
	}

	virtual void	OnKeyUp(WPARAM ininWParam, LPARAM inLParam)
	{
		switch (ininWParam)
		{
			case VK_SHIFT:
				UpdateMouseCursor();
				break;
		}
	}

	virtual void	OnChar(WPARAM ininWParam, LPARAM inLParam)
	{
		switch (ininWParam)
		{
			case VK_ESCAPE:
				if (IsFullScreenMode())
					DisableFullScreenMode();
				break;
			case 's':
			case 'S':
				SaveBitmapFile();
				break;
			case 'd':
			case 'D':
				DumpBitmapInfo();
				break;
			case 'f':
			case 'F':
				if (IsFullScreenMode())
					DisableFullScreenMode();
				else
					EnableFullScreenMode();
				break;
			case '+':
				ZoomIn();
				break;
			case '-':
				ZoomOut();
				break;
			case '=':
				SetDispScale(100);
				break;
		}
	}

	virtual void	OnLButtonDown(UINT inMessage, WPARAM inWParam, LPARAM inLParam)
	{
		POINT	localPos;
		int	x, y;
		double	scale;

		mMouseDownPos.x = (short )LOWORD(inLParam);
		mMouseDownPos.y = (short )HIWORD(inLParam);

		if ((inWParam & MK_CONTROL) == 0)
			mMouseDownMode = mCursorMode;
		else
			mMouseDownMode = CURSOR_MODE_SCROLL_TOOL;

		switch (mMouseDownMode)
		{
			case CURSOR_MODE_SCROLL_TOOL:
				mIsMouseDragging = true;
				mImageDispOffsetStart = mImageDispOffset;
				SetCapture(mWindowH);
				break;
			case CURSOR_MODE_ZOOM_TOOL:
				localPos = mMouseDownPos;
				localPos.x -= mImageDispRect.left;
				localPos.y -= mImageDispRect.top;
				scale = mImageDispScale / 100.0;
				x = (int )(localPos.x / scale);
				y = (int )(localPos.y / scale);
				x += mImageDispOffset.cx;
				y += mImageDispOffset.cy;

				if ((inWParam & MK_SHIFT) == 0)
					scale = CalcImageScale(IMAGE_ZOOM_STEP);
				else
					scale = CalcImageScale(-IMAGE_ZOOM_STEP);

				mImageDispOffset.cx = x - (int )(localPos.x / (scale / 100.0));
				mImageDispOffset.cy = y - (int )(localPos.y / (scale / 100.0));
				SetDispScale(scale);
				break;
			case CURSOR_MODE_INFO_TOOL:
				localPos = mMouseDownPos;
				localPos.x -= mImageDispRect.left;
				localPos.y -= mImageDispRect.top;
				scale = mImageDispScale / 100.0;
				x = (int )(localPos.x / scale);
				y = (int )(localPos.y / scale);
				x += mImageDispOffset.cx;
				y += mImageDispOffset.cy;

				if (x <0 || y < 0 ||
					x >= GetImageWidth() || y >= GetImageHeight())
					break;

				mImageClickNum++;
				mLastImageClickX = x;
				mLastImageClickY = y;

				int	v = mBitmapBits[mImageSize.cx * y + x];
				char	buf[256];

				if (mIs16BitsImage == true)
				{
					unsigned short	value = 0;

					if (mAllocated16BitsImageBuffer != NULL)
						value = mAllocated16BitsImageBuffer[x + y * mImageSize.cx];
					if (mExternal16BitsImageBuffer != NULL)
						value = mExternal16BitsImageBuffer[x + y * mImageSize.cx];

					printf("%d: X:%.4d Y:%.4d VALUE:%.3d %.5d\n", mImageClickNum, x, y, v, value);
				}
				else
				{
					_itoa(v, buf, 2);
					printf("%d: X:%.4d Y:%.4d VALUE:%.3d %.2X %s\n", mImageClickNum, x, y, v, v, buf);
				}
				break;
		}
	}

	virtual void	OnMButtonDown(UINT inMessage, WPARAM inWParam, LPARAM inLParam)
	{
		POINT	localPos;
		int	x, y;
		double	scale;

		localPos.x = (short )LOWORD(inLParam);
		localPos.y = (short )HIWORD(inLParam);

		localPos.x -= mImageDispRect.left;
		localPos.y -= mImageDispRect.top;
		scale = mImageDispScale / 100.0;
		x = (int )(localPos.x / scale);
		y = (int )(localPos.y / scale);
		x += mImageDispOffset.cx;
		y += mImageDispOffset.cy;

		if ((inWParam & (MK_SHIFT + MK_CONTROL)) == 0)
			scale = 100;
		else
		{
			if ((inWParam & MK_SHIFT) == 0)
				scale = 3000;
			else
				scale = CalcWindowSizeFitScale();
		}

		if (mImageDispScale == scale)
			scale = mImagePrevScale;

		mImagePrevScale = mImageDispScale;

		mImageDispOffset.cx = x - (int )(localPos.x / (scale / 100.0));
		mImageDispOffset.cy = y - (int )(localPos.y / (scale / 100.0));
		SetDispScale(scale);
	}

	virtual void	OnMouseMove(WPARAM inWParam, LPARAM inLParam)
	{
		if (mIsMouseDragging == false)
			return;

		POINT	currentPos;
		double	scale = mImageDispScale / 100.0;

		currentPos.x = (short )LOWORD(inLParam);
		currentPos.y = (short )HIWORD(inLParam);

		switch (mMouseDownMode)
		{
			case CURSOR_MODE_SCROLL_TOOL:
				mImageDispOffset = mImageDispOffsetStart;
				mImageDispOffset.cx -= (int )((currentPos.x - mMouseDownPos.x) / scale);
				mImageDispOffset.cy -= (int )((currentPos.y - mMouseDownPos.y) / scale);
				CheckImageDispOffset();
				UpdateImageDisp();
				break;
			case CURSOR_MODE_ZOOM_TOOL:
				break;
			case CURSOR_MODE_INFO_TOOL:
				break;
		}
	}

	virtual void	OnLButtonUp(WPARAM inWParam, LPARAM inLParam)
	{
		switch (mMouseDownMode)
		{
			case CURSOR_MODE_SCROLL_TOOL:
				mIsMouseDragging = false;
				ReleaseCapture();
				break;
			case CURSOR_MODE_ZOOM_TOOL:
				break;
			case CURSOR_MODE_INFO_TOOL:
				break;
		}
	}

	virtual void	OnRButtonDown(UINT inMessage, WPARAM inWParam, LPARAM inLParam)
	{
		POINT	pos;

		pos.x = (short )LOWORD(inLParam);
		pos.y = (short )HIWORD(inLParam);

		HMENU	menu = GetSubMenu(mPopupMenuH, 0);
		ClientToScreen(mWindowH, &pos);
		TrackPopupMenu(menu, TPM_LEFTALIGN, pos.x, pos.y, 0, mWindowH, NULL);
	}

	virtual void	OnMouseWheel(WPARAM inWParam, LPARAM inLParam)
	{
		POINT	mousePos;
		int	x, y, zDelta;
		double	scale;

		mousePos.x = (short )LOWORD(inLParam);
		mousePos.y = (short )HIWORD(inLParam);
		ScreenToClient(mWindowH, &mousePos);
		if (PtInRect(&mImageDispRect, mousePos) == 0)
			return;

		zDelta = GET_WHEEL_DELTA_WPARAM(inWParam);

		mousePos.x -= mImageDispRect.left;
		mousePos.y -= mImageDispRect.top;
		scale = mImageDispScale / 100.0;
		x = (int )(mousePos.x / scale);
		y = (int )(mousePos.y / scale);
		x += mImageDispOffset.cx;
		y += mImageDispOffset.cy;

		if ((inWParam & (MK_SHIFT + MK_CONTROL)) == 0)
			scale = CalcImageScale(zDelta / MOUSE_WHEEL_STEP);
		else
		{
			if ((inWParam & MK_SHIFT) == 0)
				scale = CalcImageScale(zDelta / MOUSE_WHEEL_STEP * 4);
			else
				scale = CalcImageScale(zDelta / MOUSE_WHEEL_STEP / 2);
		}

		mImageDispOffset.cx = x - (int )(mousePos.x / (scale / 100.0));
		mImageDispOffset.cy = y - (int )(mousePos.y / (scale / 100.0));
		SetDispScale(scale);
	}

	virtual void	OnSize(WPARAM inWParam, LPARAM inLParam)
	{
		UpdateImageDispDispRect();
		if (GetKeyState(VK_SHIFT) < 0)
		{
			FitDispScaleToWindowSize();
			return;
		}

		if (CheckImageDispOffset())
			UpdateImageDisp();

		UpdateStatusBar();
	}

	virtual bool	OnSetCursor(WPARAM inWParam, LPARAM inLParam)
	{
		return UpdateMouseCursor();
	}


	bool	UpdateMouseCursor()
	{
		if (UpdateMousePixelReadout() == false)
			return false;

		switch (mCursorMode)
		{
			case CURSOR_MODE_SCROLL_TOOL:
				if (IsScrollable())
					SetCursor(mScrollCursor);
				else
					SetCursor(mArrowCursor);
				break;
			case CURSOR_MODE_ZOOM_TOOL:
				if (GetKeyState(VK_SHIFT) < 0)
					SetCursor(mZoomMinusCursor);
				else
					SetCursor(mZoomPlusCursor);
				break;
			case CURSOR_MODE_INFO_TOOL:
				SetCursor(mInfoCursor);
				break;
		}

		return true;
	}

	virtual void	SetCursorMode(int inCursorMode)
	{
		mCursorMode = inCursorMode;

		if (mMenuH == NULL)
			return;

		switch (mCursorMode)
		{
			case CURSOR_MODE_SCROLL_TOOL:
				CheckMenuItem(mMenuH, IDM_SCROLL_TOOL, MF_CHECKED);
				CheckMenuItem(mMenuH, IDM_ZOOM_TOOL, MF_UNCHECKED);
				CheckMenuItem(mMenuH, IDM_INFO_TOOL, MF_UNCHECKED);
				break;
			case CURSOR_MODE_ZOOM_TOOL:
				CheckMenuItem(mMenuH, IDM_SCROLL_TOOL, MF_UNCHECKED);
				CheckMenuItem(mMenuH, IDM_ZOOM_TOOL, MF_CHECKED);
				CheckMenuItem(mMenuH, IDM_INFO_TOOL, MF_UNCHECKED);
				break;
			case CURSOR_MODE_INFO_TOOL:
				CheckMenuItem(mMenuH, IDM_SCROLL_TOOL, MF_UNCHECKED);
				CheckMenuItem(mMenuH, IDM_ZOOM_TOOL, MF_UNCHECKED);
				CheckMenuItem(mMenuH, IDM_INFO_TOOL, MF_CHECKED);
				break;
		}
	}

	void	UpdateImageDisp(bool inErase = false)
	{
		if (mWindowState != WINDOW_OPEN_STATE)
			return;
		::InvalidateRect(mWindowH, &mImageClientRect, inErase);
	}

	double	CalcImageScale(int inStep)
	{
		double	val, scale;

		val = log10(mImageDispScale) + inStep / 100.0;
		scale = pow(10, val);

		// 100% snap & 1% limit (just in case...)
		if (fabs(scale - 100.0) <= 1.0)
			scale = 100;
		if (scale <= 1.0)
			scale = 1.0;

		return scale;
	}

private:

	enum
	{
		WINDOW_INIT_STATE	=	1,
		WINDOW_OPEN_STATE,
		WINDOW_CLOSED_STATE,
		WINDOW_ERROR_STATE
	};

	int					mWindowState;
	int					mPosX, mPosY;
	char				*mWindowTitle;
	int					mFileNameIndex;

	HINSTANCE			mModuleH;
	HWND				mWindowH;
	HWND				mToolbarH;
	HWND				mStatusbarH;
	HWND				mRebarH;
	HMENU				mMenuH;
	HMENU				mPopupMenuH;

	HCURSOR				mArrowCursor;
	HCURSOR				mScrollCursor;
	HCURSOR				mZoomPlusCursor;
	HCURSOR				mZoomMinusCursor;
	HCURSOR				mInfoCursor;

	HICON				mAppIconH;

	HFONT				mPixValueFont;

	HANDLE				mMutexHandle;
	HANDLE				mThreadHandle;
	HANDLE				mEventHandle;
	BITMAPINFOHEADER	*mBitmapInfo;
	unsigned int		mBitmapInfoSize;
	unsigned char		*mBitmapBits;
	unsigned int		mBitmapBitsSize;

	bool				mIsHiDPI;

	bool				mIsThreadRunning;

	unsigned char		*mAllocatedImageBuffer;
	unsigned short		*mAllocated16BitsImageBuffer;
	unsigned short		*mExternal16BitsImageBuffer;

	SIZE				mImageSize;
	RECT				mImageDispRect;
	RECT				mImageClientRect;
	SIZE				mImageDispSize;
	SIZE				mImageDispOffset;
	double				mImageDispScale;
	double				mImagePrevScale;
	SIZE				mImageDispOffsetStart;

	bool				mIsColorImage;
	bool				mIs16BitsImage;

	int					mCursorMode;
	int					mMouseDownMode;
	bool				mIsMouseDragging;
	POINT				mMouseDownPos;

	bool				mIsFullScreenMode;
	RECT				mLastWindowRect;

	bool				mIsMenubarEnabled;
	bool				mIsToolbarEnabled;
	bool				mIsStatusbarEnabled;
	bool				mIsPlotEnabled;

	void				(*mDrawOverlayFunc)(ImageWindow *, HDC, void *);
	void				*mOverlayFuncData;

#define	FPS_DATA_NUM	25
	double				mFPSValue;
	double				mFPSData[FPS_DATA_NUM];
	int					mFPSDataCount;

	unsigned __int64	mFrequency;
	unsigned __int64	mPrevCount;

	int					mMonitorNum;
	RECT				mMonitorRect[MONITOR_ENUM_MAX];

	bool				mIsMapModeEnabled;
	unsigned short		mMapBottomValue;
	unsigned short		mMapTopValue;
	bool				mIsMapReverse;
	unsigned short		mMapDirectMapLimit;

	void	InitFPS()
	{
		::QueryPerformanceFrequency((LARGE_INTEGER *)&mFrequency);
		::QueryPerformanceCounter((LARGE_INTEGER *)&mPrevCount);

		mFPSValue = 0;
		mFPSDataCount = 0;
	}

	void	UpdateFPS()
	{
		if (mWindowState != WINDOW_OPEN_STATE || mBitmapInfo == NULL)
			return;

		unsigned __int64	currentCount;
		::QueryPerformanceCounter((LARGE_INTEGER *)&currentCount);

		mFPSValue = 1.0 * (double )mFrequency / (double )(currentCount - mPrevCount);
		mPrevCount = currentCount;

		if (mFPSDataCount < FPS_DATA_NUM)
			mFPSDataCount++;
		else
			::MoveMemory(mFPSData, &(mFPSData[1]), sizeof(double) * (FPS_DATA_NUM - 1));
		mFPSData[mFPSDataCount - 1] = mFPSValue;

		double	averageValue = 0;
		for (int i = 0; i < mFPSDataCount; i++)
			averageValue += mFPSData[i];
		averageValue /= mFPSDataCount;

#ifdef _UNICODE
		wchar_t	buf[IMAGE_STR_BUF_SIZE];

		swprintf_s(buf, IMAGE_STR_BUF_SIZE, TEXT("FPS: %.1f (avg.=%.1f)"), mFPSValue, averageValue);
		SendMessage(mStatusbarH, SB_SETTEXT, (WPARAM )3, (LPARAM )buf);
#else
		char	buf[IMAGE_STR_BUF_SIZE];

		sprintf_s(buf, IMAGE_STR_BUF_SIZE, TEXT("FPS: %.1f (avg.=%.1f)"), mFPSValue, averageValue);
		SendMessage(mStatusbarH, SB_SETTEXT, (WPARAM )3, (LPARAM )buf);
#endif
	}

	unsigned char	*GetPixelPointer(int inX, int inY)
	{
		if (inX < 0 || inX >= mImageSize.cx ||
			inY < 0 || inY >= mImageSize.cy)
			return NULL;

		if (mBitmapInfo->biBitCount == 8)
			return &(mBitmapBits[mImageSize.cx * inY + inX]);

		int	lineSize = mImageSize.cx * 3;
		if (lineSize % 4 != 0)
			lineSize = lineSize + (4 - (lineSize % 4));

		if (mBitmapInfo->biHeight < 0)	// Topdown-up DIB
			return &(mBitmapBits[lineSize * inY+ (inX * 3)]);

		return &(mBitmapBits[lineSize * (mImageSize.cy - inY - 1)+ (inX * 3)]);
	}

	bool	UpdateMousePixelReadout()
	{
		POINT	pos;
		int	x, y;
		bool	result;

		GetCursorPos(&pos);
		ScreenToClient(mWindowH, &pos);
		if (PtInRect(&mImageDispRect, pos) == 0)
			result = false;
		else
			result = true;

		pos.x -= mImageDispRect.left;
		pos.y -= mImageDispRect.top;
		x = (int )(pos.x / (mImageDispScale / 100.0));
		y = (int )(pos.y / (mImageDispScale / 100.0));
		x += mImageDispOffset.cx;
		y += mImageDispOffset.cy;

		unsigned char	*pixelPtr = GetPixelPointer(x, y);
		unsigned short	value = 0;

		if (mIs16BitsImage == true && mIsColorImage == false
			&& result == true && pixelPtr != NULL)
		{
			if (mAllocated16BitsImageBuffer != NULL)
				value = mAllocated16BitsImageBuffer[x + y * mImageSize.cx];
			if (mExternal16BitsImageBuffer != NULL)
				value = mExternal16BitsImageBuffer[x + y * mImageSize.cx];
		}

#ifdef _UNICODE
		wchar_t	buf[IMAGE_STR_BUF_SIZE];

		if (result == false || pixelPtr == NULL)
			swprintf_s(buf, IMAGE_STR_BUF_SIZE, TEXT(""));
		else
		{
			if (mIs16BitsImage == true && mIsColorImage == false)
			{
				swprintf_s(buf, IMAGE_STR_BUF_SIZE, TEXT("%05d (%d,%d)"), value, x, y);
			}
			else
			{
				if (mBitmapInfo->biBitCount == 8)
					swprintf_s(buf, IMAGE_STR_BUF_SIZE, TEXT("%03d (%d,%d)"), pixelPtr[0], x, y);
				else
					swprintf_s(buf, IMAGE_STR_BUF_SIZE, TEXT("%03d %03d %03d (%d,%d)"),
								(int )pixelPtr[2], (int )pixelPtr[1], (int )pixelPtr[0], x, y);
			}
		}

		SendMessage(mStatusbarH, SB_SETTEXT, (WPARAM )2, (LPARAM )buf);
#else
		char	buf[IMAGE_STR_BUF_SIZE];

		if (result == false || pixelPtr == NULL)
			sprintf_s(buf, IMAGE_STR_BUF_SIZE, TEXT(""));
		else
		{
			if (mIs16BitsImage == true && mIsColorImage == false)
			{
				sprintf_s(buf, IMAGE_STR_BUF_SIZE, TEXT("%05d (%d,%d)"), value, x, y);
			}
			else
			{
				if (mBitmapInfo->biBitCount == 8)
					sprintf_s(buf, IMAGE_STR_BUF_SIZE, TEXT("%03d (%d,%d)"), pixelPtr[0], x, y);
				else
					sprintf_s(buf, IMAGE_STR_BUF_SIZE, TEXT("%03d %03d %03d (%d,%d)"),
								(int )pixelPtr[2], (int )pixelPtr[1], (int )pixelPtr[0], x, y);
			}
		}
		SendMessage(mStatusbarH, SB_SETTEXT, (WPARAM )2, (LPARAM )buf);
#endif

		return result;
	}

	void	UpdateWindowSize(bool inKeepDispScale = false)
	{
		if (mWindowState != WINDOW_OPEN_STATE || mBitmapInfo == NULL)
			return;

		RECT	rebarRect, statusbarRect, rect;
		int		imageWidth, imageHeight;

		GetWindowRect(mRebarH, &rebarRect);
		int	rebarHeight = rebarRect.bottom - rebarRect.top;
		if (!IsToolbarEnabled())
			rebarHeight = 0;
		GetWindowRect(mStatusbarH, &statusbarRect);
		int	statusbarHeight = statusbarRect.bottom - statusbarRect.top;
		if (!IsStatusbarEnabled())
			statusbarHeight = 0;

		mImageSize.cx = mBitmapInfo->biWidth;
		mImageSize.cy = abs(mBitmapInfo->biHeight);

		if (inKeepDispScale == false)
		{
			mImageDispScale = 100.0;
			imageWidth = mImageSize.cx;
			imageHeight = mImageSize.cy;
		}
		else
		{
			imageWidth = (int )(mImageSize.cx * mImageDispScale / 100.0);
			imageHeight = (int )(mImageSize.cy * mImageDispScale / 100.0);
		}

		int	displayWidth = GetSystemMetrics(SM_CXSCREEN);
		int	displayHEIGHT = GetSystemMetrics(SM_CYSCREEN);

		if (imageWidth > displayWidth)
			imageWidth = displayWidth;
		if (imageHeight > displayHEIGHT)
			imageHeight = displayHEIGHT;

		rect.left = 0;
		rect.top = 0;
		rect.right = imageWidth;
		rect.bottom = imageHeight + rebarHeight + statusbarHeight;
		mImageClientRect = rect;
		mImageClientRect.top = rebarHeight;
		mImageClientRect.bottom -= statusbarHeight;
		mImageDispRect = mImageClientRect;
		mImageDispSize.cx = mImageDispRect.right - mImageDispRect.left;
		mImageDispSize.cy = mImageDispRect.bottom - mImageDispRect.top;
		mImageDispOffset.cx = 0;
		mImageDispOffset.cy = 0;
		AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, TRUE);	// if menu is enabled

		SetWindowPos(mWindowH, NULL, 0, 0,
			rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOOWNERZORDER);

		UpdateStatusBar();
	}

	void	UpdateImageDispDispRect()
	{
		if (mWindowState != WINDOW_OPEN_STATE || mBitmapInfo == NULL)
			return;

		RECT	rebarRect, statusbarRect, rect;

		GetWindowRect(mRebarH, &rebarRect);
		int	rebarHeight = rebarRect.bottom - rebarRect.top;
		if (!IsToolbarEnabled())
			rebarHeight = 0;
		GetWindowRect(mStatusbarH, &statusbarRect);
		int	statusbarHeight = statusbarRect.bottom - statusbarRect.top;
		if (!IsStatusbarEnabled())
			statusbarHeight = 0;

		GetClientRect(mWindowH, &rect);
		rect.top += rebarHeight;
		rect.bottom -= statusbarHeight;
		mImageClientRect = rect;
		bool	update = false;
		{
			int imageWidth = (int )(mImageSize.cx * mImageDispScale / 100.0);
			int imageHeight = (int )(mImageSize.cy * mImageDispScale / 100.0);
			if (imageWidth < rect.right - rect.left)
			{
				rect.left	+= (rect.right - rect.left - imageWidth) / 2;
				rect.right	+= rect.left + imageWidth;
				update = true;
			}
			if (imageHeight < rect.bottom - rect.top)
			{
				rect.top	+= (rect.bottom - rect.top - imageHeight) / 2;
				rect.right	+= rect.top + imageHeight;
				update = true;
			}
		}
		mImageDispRect = rect;
		mImageDispSize.cx = mImageDispRect.right - mImageDispRect.left;
		mImageDispSize.cy = mImageDispRect.bottom - mImageDispRect.top;
		if (update)
			UpdateImageDisp(true);
		else
			UpdateImageDisp();
	}


	void	UpdateStatusBar()
	{
		RECT	rect;
		GetClientRect(mWindowH, &rect);

		int	statusbarSize[] = {100, 300, rect.right - 200, rect.right};
		SendMessage(mStatusbarH, SB_SETPARTS, (WPARAM )4, (LPARAM )(LPINT)statusbarSize);

#ifdef _UNICODE
		wchar_t	buf[IMAGE_STR_BUF_SIZE];

		swprintf_s(buf, IMAGE_STR_BUF_SIZE, TEXT("Zoom: %.0f%%"), mImageDispScale);
		SendMessage(mStatusbarH, SB_SETTEXT, (WPARAM )0, (LPARAM )buf);

		if (mBitmapInfo != NULL)
		{
			swprintf_s(buf, IMAGE_STR_BUF_SIZE,
				TEXT("Image: %dx%dx%d"),
				mBitmapInfo->biWidth, abs(mBitmapInfo->biHeight), mBitmapInfo->biBitCount);
			SendMessage(mStatusbarH, SB_SETTEXT, (WPARAM )1, (LPARAM )buf);
		}

		swprintf_s(buf, IMAGE_STR_BUF_SIZE, TEXT("FPS: %.1f"), mFPSValue);
		SendMessage(mStatusbarH, SB_SETTEXT, (WPARAM )3, (LPARAM )buf);
#else
		char	buf[IMAGE_STR_BUF_SIZE];

		sprintf_s(buf, IMAGE_STR_BUF_SIZE, TEXT("Zoom: %.0f%%"), mImageDispScale);
		SendMessage(mStatusbarH, SB_SETTEXT, (WPARAM )0, (LPARAM )buf);

		if (mBitmapInfo != NULL)
		{
			sprintf_s(buf, IMAGE_STR_BUF_SIZE,
				TEXT("Image: %dx%dx%d"),
				mBitmapInfo->biWidth, abs(mBitmapInfo->biHeight), mBitmapInfo->biBitCount);
			SendMessage(mStatusbarH, SB_SETTEXT, (WPARAM )1, (LPARAM )buf);
		}

		sprintf_s(buf, IMAGE_STR_BUF_SIZE, TEXT("FPS: %.1f"), mFPSValue);
		SendMessage(mStatusbarH, SB_SETTEXT, (WPARAM )3, (LPARAM )buf);
#endif
	}

	bool	CheckImageDispOffset()
	{
		int	limit;
		double	scale = mImageDispScale / 100.0;
		bool	update = false;
		SIZE	prevOffset = mImageDispOffset;

		limit = (int )(mImageSize.cx - mImageDispSize.cx / scale);
		if (mImageDispOffset.cx > limit)
		{
			mImageDispOffset.cx = limit;
			update = true;
		}
		if (mImageDispOffset.cx < 0)
		{
			mImageDispOffset.cx = 0;
			update = true;
		}
		limit = (int )(mImageSize.cy - mImageDispSize.cy / scale);
		if (mImageDispOffset.cy > limit)
		{
			mImageDispOffset.cy = limit;
			update = true;
		}
		if (mImageDispOffset.cy < 0)
		{
			mImageDispOffset.cy = 0;
			update = true;
		}

		if (prevOffset.cx == mImageDispOffset.cx &&
			prevOffset.cy == mImageDispOffset.cy)
			update = false;

		return update;
	}

	void	DrawImage(HDC inHDC)
	{
		if (mImageDispScale == 100)
		{
			SetDIBitsToDevice(inHDC,
				mImageDispRect.left, mImageDispRect.top,
				mImageSize.cx, mImageSize.cy,
				mImageDispOffset.cx, -1 * mImageDispOffset.cy,
				0, mImageSize.cy,
				mBitmapBits, (BITMAPINFO *)mBitmapInfo, DIB_RGB_COLORS);
		}
		else
		{
			double	scale = (mImageDispScale / 100.0);
			SetStretchBltMode(inHDC, COLORONCOLOR);
			StretchDIBits(inHDC,
				mImageDispRect.left, mImageDispRect.top,
				(int )(mImageSize.cx * scale),
				(int )(mImageSize.cy * scale),
				mImageDispOffset.cx, -1 * mImageDispOffset.cy,
				mImageSize.cx, mImageSize.cy,
				mBitmapBits, (BITMAPINFO *)mBitmapInfo, DIB_RGB_COLORS, SRCCOPY);

			if (mImageDispScale >= 3000 && mBitmapInfo->biBitCount == 8)
			{
				int	numX = (int )ceil((mImageDispRect.right - mImageDispRect.left) / scale);
				int	numY = (int )ceil((mImageDispRect.bottom - mImageDispRect.top) / scale);
				int	x, y;

				HGDIOBJ	prevFont = SelectObject(inHDC, mPixValueFont);
				SetBkMode(inHDC, TRANSPARENT);
				for (y = 0; y < numY; y++)
				{
					unsigned char	*pixelPtr = GetPixelPointer(mImageDispOffset.cx, mImageDispOffset.cy + y);
					for (x = 0; x < numX; x++)
					{
						wchar_t	buf[IMAGE_STR_BUF_SIZE];
						unsigned char	pixelValue = pixelPtr[x];

						swprintf_s(buf, IMAGE_STR_BUF_SIZE, TEXT("%03d"), pixelValue);

						if (pixelValue > 0x80)
							SetTextColor(inHDC, RGB(0x00, 0x00, 0x00));
						else
							SetTextColor(inHDC, RGB(0xFF, 0xFF, 0xFF));
						TextOut(inHDC,
							(int )((double )x * scale + scale * 0.5 - 10) + mImageDispRect.left,
							(int )((double )y * scale + scale * 0.5 - 5) + mImageDispRect.top,
							buf, 3);
					}
				}
				SelectObject(inHDC, prevFont);
			}
		}

		if (mIsPlotEnabled)
		{
			RECT	rect = GetImageClientRect();
			int	width = GetImageWidth();
			int	height = GetImageHeight();
			double	v;
			int	x, y, prevY;

			HPEN	hPen = CreatePen(PS_SOLID, 1, RGB(0xFF, 0xFF, 0xFF));
			SelectObject(inHDC, hPen);

			if (mIs16BitsImage)
			{
				unsigned short	*imagePtr = (unsigned short *)GetImageBufferPtr();
				double	k = (rect.bottom - rect.top) / 65535.0;

				if (mImageClickNum == 0 || mLastImageClickY > height || mLastImageClickY < 0)
					imagePtr += (width * (height / 2));
				else
					imagePtr += (width * mLastImageClickY);
				x = rect.left;

				for (int i = 0; i < width; i++)
				{
					v = k * (*imagePtr);
					y = rect.bottom - (int )v;
					if (x != 0)
					{
						MoveToEx(inHDC, (x - 1), prevY, NULL);
						LineTo(inHDC, x, y);
					}
					prevY = y;
					imagePtr++;
					x++;
				}
			}
			else
			{
				unsigned char	*imagePtr = GetImageBufferPtr();
				double	k = (rect.bottom - rect.top) / 255.0;

				if (mImageClickNum == 0 || mLastImageClickY > height || mLastImageClickY < 0)
					imagePtr += (width * (height / 2));
				else
					imagePtr += (width * mLastImageClickY);
				x = rect.left;

				for (int i = 0; i < width; i++)
				{
					v = k * (*imagePtr);
					y = rect.bottom - (int )v;
					if (x != 0)
					{
						MoveToEx(inHDC, (x - 1), prevY, NULL);
						LineTo(inHDC, x, y);
					}
					prevY = y;
					imagePtr++;
					x++;
				}
			}

			DeleteObject(hPen);
		}

		if (mDrawOverlayFunc != NULL)
			mDrawOverlayFunc(this, inHDC, mOverlayFuncData);
	}

	static unsigned int _stdcall	ThreadFunc(void *arg)
	{
		ImageWindow	*imageDisp = (ImageWindow *)arg;
		LPCTSTR	windowName;

		//	Initialze important vairables
		imageDisp->mImageDispScale = 100;
		imageDisp->mImagePrevScale = 100;
		imageDisp->InitFPS();

		imageDisp->InitMenu();

#ifdef _UNICODE
		int wcharsize = MultiByteToWideChar(CP_ACP, 0, imageDisp->mWindowTitle, -1, NULL, 0);
		windowName = new WCHAR[wcharsize];
		MultiByteToWideChar(CP_ACP, 0, imageDisp->mWindowTitle, -1, (LPWSTR )windowName, wcharsize);
#else
		windowName = (LPCSTR )imageDisp->mWindowTitle;
#endif
		//	Create WinDisp window
		imageDisp->mWindowH = ::CreateWindow(
				IMAGE_WINDOW_CLASS_NAME,		//	window class name
				windowName,						//	window title
				WS_OVERLAPPEDWINDOW,			//	normal window style
				imageDisp->mPosX,				//	x
				imageDisp->mPosY,				//	y
				IMAGE_WINDOW_DEFAULT_SIZE,	//	width
				IMAGE_WINDOW_DEFAULT_SIZE,	//	height
				HWND_DESKTOP,					//	no parent window
				imageDisp->mMenuH,				//	no menus
				GetModuleHandle(NULL),			//	handle to this module
				NULL);							//	no lpParam

		if (imageDisp->mWindowH == NULL)
		{
			imageDisp->mWindowState = WINDOW_ERROR_STATE;
			return 1;
		}

#ifdef _UNICODE
		delete windowName;
#endif

#ifdef _WIN64
		::SetWindowLongPtr(imageDisp->mWindowH, GWLP_USERDATA, (LONG_PTR )imageDisp);
#else
		::SetWindowLongPtr(imageDisp->mWindowH, GWLP_USERDATA, PtrToLong(imageDisp));
#endif

		// Check DPI
		double fx = GetSystemMetrics(SM_CXSMICON) / 16.0f;
		double fy = GetSystemMetrics(SM_CYSMICON) / 16.0f;
		if (fx < 1) fx = 1;
		if (fy < 1) fy = 1;
		//printf("%f, %f\n", fx, fy);
		if (fx >= 1.5 || fy >= 1.5)
			imageDisp->mIsHiDPI = true;

		imageDisp->InitToolbar();
		imageDisp->InitCursor();

		imageDisp->mPixValueFont = CreateFont(
			10, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, ANSI_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			ANTIALIASED_QUALITY | PROOF_QUALITY,
			//PROOF_QUALITY,
			FIXED_PITCH | FF_MODERN,
			TEXT("Lucida Console"));
//			TEXT("Courier"));


		imageDisp->mWindowState = WINDOW_OPEN_STATE;

		imageDisp->UpdateWindowSize();
		::ShowWindow(imageDisp->mWindowH, SW_SHOW);
		SetEvent(imageDisp->mEventHandle);
		MSG	msg;
		while (::GetMessage(&msg, NULL, 0, 0) != 0)
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}

		::DestroyWindow(imageDisp->mWindowH);
		imageDisp->mWindowState = WINDOW_INIT_STATE;
		imageDisp->mWindowH = NULL;
		imageDisp->mIsThreadRunning = false;
		return 0;
	}

	static LRESULT CALLBACK	WindowFunc(HWND hwnd, UINT inMessage, WPARAM inWParam, LPARAM inLParam)
	{
		ImageWindow	*imageDisp;
		HDC	hdc;
		PAINTSTRUCT	paintstruct;
		DWORD	result;

#ifdef _WIN64
		imageDisp = (ImageWindow *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
#else
		imageDisp = (ImageWindow *)LongToPtr(GetWindowLongPtr(hwnd, GWLP_USERDATA));
#endif

		switch (inMessage)
		{
			case WM_PAINT:
				if (imageDisp == NULL)
					break;
				if (imageDisp->mBitmapInfo == NULL)
					break;

				result = WaitForSingleObject(imageDisp->mMutexHandle, INFINITE);
				if (result != WAIT_OBJECT_0)
					break;

				hdc = BeginPaint(hwnd, &paintstruct);
				imageDisp->DrawImage(hdc);
				EndPaint(hwnd, &paintstruct);
				ReleaseMutex(imageDisp->mMutexHandle);
				break;
			case WM_SIZE:
				SendMessage(imageDisp->mRebarH, inMessage, inWParam, inLParam);
				SendMessage(imageDisp->mStatusbarH, inMessage, inWParam, inLParam);
				imageDisp->OnSize(inWParam, inLParam);
				break;
			case WM_COMMAND:
				if (imageDisp == NULL)
					break;
				imageDisp->OnCommand(inWParam, inLParam);
				break;
			case WM_KEYDOWN:
				imageDisp->OnKeyDown(inWParam, inLParam);
				break;
			case WM_KEYUP:
				imageDisp->OnKeyUp(inWParam, inLParam);
				break;
			case WM_CHAR:
				if (imageDisp == NULL)
					break;
				imageDisp->OnChar(inWParam, inLParam);
				break;
			case WM_LBUTTONDBLCLK:
			case WM_LBUTTONDOWN:
				imageDisp->OnLButtonDown(inMessage, inWParam, inLParam);
				break;
			case WM_MBUTTONDBLCLK:
			case WM_MBUTTONDOWN:
				imageDisp->OnMButtonDown(inMessage, inWParam, inLParam);
				break;
			case WM_MOUSEMOVE:
				imageDisp->OnMouseMove(inWParam, inLParam);
				break;
			case WM_LBUTTONUP:
				imageDisp->OnLButtonUp(inWParam, inLParam);
				break;
			case WM_RBUTTONDOWN:
				imageDisp->OnRButtonDown(inMessage, inWParam, inLParam);
				break;
			case WM_SETCURSOR:
				if (imageDisp->OnSetCursor(inWParam, inLParam) == false)
					return DefWindowProc(hwnd, inMessage, inWParam, inLParam);
				break;
			case WM_MOUSEWHEEL:
				imageDisp->OnMouseWheel(inWParam, inLParam);
				break;
			case WM_DESTROY:
				PostQuitMessage(0);
				break;
			default:
				return DefWindowProc(hwnd, inMessage, inWParam, inLParam);
		}

		return 0;
	}

	static BOOL CALLBACK	MyMonitorEnumProc(HMONITOR hMon, HDC hdcMon, LPRECT inMonRect, LPARAM inLParam)
	{
		ImageWindow	*imageWindow = (ImageWindow *)inLParam;

		printf("Monitor(%d) %d, %d, %d, %d\n", imageWindow->mMonitorNum,
			inMonRect->left, inMonRect->top, inMonRect->right, inMonRect->bottom);
		imageWindow->mMonitorRect[imageWindow->mMonitorNum].left = inMonRect->left;
		imageWindow->mMonitorRect[imageWindow->mMonitorNum].top = inMonRect->top;
		imageWindow->mMonitorRect[imageWindow->mMonitorNum].right = inMonRect->right;
		imageWindow->mMonitorRect[imageWindow->mMonitorNum].bottom = inMonRect->bottom;
		if (imageWindow->mMonitorNum == MONITOR_ENUM_MAX - 1)
			return FALSE;
		imageWindow->mMonitorNum++;
		return TRUE;
	}

	int	RegisterWindowClass()
	{
		WNDCLASSEX	wcx;

		if (GetClassInfoEx(
				GetModuleHandle(NULL),
				IMAGE_WINDOW_CLASS_NAME,
				&wcx) != 0)
		{
			return 1;
		}

		InitIcon();

		ZeroMemory(&wcx,sizeof(WNDCLASSEX));
		wcx.cbSize = sizeof(WNDCLASSEX);

		wcx.hInstance = mModuleH;
		wcx.lpszClassName = IMAGE_WINDOW_CLASS_NAME;
		wcx.lpfnWndProc = WindowFunc;
		wcx.style = CS_DBLCLKS ; // Class styles

		wcx.cbClsExtra = 0;
		wcx.cbWndExtra = 0;

		wcx.hIcon = mAppIconH;
		wcx.hIconSm = mAppIconH;
		wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcx.lpszMenuName = NULL;

		wcx.hbrBackground = (HBRUSH)COLOR_WINDOW; // Background brush

		if (RegisterClassEx(&wcx) == 0)
			return 0;

		return 1;
	}
	bool	PrepareImageBuffers(int inWidth, int inHeight, bool inIsColor, bool inIsBottomUp, bool inIs16Bits)
	{
		bool	doUpdateSize = CreateBitmapInfo(inWidth, inHeight, inIsColor, inIsBottomUp, inIs16Bits);

		mExternal16BitsImageBuffer = NULL;

		if (inIs16Bits == false && mAllocated16BitsImageBuffer != NULL)
		{
			delete mAllocated16BitsImageBuffer;
			mAllocated16BitsImageBuffer = NULL;
		}

		if (mAllocatedImageBuffer != NULL && doUpdateSize != false)
		{
			delete mAllocatedImageBuffer;
			mAllocatedImageBuffer = NULL;
			if (mAllocated16BitsImageBuffer != NULL)
			{
				delete mAllocated16BitsImageBuffer;
				mAllocated16BitsImageBuffer = NULL;
			}
		}

		if (mAllocatedImageBuffer == NULL)
			CreateNewImageBuffer(true);

		if (inIs16Bits == true && mAllocated16BitsImageBuffer == NULL)
			CreateNewImageBuffer16Bits(true);

		return doUpdateSize;
	}
	void	Update16BitsImageDisp()
	{
		unsigned short	*srcImagePtr = mAllocated16BitsImageBuffer;
		unsigned char	*dstImagePtr = mAllocatedImageBuffer;
		double	mapK = 255.0 / (double )(mMapTopValue - mMapBottomValue);

		if (mIsMapModeEnabled == false)
		{
			for (int y = 0; y < mImageSize.cy; y++)
			{
				for (int x = 0; x < mImageSize.cx; x++)
				{
					unsigned short	value = (*srcImagePtr) >> 8;
					*dstImagePtr = (unsigned char)value;
					dstImagePtr++;
					srcImagePtr++;
				}
			}
		}
		else
		{
			for (int y = 0; y < mImageSize.cy; y++)
			{
				for (int x = 0; x < mImageSize.cx; x++)
				{
					unsigned short	value;

					if (*srcImagePtr <= mMapDirectMapLimit)
						value = *srcImagePtr;
					else
					{
						double	d;
						if (mIsMapReverse == false)
							d = mapK * (double )(*srcImagePtr - mMapBottomValue);
						else
							d = 255.0 - (mapK * (double )(*srcImagePtr - mMapBottomValue));

						if (d > 255)
							d = 255;
						if (d <= mMapDirectMapLimit)
							d = mMapDirectMapLimit + 1;
						value = (unsigned short)d;
					}

					*dstImagePtr = (unsigned char)value;
					dstImagePtr++;
					srcImagePtr++;
				}
			}
		}
	}
	bool	CreateBitmapInfo(int inWidth, int inHeight, bool inIsColor, bool inIsBottomUp, bool inIs16Bits)
	{
		mIsColorImage = inIsColor;
		mIs16BitsImage = inIs16Bits;

		if (inIsColor)
			return CreateColorBitmapInfo(inWidth, inHeight, inIsBottomUp);
		else
			return CreateMonoBitmapInfo(inWidth, inHeight, inIsBottomUp);
	}
	int		CreateNewImageBuffer(bool inDoZeroClear)
	{
		mAllocatedImageBuffer = new unsigned char[mBitmapBitsSize];
		if (mAllocatedImageBuffer == NULL)
		{
			printf("Error: Can't allocate mBitmapBits (CreateNewImageBuffer)\n");
			ReleaseMutex(mMutexHandle);
			return 0;
		}
		mBitmapBits = mAllocatedImageBuffer;
		if (inDoZeroClear == true)
			ZeroMemory(mAllocatedImageBuffer, mBitmapBitsSize);

		return 1;
	}
	int		CreateNewImageBuffer16Bits(bool inDoZeroClear)
	{
		mAllocated16BitsImageBuffer = new unsigned short[mBitmapBitsSize];
		if (mAllocated16BitsImageBuffer == NULL)
		{
			printf("Error: Can't allocate mAllocated16BitsImageBuffer (CreateNewImageBuffer16Bits)\n");
			ReleaseMutex(mMutexHandle);
			return 0;
		}
		if (inDoZeroClear == true)
			ZeroMemory(mAllocated16BitsImageBuffer, mBitmapBitsSize * sizeof(unsigned short));

		return 1;
	}


	bool	CreateMonoBitmapInfo(int inWidth, int inHeight, bool inIsBottomUp)
	{
		bool	doCreateBitmapInfo = false;

		if (inIsBottomUp == true)
			inHeight = abs(inHeight);
		else
			inHeight = -1 * abs(inHeight);

		if (mBitmapInfo != NULL)
		{
			if (mBitmapInfo->biBitCount		!= 8 ||
				mBitmapInfo->biWidth		!= inWidth ||
				mBitmapInfo->biHeight		!= inHeight)
			{
				doCreateBitmapInfo = true;
			}
		}
		else
		{
			doCreateBitmapInfo = true;
		}

		if (doCreateBitmapInfo)
		{
			if (mBitmapInfo != NULL)
				delete mBitmapInfo;

			mBitmapInfoSize = sizeof(ImageBitmapInfoMono8);
			mBitmapInfo = (BITMAPINFOHEADER *)(new unsigned char[mBitmapInfoSize]);
			if (mBitmapInfo == NULL)
			{
				printf("Error: Can't allocate mBitmapInfo (CreateMonoBitmapInfo)\n");
				return false;
			}
			ImageBitmapInfoMono8	*bitmapInfo = (ImageBitmapInfoMono8 *)mBitmapInfo;
			bitmapInfo->Header.biSize			= sizeof(BITMAPINFOHEADER);
			bitmapInfo->Header.biWidth			= inWidth;
			bitmapInfo->Header.biHeight			= -1 * abs(inHeight);
			bitmapInfo->Header.biPlanes			= 1;
			bitmapInfo->Header.biBitCount		= 8;
			bitmapInfo->Header.biCompression	= BI_RGB;
			bitmapInfo->Header.biSizeImage		= 0;
			bitmapInfo->Header.biXPelsPerMeter	= 100;
			bitmapInfo->Header.biYPelsPerMeter	= 100;
			bitmapInfo->Header.biClrUsed		= IMAGE_PALLET_SIZE_8BIT;
			bitmapInfo->Header.biClrImportant	= IMAGE_PALLET_SIZE_8BIT;

			for (int i = 0; i < IMAGE_PALLET_SIZE_8BIT; i++)
			{
				bitmapInfo->RGBQuad[i].rgbBlue		= i;
				bitmapInfo->RGBQuad[i].rgbGreen		= i;
				bitmapInfo->RGBQuad[i].rgbRed		= i;
				bitmapInfo->RGBQuad[i].rgbReserved	= 0;
			}

			mBitmapBitsSize = abs(inWidth * inHeight);
		}

		return doCreateBitmapInfo;
	}

	bool	CreateColorBitmapInfo(int inWidth, int inHeight, bool inIsBottomUp)
	{
		bool	doCreateBitmapInfo = false;

		if (inIsBottomUp == true)
			inHeight = abs(inHeight);
		else
			inHeight = -1 * abs(inHeight);

		if (mBitmapInfo != NULL)
		{
			if (mBitmapInfo->biBitCount		!= 24 ||
				mBitmapInfo->biWidth		!= inWidth ||
				abs(mBitmapInfo->biHeight)	!= inHeight)
			{
				doCreateBitmapInfo = true;
			}
		}
		else
		{
			doCreateBitmapInfo = true;
		}

		if (doCreateBitmapInfo)
		{
			if (mBitmapInfo != NULL)
				delete mBitmapInfo;

			mBitmapInfoSize = sizeof(BITMAPINFOHEADER);
			mBitmapInfo = (BITMAPINFOHEADER *)(new unsigned char[mBitmapInfoSize]);
			if (mBitmapInfo == NULL)
			{
				printf("Error: Can't allocate mBitmapInfo (CreateColorBitmapInfo)\n");
				return false;
			}
			mBitmapInfo->biSize				= sizeof(BITMAPINFOHEADER);
			mBitmapInfo->biWidth			= inWidth;
			mBitmapInfo->biHeight			= inHeight;
			mBitmapInfo->biPlanes			= 1;
			mBitmapInfo->biBitCount			= 24;
			mBitmapInfo->biCompression		= BI_RGB;
			mBitmapInfo->biSizeImage		= 0;
			mBitmapInfo->biXPelsPerMeter	= 100;
			mBitmapInfo->biYPelsPerMeter	= 100;
			mBitmapInfo->biClrUsed			= 0;
			mBitmapInfo->biClrImportant		= 0;

			mBitmapBitsSize = abs(inWidth * inHeight * 3);
		}

		return doCreateBitmapInfo;
	}

	void	InitMenu()
	{
		mMenuH = CreateMenu();
		mPopupMenuH = CreateMenu();
		HMENU	popupSubMenuH = CreateMenu();
		HMENU	fileMenuH = CreateMenu();
		HMENU	editMenuH = CreateMenu();
		HMENU	viewMenuH = CreateMenu();
		HMENU	zoomMenuH = CreateMenu();
		HMENU	windowMenuH = CreateMenu();
		HMENU	helpMenuH = CreateMenu();

		AppendMenu(mMenuH, MF_POPUP, (UINT_PTR )fileMenuH, TEXT("&File"));
		AppendMenu(mMenuH, MF_POPUP, (UINT_PTR )editMenuH, TEXT("&Edit"));
		AppendMenu(mMenuH, MF_POPUP, (UINT_PTR )viewMenuH, TEXT("&View"));
		AppendMenu(mMenuH, MF_POPUP, (UINT_PTR )zoomMenuH, TEXT("&Zoom"));
		AppendMenu(mMenuH, MF_POPUP, (UINT_PTR )windowMenuH, TEXT("&Window"));
		AppendMenu(mMenuH, MF_POPUP, (UINT_PTR )helpMenuH, TEXT("&Help"));

		AppendMenu(mPopupMenuH, MF_POPUP, (UINT_PTR )popupSubMenuH, TEXT("&RightClickMenu"));
		AppendMenu(popupSubMenuH, MF_POPUP, (UINT_PTR )fileMenuH, TEXT("&File"));
		AppendMenu(popupSubMenuH, MF_POPUP, (UINT_PTR )editMenuH, TEXT("&Edit"));
		AppendMenu(popupSubMenuH, MF_POPUP, (UINT_PTR )viewMenuH, TEXT("&View"));
		AppendMenu(popupSubMenuH, MF_POPUP, (UINT_PTR )zoomMenuH, TEXT("&Zoom"));
		AppendMenu(popupSubMenuH, MF_POPUP, (UINT_PTR )windowMenuH, TEXT("&Window"));
		AppendMenu(popupSubMenuH, MF_POPUP, (UINT_PTR )helpMenuH, TEXT("&Help"));

		AppendMenu(fileMenuH, MF_ENABLED, IDM_SAVE, TEXT("&Save Image"));
		AppendMenu(fileMenuH, MF_ENABLED, IDM_SAVE_AS, TEXT("Save Image &As..."));
		AppendMenu(fileMenuH, MF_SEPARATOR, 0, NULL);
		AppendMenu(fileMenuH, MF_ENABLED, IDM_PROPERTY, TEXT("&Property"));
		AppendMenu(fileMenuH, MF_SEPARATOR, 0, NULL);
		AppendMenu(fileMenuH, MF_ENABLED, IDM_CLOSE, TEXT("&Close"));

		AppendMenu(editMenuH, MF_GRAYED, IDM_UNDO, TEXT("&Undo"));
		AppendMenu(editMenuH, MF_SEPARATOR, 0, NULL);
		AppendMenu(editMenuH, MF_GRAYED, IDM_CUT, TEXT("Cu&t"));
		AppendMenu(editMenuH, MF_ENABLED, IDM_COPY, TEXT("&Copy"));
		AppendMenu(editMenuH, MF_GRAYED, IDM_PASTE, TEXT("&Paste"));

		AppendMenu(viewMenuH, MF_ENABLED, IDM_MENUBAR, TEXT("&Menubar"));
		AppendMenu(viewMenuH, MF_ENABLED, IDM_TOOLBAR, TEXT("&Toolbar"));
		AppendMenu(viewMenuH, MF_ENABLED, IDM_STATUSBAR, TEXT("&Status Bar"));
		AppendMenu(viewMenuH, MF_GRAYED, IDM_ZOOMPANE, TEXT("&Zoom &Pane"));
		AppendMenu(viewMenuH, MF_SEPARATOR, 0, NULL);
		AppendMenu(viewMenuH, MF_ENABLED, IDM_FULL_SCREEN, TEXT("&Full Screen"));
		AppendMenu(viewMenuH, MF_SEPARATOR, 0, NULL);
		AppendMenu(viewMenuH, MF_ENABLED, IDM_FPS, TEXT("FPS"));
		AppendMenu(viewMenuH, MF_ENABLED, IDM_PLOT, TEXT("Plot"));
		AppendMenu(viewMenuH, MF_SEPARATOR, 0, NULL);
		AppendMenu(viewMenuH, MF_GRAYED, IDM_FREEZE, TEXT("Freeze"));
		AppendMenu(viewMenuH, MF_SEPARATOR, 0, NULL);
		AppendMenu(viewMenuH, MF_ENABLED, IDM_SCROLL_TOOL, TEXT("Scroll Tool"));
		AppendMenu(viewMenuH, MF_ENABLED, IDM_ZOOM_TOOL, TEXT("Zoom Tool"));
		AppendMenu(viewMenuH, MF_ENABLED, IDM_INFO_TOOL, TEXT("Info Tool"));

		AppendMenu(zoomMenuH, MF_ENABLED, IDM_ZOOM_IN, TEXT("Zoom In"));
		AppendMenu(zoomMenuH, MF_ENABLED, IDM_ZOOM_OUT, TEXT("Zoom Out"));
		AppendMenu(zoomMenuH, MF_SEPARATOR, 0, NULL);
		AppendMenu(zoomMenuH, MF_ENABLED, IDM_ACTUAL_SIZE, TEXT("Actual Size"));
		AppendMenu(zoomMenuH, MF_ENABLED, IDM_FIT_WINDOW, TEXT("Fit to Window"));
		AppendMenu(zoomMenuH, MF_SEPARATOR, 0, NULL);
		AppendMenu(zoomMenuH, MF_ENABLED, IDM_ADJUST_WINDOW_SIZE, TEXT("Adjust Window Size"));

		AppendMenu(windowMenuH, MF_GRAYED, IDM_CASCADE_WINDOW, TEXT("&Cascade"));
		AppendMenu(windowMenuH, MF_GRAYED, IDM_TILE_WINDOW, TEXT("&Tile"));

		AppendMenu(helpMenuH, MF_ENABLED, IDM_ABOUT, TEXT("&About"));

		SetCursorMode(mCursorMode);

		CheckMenuItem(mMenuH, IDM_MENUBAR, MF_CHECKED);
		CheckMenuItem(mMenuH, IDM_TOOLBAR, MF_CHECKED);
		CheckMenuItem(mMenuH, IDM_STATUSBAR, MF_CHECKED);
	}

	void	InitToolbar()
	{
		typedef BOOL		(WINAPI *_InitCommonControlsEx)(LPINITCOMMONCONTROLSEX);
		typedef HIMAGELIST	(WINAPI *_ImageList_Create)(int cx, int cy, UINT flags, int cInitial, int cGrow);
		typedef int			(WINAPI *_ImageList_AddMasked)(HIMAGELIST himl, HBITMAP hbmImage, COLORREF crMask);

		static HINSTANCE				hComctl32 = NULL;
		static _InitCommonControlsEx	_iw_InitCommonControlsEx = NULL;
		static _ImageList_Create		_iw_ImageList_Create = NULL;
		static _ImageList_AddMasked	_iw_ImageList_AddMasked = NULL;

		//	to bypass "comctl32.lib" linking, do some hack...
		if (hComctl32 == NULL)
		{
			HINSTANCE	hComctl32 = LoadLibrary(TEXT("comctl32.dll"));
			if (hComctl32 == NULL)
			{
				printf("Can't load comctl32.dll. Panic\n");
				return;
			}

			_iw_InitCommonControlsEx = (_InitCommonControlsEx )GetProcAddress(hComctl32, "InitCommonControlsEx");
			_iw_ImageList_Create = (_ImageList_Create )GetProcAddress(hComctl32, "ImageList_Create");
			_iw_ImageList_AddMasked = (_ImageList_AddMasked )GetProcAddress(hComctl32, "ImageList_AddMasked");
			if (_iw_InitCommonControlsEx == NULL ||
				_iw_ImageList_Create == NULL ||
				_iw_ImageList_AddMasked == NULL)
			{
				printf("GetProcAddress failed. Panic\n");
				return;
			}

			// Ensure common control DLL is loaded
			INITCOMMONCONTROLSEX icx;
			icx.dwSize = sizeof(INITCOMMONCONTROLSEX);
			icx.dwICC = ICC_BAR_CLASSES | ICC_COOL_CLASSES; // Specify BAR classes
			(*_iw_InitCommonControlsEx)(&icx); // Load the common control DLL
		}

		mRebarH = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL,
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS
				| RBS_BANDBORDERS | WS_CLIPCHILDREN | CCS_NODIVIDER,
			0, 0, 0, 0, mWindowH, NULL, GetModuleHandle(NULL), NULL);

		REBARINFO	rInfo;
		ZeroMemory(&rInfo, sizeof(REBARINFO));
		rInfo.cbSize = sizeof(REBARINFO);
		SendMessage(mRebarH, RB_SETBARINFO, 0, (LPARAM )&rInfo);

#define	TOOLBAR_BUTTON_NUM	16
		TBBUTTON	tbb[TOOLBAR_BUTTON_NUM];
		ZeroMemory(&tbb, sizeof(tbb));
		for (int i = 0; i < TOOLBAR_BUTTON_NUM; i++)
		{
			tbb[i].fsState = TBSTATE_ENABLED;
			tbb[i].fsStyle = TBSTYLE_BUTTON | BTNS_AUTOSIZE;
      tbb[i].dwData = 0L;
			tbb[i].iString = 0;
			tbb[i].iBitmap = MAKELONG(0, 0);
		}

		tbb[0].idCommand	= IDM_SAVE;
		tbb[0].iBitmap		= MAKELONG(0, 0);
		tbb[1].idCommand	= IDM_COPY;
		tbb[1].iBitmap		= MAKELONG(1, 0);
		tbb[2].fsStyle		= TBSTYLE_SEP;
		tbb[3].idCommand	= IDM_SCROLL_TOOL;
		tbb[3].iBitmap		= MAKELONG(2, 0);
		tbb[4].idCommand	= IDM_ZOOM_TOOL;
		tbb[4].iBitmap		= MAKELONG(3, 0);
		tbb[5].idCommand	= IDM_INFO_TOOL;
		tbb[5].iBitmap		= MAKELONG(4, 0);
		tbb[6].fsStyle		= TBSTYLE_SEP;
		tbb[7].idCommand	= IDM_FULL_SCREEN;
		tbb[7].iBitmap		= MAKELONG(5, 0);
		tbb[8].fsStyle		= TBSTYLE_SEP;
		tbb[9].idCommand	= IDM_FREEZE;
		tbb[9].iBitmap		= MAKELONG(6, 0);
		tbb[10].fsStyle		= TBSTYLE_SEP;
		tbb[11].idCommand	= IDM_ZOOM_IN;
		tbb[11].iBitmap		= MAKELONG(7, 0);
		tbb[12].idCommand	= IDM_ZOOM_OUT;
		tbb[12].iBitmap		= MAKELONG(8, 0);
		tbb[13].idCommand	= IDM_ACTUAL_SIZE;
		tbb[13].iBitmap		= MAKELONG(9, 0);
		tbb[14].idCommand	= IDM_FIT_WINDOW;
		tbb[14].iBitmap		= MAKELONG(10, 0);
		tbb[15].idCommand	= IDM_ADJUST_WINDOW_SIZE;
		tbb[15].iBitmap		= MAKELONG(11, 0);

		mToolbarH = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
			WS_VISIBLE | WS_CHILD| CCS_NODIVIDER | CCS_NORESIZE | TBSTYLE_FLAT,
			0, 0, 0, 0, mRebarH, NULL, GetModuleHandle(NULL), NULL);

		SendMessage(mToolbarH, CCM_SETVERSION, (WPARAM )5, (LPARAM )0);
		SendMessage(mToolbarH, TB_BUTTONSTRUCTSIZE, (WPARAM )sizeof(TBBUTTON), 0);

		static const unsigned char imageHeader[] = {
      0x28, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xC3, 0x0E, 0x00, 0x00, 0xC3, 0x0E,
			0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x34,
			0x8A, 0x38, 0x00, 0x0D, 0x26, 0xA1, 0x00, 0xCA, 0xCF, 0xE6, 0x00, 0x1A, 0x7D, 0xC2, 0x00,
			0x7A, 0xB6, 0xDC, 0x00, 0x00, 0xCC, 0xFF, 0x00, 0xBF, 0xD8, 0xE9, 0x00, 0x9C, 0x53, 0x00,
			0x00, 0xD1, 0xB5, 0x96, 0x00, 0xEB, 0xE2, 0xD7, 0x00, 0x42, 0x42, 0x42, 0x00, 0x6F, 0x6F,
			0x6F, 0x00, 0x86, 0x86, 0x86, 0x00, 0xE6, 0xE4, 0xE5, 0x00, 0xF6, 0xF6, 0xF6, 0x00
		};

		static const unsigned char imageData[12][128] = {{	// Save Icon
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xF0, 0x0F, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0xF0, 0x0F, 0x88, 0x88, 0x88, 0x88, 0x88,
			0x88, 0xF0, 0x0F, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0xF0, 0x0F, 0x88, 0x88, 0x88, 0x88,
			0x88, 0x88, 0xF0, 0x0F, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0xF0, 0x0F, 0x88, 0x88, 0x88,
			0x88, 0x88, 0x88, 0xF0, 0x0F, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0xF0, 0x0F, 0x88, 0x8F,
			0xFF, 0xFF, 0xF8, 0x88, 0xF0, 0x0F, 0x88, 0x8F, 0xFF, 0xFF, 0xF8, 0x88, 0xF0, 0x0F, 0x88,
			0x8F, 0xFF, 0x88, 0xF8, 0x88, 0xF0, 0x0F, 0x88, 0x8F, 0xFF, 0x88, 0xF8, 0x8D, 0xF0, 0x0F,
			0x88, 0x8F, 0xFF, 0x88, 0xF8, 0xDF, 0xF0, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		}, {		// Copy Icon
			0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x0F, 0xBB, 0xBB, 0xBB, 0xBB, 0xF0, 0x00,
			0x00, 0x0F, 0xBF, 0xFF, 0xFF, 0xFB, 0xF0, 0x00, 0x00, 0x0F, 0xBF, 0xFF, 0xFF, 0xFB, 0xF0,
			0x00, 0x00, 0x0F, 0xBF, 0xFF, 0xFF, 0xFB, 0xFF, 0xFF, 0xF0, 0x0F, 0xBF, 0xFF, 0xFF, 0xFB,
			0xFB, 0xBB, 0xF0, 0x0F, 0xBF, 0xFF, 0xFF, 0xFB, 0xFF, 0xFB, 0xF0, 0x0F, 0xBF, 0xFF, 0xFF,
			0xFB, 0xFF, 0xFB, 0xF0, 0x0F, 0xBF, 0xFF, 0xFF, 0xFB, 0xFF, 0xFB, 0xF0, 0x0F, 0xBF, 0xFF,
			0xFF, 0xFB, 0xFF, 0xFB, 0xF0, 0x0F, 0xBB, 0xBB, 0xBB, 0xBB, 0xFF, 0xFB, 0xF0, 0x0F, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFB, 0xF0, 0x00, 0x00, 0x0F, 0xBF, 0xFF, 0xFF, 0xFB, 0xF0, 0x00,
			0x00, 0x0F, 0xBF, 0xFF, 0xFF, 0xFB, 0xF0, 0x00, 0x00, 0x0F, 0xBB, 0xBB, 0xBB, 0xBB, 0xF0,
			0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0
		}, {	//	Scroll Icon (Translate Icon)
			0x00, 0x00, 0x0F, 0xFD, 0xDF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xDB, 0xBD, 0xFF, 0x00,
			0x00, 0x00, 0x00, 0xFD, 0xBB, 0xBB, 0xDF, 0x00, 0x00, 0x00, 0x00, 0xFB, 0xDB, 0xBD, 0xBF,
			0x00, 0x00, 0x0F, 0xFF, 0xFD, 0xFB, 0xBF, 0xDF, 0xFF, 0xF0, 0xFF, 0xDB, 0xDF, 0xFB, 0xBF,
			0xFD, 0xBD, 0xFF, 0xFD, 0xBD, 0xFF, 0xFF, 0xFF, 0xFF, 0xDB, 0xDF, 0xDB, 0xBB, 0xBB, 0xFB,
			0xBF, 0xBB, 0xBB, 0xBD, 0xDB, 0xBB, 0xBB, 0xFB, 0xBF, 0xBB, 0xBB, 0xBD, 0xFD, 0xBD, 0xFF,
			0xFF, 0xFF, 0xFF, 0xDB, 0xDF, 0xFF, 0xDB, 0xDF, 0xFB, 0xBF, 0xFD, 0xBD, 0xFF, 0x0F, 0xFF,
			0xFD, 0xFB, 0xBF, 0xDF, 0xFF, 0xF0, 0x00, 0x00, 0xFB, 0xDB, 0xBD, 0xBF, 0x00, 0x00, 0x00,
			0x00, 0xFD, 0xBB, 0xBB, 0xDF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xDB, 0xBD, 0xFF, 0x00, 0x00,
			0x00, 0x00, 0x0F, 0xFD, 0xDF, 0xF0, 0x00, 0x00
		}, {	//	Zoom Icon
			0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x3C,
			0xEF, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF3, 0xBB, 0xCF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x3B,
			0xBB, 0x9F, 0x00, 0x0F, 0xFF, 0xFF, 0xF3, 0xBB, 0xB9, 0xFF, 0x00, 0xFE, 0xCB, 0xBB, 0xDB,
			0xBB, 0x9F, 0xF0, 0x0F, 0xEB, 0x9F, 0xFF, 0x9B, 0xB9, 0xFF, 0x00, 0x0F, 0xC9, 0xFF, 0xFF,
			0xF9, 0xCF, 0xF0, 0x00, 0x0F, 0xBF, 0xFF, 0xFF, 0xFF, 0xBF, 0xF0, 0x00, 0x0F, 0xBF, 0xFF,
			0xFF, 0xFF, 0xBE, 0xF0, 0x00, 0x0F, 0xBF, 0xFF, 0xFF, 0xFF, 0xBF, 0xF0, 0x00, 0x0F, 0xC9,
			0xFF, 0xFF, 0xF9, 0xCF, 0x00, 0x00, 0x0F, 0xEB, 0x9F, 0xFF, 0x9B, 0xEF, 0x00, 0x00, 0x00,
			0xFE, 0xCB, 0xBB, 0xC3, 0xF0, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		}, {	//	Cross Icon
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0xB0, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x0B, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0xB0, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0xB0,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0xB0, 0x00, 0x00, 0x00, 0x0B, 0xBB, 0xBB, 0xBB,
			0xBB, 0xBB, 0xBB, 0xB0, 0x0B, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xB0, 0x00, 0x00, 0x00,
			0x0B, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x0B, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0xB0, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x0B, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0xB0, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		}, {	//	Full Icon
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB,
			0xBF, 0xFB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0xFB, 0xFB, 0xBB, 0xBB, 0xBB, 0xFF,
			0xFF, 0xBF, 0xFB, 0xFB, 0xFF, 0xFF, 0xFB, 0xFF, 0xFF, 0xBF, 0xFB, 0xFB, 0xFF, 0xFF, 0xFB,
			0xFF, 0xFF, 0xBF, 0xFB, 0xFB, 0xFF, 0xFF, 0xFB, 0xFF, 0xFF, 0xBF, 0xFB, 0xFB, 0xFF, 0xFF,
			0xFB, 0xFF, 0xFF, 0xBF, 0xFB, 0xFB, 0xBB, 0xBB, 0xB9, 0xFF, 0xFF, 0xBF, 0xFB, 0xFB, 0xBB,
			0xBB, 0x9D, 0xDF, 0xDF, 0xBF, 0xFB, 0xFF, 0xFF, 0xFF, 0xFD, 0xBD, 0xBF, 0xBF, 0xFB, 0xFF,
			0xFF, 0xFF, 0xFF, 0xDB, 0xBF, 0xBF, 0xFB, 0xFF, 0xFF, 0xFF, 0xFD, 0xBB, 0xBF, 0xBF, 0xFB,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0xFB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
		}, {	//	Pause Icon
			0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x3C, 0xBB, 0xBB, 0xC3, 0xFF,
			0x00, 0x0F, 0xFD, 0xBB, 0xBB, 0xBB, 0xBB, 0xDF, 0xF0, 0x0F, 0xDB, 0xBB, 0xBB, 0xBB, 0xBB,
			0xBD, 0xF0, 0xF3, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0x3F, 0xFC, 0xBB, 0xBF, 0xFB, 0xBF,
			0xFB, 0xBB, 0xCF, 0xFB, 0xBB, 0xBF, 0xFB, 0xBF, 0xFB, 0xBB, 0xBF, 0xFB, 0xBB, 0xBF, 0xFB,
			0xBF, 0xFB, 0xBB, 0xBF, 0xFB, 0xBB, 0xBF, 0xFB, 0xBF, 0xFB, 0xBB, 0xBF, 0xFB, 0xBB, 0xBF,
			0xFB, 0xBF, 0xFB, 0xBB, 0xBF, 0xFC, 0xBB, 0xBF, 0xFB, 0xBF, 0xFB, 0xBB, 0xCF, 0xF3, 0xBB,
			0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0x3F, 0x0F, 0xDB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBD, 0xF0, 0x0F,
			0xFD, 0xBB, 0xBB, 0xBB, 0xBB, 0xDF, 0xF0, 0x00, 0xFF, 0x3C, 0xBB, 0xBB, 0xC3, 0xFF, 0x00,
			0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00
		}, {	// Zoom In Icon
			0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x3C,
			0xEF, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF3, 0xBB, 0xCF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x3B,
			0xBB, 0x9F, 0x00, 0x0F, 0xFF, 0xFF, 0xF3, 0xBB, 0xB9, 0xFF, 0x00, 0xFE, 0xCB, 0xBB, 0xDB,
			0xBB, 0x9F, 0xF0, 0x0F, 0xEB, 0x9F, 0xFF, 0x9B, 0xB9, 0xFF, 0x00, 0x0F, 0xC9, 0xFF, 0x8F,
			0xF9, 0xCF, 0xF0, 0x00, 0x0F, 0xBF, 0xFF, 0x8F, 0xFF, 0xBF, 0xF0, 0x00, 0x0F, 0xBF, 0x88,
			0x88, 0x8F, 0xBE, 0xF0, 0x00, 0x0F, 0xBF, 0xFF, 0x8F, 0xFF, 0xBF, 0xF0, 0x00, 0x0F, 0xC9,
			0xFF, 0x8F, 0xF9, 0xCF, 0x00, 0x00, 0x0F, 0xEB, 0x9F, 0xFF, 0x9B, 0xEF, 0x00, 0x00, 0x00,
			0xFE, 0xCB, 0xBB, 0xC3, 0xF0, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		}, {	//	Zoom Out Icon
			0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x3C,
			0xEF, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF3, 0xBB, 0xCF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x3B,
			0xBB, 0x9F, 0x00, 0x0F, 0xFF, 0xFF, 0xF3, 0xBB, 0xB9, 0xFF, 0x00, 0xFE, 0xCB, 0xBB, 0xDB,
			0xBB, 0x9F, 0xF0, 0x0F, 0xEB, 0x9F, 0xFF, 0x9B, 0xB9, 0xFF, 0x00, 0x0F, 0xC9, 0xFF, 0xFF,
			0xF9, 0xCF, 0xF0, 0x00, 0x0F, 0xBF, 0xFF, 0xFF, 0xFF, 0xBF, 0xF0, 0x00, 0x0F, 0xBF, 0x88,
			0x88, 0x8F, 0xBF, 0xF0, 0x00, 0x0F, 0xBF, 0xFF, 0xFF, 0xFF, 0xBF, 0xF0, 0x00, 0x0F, 0xC9,
			0xFF, 0xFF, 0xF9, 0xCF, 0x00, 0x00, 0x0F, 0xEB, 0x9F, 0xFF, 0x9B, 0xEF, 0x00, 0x00, 0x00,
			0xFE, 0xCB, 0xBB, 0xC3, 0xF0, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		}, {	//	Actual Size Icon
			0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x3C,
			0xEF, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF3, 0xBB, 0xCF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x3B,
			0xBB, 0x9F, 0x00, 0x0F, 0xFF, 0xFF, 0xF3, 0xBB, 0xB9, 0xFF, 0x00, 0xFE, 0xCB, 0xBB, 0xDB,
			0xBB, 0x9F, 0xF0, 0x0F, 0xEB, 0x9F, 0xFF, 0x9B, 0xB9, 0xFF, 0x00, 0x0F, 0xCB, 0xBB, 0xBB,
			0xBB, 0xBB, 0xBB, 0xBB, 0x0F, 0xBB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFB, 0x0F, 0xBB, 0xFB,
			0xFF, 0xBF, 0xFF, 0xBF, 0xFB, 0x0F, 0xBB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0x0F, 0xCB,
			0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0x0F, 0xEB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0x00,
			0xFB, 0xFB, 0xFF, 0xBF, 0xFF, 0xBF, 0xFB, 0x00, 0x0B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFB,
			0x00, 0x0B, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB
		}, {	//	Fit to Window Icon
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0x00, 0x00, 0xFB, 0xBB, 0xBB, 0xBB, 0xFD, 0xDF, 0x00, 0x00, 0xFB, 0xBB, 0xBB, 0xB9,
			0xDB, 0xDF, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD, 0xBD, 0xFF, 0x0F, 0xBB, 0xFF, 0xDB, 0xBB,
			0xCB, 0xD9, 0xBF, 0x0F, 0xBB, 0xFD, 0xCE, 0xFE, 0xCC, 0xFB, 0xBF, 0x0F, 0xBB, 0xFB, 0xEF,
			0xBF, 0xEB, 0xFB, 0xBF, 0x0F, 0xBB, 0xFB, 0xFB, 0xBB, 0xFB, 0xFB, 0xBF, 0x0F, 0xBB, 0xFB,
			0xEF, 0xBF, 0xEB, 0xFB, 0xBF, 0x0F, 0xBB, 0xFD, 0xCE, 0xFE, 0xCD, 0xFB, 0xBF, 0x0F, 0xBB,
			0xFF, 0xDB, 0xBB, 0xDF, 0xFB, 0xBF, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
			0x00, 0xFB, 0xBB, 0xBB, 0xBB, 0xF0, 0x00, 0x00, 0x00, 0xFB, 0xBB, 0xBB, 0xBB, 0xF0, 0x00,
			0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00
		}, {	//	Adjust Window Icon
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF,
			0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xDD, 0xF0, 0xFB, 0xBB, 0xBB, 0xBB, 0xBB, 0x9D,
			0xBD, 0xF0, 0xFB, 0xFF, 0xFF, 0xFF, 0xFF, 0xDB, 0xDB, 0xF0, 0xFB, 0xFF, 0xFD, 0xBB, 0xBC,
			0xBD, 0x9B, 0xF0, 0xFB, 0xFF, 0xDC, 0xEF, 0xEC, 0xCF, 0xFB, 0xF0, 0xFB, 0xFF, 0xBE, 0xFB,
			0xFE, 0xBF, 0xFB, 0xF0, 0xFB, 0xFF, 0xBF, 0xBB, 0xBF, 0xBF, 0xFB, 0xF0, 0xFB, 0xFF, 0xBE,
			0xFB, 0xFE, 0xBF, 0xFB, 0xF0, 0xFB, 0xFF, 0xDC, 0xEF, 0xEC, 0xDF, 0xFB, 0xF0, 0xFB, 0xFF,
			0xFD, 0xBB, 0xBD, 0xFF, 0xFB, 0xF0, 0xFB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFB, 0xF0, 0xFB,
			0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xF0, 0xFB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xF0,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0
		}};

		unsigned char header[sizeof(imageHeader)];
		unsigned char data[512];
		::CopyMemory(header, imageHeader, sizeof(imageHeader));
		BITMAPINFOHEADER	*bitmapHeader = (BITMAPINFOHEADER *)header;
		if (mIsHiDPI)
		{
			bitmapHeader->biHeight *= 2;
			bitmapHeader->biWidth  *= 2;
			bitmapHeader->biSizeImage *= 4;
		}
		HIMAGELIST	imageList = (*_iw_ImageList_Create)(
				bitmapHeader->biWidth,
				bitmapHeader->biHeight,
				ILC_COLOR24 | ILC_MASK, 1, 1);

		for (int i = 0; i < 12; i++)
		{
			const unsigned char *imagePtr = imageData[i];
			if (mIsHiDPI)
			{
				int h = bitmapHeader->biHeight / 2;
				int w = bitmapHeader->biWidth / 4;
				const unsigned char *srcPtr = imageData[i];
				unsigned char *dst0Ptr = data;
				unsigned char *dst1Ptr = dst0Ptr + (bitmapHeader->biWidth / 2);
				for (int y = 0; y < h; y++)
				{
					for (int x = 0; x < w; x++)
					{
						*dst0Ptr = (*srcPtr & 0xF0) + (*srcPtr >> 4);
						*dst1Ptr = *dst0Ptr;
						dst0Ptr++;
						dst1Ptr++;
						*dst0Ptr = (*srcPtr << 4) + (*srcPtr & 0x0F);
						*dst1Ptr = *dst0Ptr;
						dst0Ptr++;
						dst1Ptr++;
						srcPtr++;
					}
					dst0Ptr += (bitmapHeader->biWidth / 2);
					dst1Ptr += (bitmapHeader->biWidth / 2);
				}
				imagePtr = data;
			}
			HBITMAP bitmap = CreateDIBitmap(
				GetDC(mToolbarH),
				(BITMAPINFOHEADER *)bitmapHeader,
				CBM_INIT,
				imagePtr,
				(BITMAPINFO *)bitmapHeader,
				DIB_RGB_COLORS);
			(*_iw_ImageList_AddMasked)(imageList, bitmap, RGB(255, 0, 255));
			DeleteObject(bitmap);
		}

		SendMessage(mToolbarH, TB_SETIMAGELIST, (WPARAM )0, (LPARAM )imageList);
		SendMessage(mToolbarH, TB_ADDBUTTONS, (WPARAM )TOOLBAR_BUTTON_NUM, (LPARAM )tbb);

		REBARBANDINFO	rbInfo;
		ZeroMemory(&rbInfo, sizeof(REBARBANDINFO));
		rbInfo.cbSize = sizeof(REBARBANDINFO);
		rbInfo.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
		rbInfo.fStyle = RBBS_CHILDEDGE;
		rbInfo.cxMinChild = 0;
		if (mIsHiDPI)
		  rbInfo.cyMinChild = 38;
		else
		  rbInfo.cyMinChild = 25;
		rbInfo.cx = 120;
		rbInfo.lpText = 0;
		rbInfo.hwndChild = mToolbarH;
		SendMessage(mRebarH, RB_INSERTBAND, (WPARAM )-1, (LPARAM )&rbInfo);

		mStatusbarH = CreateWindowEx(0, STATUSCLASSNAME, NULL,
			WS_CHILD | WS_VISIBLE,
			0, 0, 0, 0, mWindowH, NULL, NULL, NULL);
		UpdateStatusBar();
  }

	void	InitCursor()
	{
		static const unsigned char andPlane[3][128] = {{	// Scroll Hand
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
		},{	//	Zoom Plus
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
		},{	//	Zoom Minus
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
		}};

		static const unsigned char xorPlane[3][128] = {{	//	Scroll
			0x01, 0x80, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x07, 0xE0, 0x00, 0x00, 0x05, 0xA0, 0x00,
			0x00, 0x01, 0x80, 0x00, 0x00, 0x31, 0x8C, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0xFD, 0xBF,
			0x00, 0x00, 0xFD, 0xBF, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0x31, 0x8C, 0x00, 0x00, 0x01,
			0x80, 0x00, 0x00, 0x05, 0xA0, 0x00, 0x00, 0x07, 0xE0, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00,
			0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		},{	//	Zoom Plus
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x80, 0x00, 0x00, 0x10, 0x40, 0x00,
			0x00, 0x22, 0x20, 0x00, 0x00, 0x22, 0x20, 0x00, 0x00, 0x2F, 0xA0, 0x00, 0x00, 0x22, 0x20,
			0x00, 0x00, 0x22, 0x20, 0x00, 0x00, 0x10, 0x60, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00,
			0x38, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		},{	//	Zoom Minus
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x80, 0x00, 0x00, 0x10, 0x40, 0x00,
			0x00, 0x20, 0x20, 0x00, 0x00, 0x20, 0x20, 0x00, 0x00, 0x2F, 0xA0, 0x00, 0x00, 0x20, 0x20,
			0x00, 0x00, 0x20, 0x20, 0x00, 0x00, 0x10, 0x60, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00,
			0x38, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		}};

		if (mIsHiDPI == false)
		{
			mScrollCursor		= CreateCursor(mModuleH, 0, 0, 32, 32, andPlane[0], xorPlane[0]);
			mZoomPlusCursor		= CreateCursor(mModuleH, 0, 0, 32, 32, andPlane[1], xorPlane[1]);
			mZoomMinusCursor	= CreateCursor(mModuleH, 0, 0, 32, 32, andPlane[2], xorPlane[2]);
		}
		else
		{
			// ToDo : Improve this workaround.
			// At least, mZoomPlusCursor and mZoomMinusCursor
			mScrollCursor		= LoadCursor(NULL, IDC_SIZEALL);
			mZoomPlusCursor		= LoadCursor(NULL, IDC_ARROW);
			mZoomMinusCursor	= LoadCursor(NULL, IDC_ARROW);
		}

		mArrowCursor	= LoadCursor(NULL, IDC_ARROW);
		mInfoCursor		= LoadCursor(NULL, IDC_CROSS);
	}

	void	InitIcon()
	{
		static const unsigned char	data[] = {
			0x28, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x9B, 0x52, 0x00, 0x00,
			0x80, 0x00, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0xC0, 0xC0, 0xC0,
			0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0x00,
			0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x44,
			0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
			0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
			0x44, 0x4F, 0xF4, 0x44, 0xFF, 0x44, 0x44, 0xFF, 0x44, 0x4F, 0xF4, 0x44, 0xFF, 0x44, 0x44,
			0xFF, 0x44, 0x4F, 0xF4, 0x44, 0xFF, 0xF4, 0x4F, 0xFF, 0x44, 0x4F, 0xF4, 0x4F, 0xF4, 0xF4,
			0x4F, 0x4F, 0xF4, 0x4F, 0xF4, 0x4F, 0xF4, 0x4F, 0xF4, 0x4F, 0xF4, 0x4F, 0xF4, 0x4F, 0xF4,
			0x4F, 0xF4, 0x4F, 0xF4, 0x4F, 0xF4, 0x4F, 0xF4, 0x4F, 0xF4, 0x4F, 0xF4, 0x44, 0x44, 0x44,
			0x44, 0x44, 0x44, 0x44, 0x44, 0x4F, 0xF4, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x4F, 0xF4,
			0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
			0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		};

		mAppIconH = CreateIconFromResourceEx((PBYTE )data, 296, TRUE, 0x00030000, 16, 16, 0);
	}
};
#endif	// #ifdef __IMAGE_WINDOW_H
