// =============================================================================
//  ImageWindow.hpp
//
//  MIT License
//
//  Copyright (c) 2007-2023 Dairoku Sekiguchi
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.
// =============================================================================
/*!
  \file     ImageWindow.hpp
  \author   Dairoku Sekiguchi
  \version  4.0 (Release.05)
  \date     2023/09/23
  \brief    Single file image displaying utility class
*/
#ifndef IMAGE_WINDOW_H
#define IMAGE_WINDOW_H

#ifdef _M_IX86
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif _M_X64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

// Includes --------------------------------------------------------------------
#include <windows.h>
#include <commctrl.h>
#include <process.h>
#include <string.h>
#include <math.h>
#include <string>
#include <vector>

// Macros ----------------------------------------------------------------------
#define IMAGE_WINDOW_PALLET_SIZE_8BIT         256
#define IMAGE_WINDOW_CLASS_NAME               TEXT("ImageWindow")
#define IMAGE_WINDOW_DEFAULT_SIZE             160
#define IMAGE_WINDOW_AUTO_POS                 -100000
#define IMAGE_WINDOW_DEFAULT_POS              20
#define IMAGE_WINDOW_POS_SPACE                20
#define IMAGE_WINDOW_FILE_NAME_BUF_LEN        256
#define IMAGE_WINDOW_STR_BUF_SIZE             256
#define IMAGE_WINDOW_DEFAULT_NAME             "Untitled"
#define IMAGE_WINDOW_MONITOR_ENUM_MAX         32
#define IMAGE_WINDOW_ZOOM_STEP                1
#define IMAGE_WINDOW_MOUSE_WHEEL_STEP         60
#define IMAGE_WINDOW_FPS_DATA_NUM             25
#define IMAGE_WINDOW_TOOLBAR_BUTTON_NUM       16
#define IMAGE_WINDOW_ABOUT_TITLE              TEXT("About ImageWindow")
#define IMAGE_WINDOW_ABOUT_STR                TEXT("mageWindow Ver.4.0.0\2023/09/23")
#define IMAGE_WINDOW_VALUE_FONT               TEXT("Consolas")
#define IMAGE_WINDOW_PROCESS_DPI_AWARE

// Typedefs --------------------------------------------------------------------
typedef struct
{
  BITMAPINFOHEADER    Header;
  RGBQUAD             RGBQuad[IMAGE_WINDOW_PALLET_SIZE_8BIT];
} ImageBitmapInfoMono8;

// -----------------------------------------------------------------------------
//  ImageWindow class
// -----------------------------------------------------------------------------
class ImageWindow
{
public:
  // Enums ---------------------------------------------------------------------
  enum ColorMap
  {
    COLORMAP_GRAYSCALE  = 0,
    COLORMAP_JET,
    COLORMAP_RAINBOW,
    COLORMAP_SEPECTRUM,
    COLORMAP_COOLWARM
  };
  enum CursorMode
  {
    CURSOR_MODE_SCROLL_TOOL = 0,
    CURSOR_MODE_ZOOM_TOOL,
    CURSOR_MODE_INFO_TOOL
  };

  // Constructors and Destructor -----------------------------------------------
  // ---------------------------------------------------------------------------
  // ImageWindow
  // ---------------------------------------------------------------------------
  ImageWindow(const char *inWindowName = NULL,
            int inPosX = IMAGE_WINDOW_AUTO_POS,
            int inPosY = IMAGE_WINDOW_DEFAULT_POS)
  {
    static int  sPrevPosX   = 0;
    static int  sPrevPosY   = 0;
    static int  sWindowNum  = 0;

    mBitmapInfo           = NULL;
    mBitmapInfoSize       = 0;
    mBitmapBits           = NULL;
    mBitmapBitsSize       = 0;
    mAllocatedImageBuffer = NULL;
    mFileNameIndex        = 0;

    mWindowH              = NULL;
    mToolbarH             = NULL;
    mStatusbarH           = NULL;
    mRebarH               = NULL;
    mMenuH                = NULL;
    mPopupMenuH           = NULL;
    mThreadHandle         = NULL;
    mMutexHandle          = NULL;
    mEventHandle          = NULL;
    mModuleH              = GetModuleHandle(NULL);
    mIsHiDPI              = false;

    mWindowTitle          = NULL;
    mIsMouseDragging      = false;
    mCursorMode           = CURSOR_MODE_SCROLL_TOOL;
    mIsFullScreenMode     = false;
    mIsMenubarEnabled     = true;
    mIsToolbarEnabled     = true;
    mIsStatusbarEnabled   = true;
    mFPSValue             = 0;
    mImageClickNum        = 0;

    ::ZeroMemory(&mImageSize, sizeof(mImageSize));

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
#ifdef IMAGE_WINDOW_PROCESS_DPI_AWARE
      SetProcessDPIAware();
#endif
    }

    if (RegisterWindowClass() == 0)
    {
      printf("Error: RegisterWindowClass() failed\n");
      return;
    }

    char  windowName[IMAGE_WINDOW_STR_BUF_SIZE];
    if (inWindowName == NULL)
    {
      sprintf_s(windowName, IMAGE_WINDOW_STR_BUF_SIZE, "%s%d", IMAGE_WINDOW_DEFAULT_NAME, sWindowNum);
      inWindowName = windowName;
    }
    size_t  bufSize = strlen(inWindowName) + 1;
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
      mWindowTitle = NULL;
      return;
    }
    mEventHandle = CreateEvent(NULL, false, false, NULL);
    if (mEventHandle == NULL)
    {
      printf("Error: Can't create Event object\n");
      CloseHandle(mMutexHandle);
      delete mWindowTitle;
      mWindowTitle = NULL;
      return;
    }
    sWindowNum++;
  }
  // ---------------------------------------------------------------------------
  // ~ImageWindow
  // ---------------------------------------------------------------------------
  virtual ~ImageWindow()
  {
    CloseWindow();

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
  // ---------------------------------------------------------------------------
  // Buffer related member functions -------------------------------------------
  // ---------------------------------------------------------------------------
  // AllocateBuffer
  // ---------------------------------------------------------------------------
  void  AllocateBuffer(int inWidth, int inHeight,
                       bool inIsColor, bool inIsBottomUp = false)
  {
    DWORD result;
    bool  doUpdateSize;

    result = WaitForSingleObject(mMutexHandle, INFINITE);
    if (result != WAIT_OBJECT_0)
    {
      printf("Error: WaitForSingleObject failed (AllocateBuffer)\n");
      return;
    }
    doUpdateSize = PrepareBuffer(inWidth, inHeight, inIsColor, inIsBottomUp);
    ReleaseMutex(mMutexHandle);

    if (doUpdateSize)
      UpdateWindowSize();
  }
  // ---------------------------------------------------------------------------
  // SetExternalBuffer
  // ---------------------------------------------------------------------------
  void  SetExternalBuffer(int inWidth, int inHeight,
                          unsigned char *inExternalBuffer,
                          bool inIsColor, bool inIsBottomUp = false)
  {
    DWORD result;
    bool  doUpdateSize;

    result = WaitForSingleObject(mMutexHandle, INFINITE);
    if (result != WAIT_OBJECT_0)
    {
      printf("Error: WaitForSingleObject failed (SetExternalBuffer)\n");
      return;
    }
    doUpdateSize = CreateBitmapInfo(inWidth, inHeight, inIsColor, inIsBottomUp);

    if (mAllocatedImageBuffer != NULL)
    {
      delete mAllocatedImageBuffer;
      mAllocatedImageBuffer = NULL;
    }

    mBitmapBits = inExternalBuffer;
    ReleaseMutex(mMutexHandle);

    UpdateFPS();
    if (doUpdateSize)
      UpdateWindowSize();
    else
      UpdateWindow();
  }
  // ---------------------------------------------------------------------------
  // CopyImage
  // ---------------------------------------------------------------------------
  void  CopyImage(int inWidth, int inHeight, const unsigned char *inImage,
                  bool inIsColor, bool inIsBottomUp = false)
  {
    DWORD result;
    bool  doUpdateSize;

    result = WaitForSingleObject(mMutexHandle, INFINITE);
    if (result != WAIT_OBJECT_0)
    {
      printf("Error: WaitForSingleObject failed (AllocateMonoImageBuffer)\n");
      return;
    }
    doUpdateSize = PrepareBuffer(inWidth, inHeight, inIsColor, inIsBottomUp);
    ::CopyMemory(GetBufferPtr(), inImage, GetBufferSize());

    ReleaseMutex(mMutexHandle);

    UpdateFPS();
    if (doUpdateSize)
      UpdateWindowSize();
    else
      UpdateWindow();
  }
  // ---------------------------------------------------------------------------
  // UpdateWindow
  // ---------------------------------------------------------------------------
  void  UpdateWindow()
  {
    if (IsWindowOpen() == false)
      return;

    UpdateFPS();
    UpdateMousePixelReadout();
    UpdateWindowDisp();
  }
  // ---------------------------------------------------------------------------
  // GetBufferPtr
  // ---------------------------------------------------------------------------
  unsigned char *GetBufferPtr()
  {
    return mBitmapBits;
  }
  // ---------------------------------------------------------------------------
  // GetBufferSize
  // ---------------------------------------------------------------------------
  size_t  GetBufferSize()
  {
    return mBitmapBitsSize;
  }
  // ---------------------------------------------------------------------------
  // GetPixelPtr
  // ---------------------------------------------------------------------------
  unsigned char *GetPixelPtr(int inX, int inY)
  {
    if (mBitmapBits == NULL)
      return NULL;
    if (inX < 0 || inX >= mImageSize.cx ||
      inY < 0 || inY >= mImageSize.cy)
      return NULL;

    if (mBitmapInfo->biBitCount == 8)
      return &(mBitmapBits[mImageSize.cx * inY + inX]);

    int lineSize = mImageSize.cx * 3;
    if (lineSize % 4 != 0)
      lineSize = lineSize + (4 - (lineSize % 4));

    if (mBitmapInfo->biHeight < 0)  // Topdown-up DIB
      return &(mBitmapBits[lineSize * inY+ (inX * 3)]);

    return &(mBitmapBits[lineSize * (mImageSize.cy - inY - 1)+ (inX * 3)]);
  }
  // ---------------------------------------------------------------------------
  // GetWidth
  // ---------------------------------------------------------------------------
  int GetWidth()
  {
    return mImageSize.cx;
  }
  // ---------------------------------------------------------------------------
  // GetHeight
  // ---------------------------------------------------------------------------
  int GetHeight()
  {
    return mImageSize.cy;
  }
  // ---------------------------------------------------------------------------
  // IsBottomUp
  // ---------------------------------------------------------------------------
  bool IsBottomUp()
  {
    if (mBitmapInfo == NULL)
      return false;
    if (mBitmapInfo->biHeight > 0)
      return true;
    return false;
  }
  // ---------------------------------------------------------------------------
  // IsColor
  // ---------------------------------------------------------------------------
  bool IsColor()
  {
    if (mBitmapInfo == NULL)
      return false;
    if (mBitmapInfo->biBitCount == 24)
      return true;
    return false;
  }
  // ---------------------------------------------------------------------------
  // ReadBitmapFile
  // ---------------------------------------------------------------------------
  bool  ReadBitmapFile(const char *inFileName)
  {
    FILE  *fp;
    BITMAPFILEHEADER  fileHeader;

    if (fopen_s(&fp, inFileName, "rb") != 0)
    {
      printf("Error: Can't open file %s (ReadBitmapFile)\n", inFileName);
      return false;
    }

    if (fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fp) != 1)
    {
      printf("Error: fread failed (ReadBitmapFile)\n");
      fclose(fp);
      return false;
    }

    DWORD result = WaitForSingleObject(mMutexHandle, INFINITE);
    if (result != WAIT_OBJECT_0)
    {
      printf("Error: WaitForSingleObject failed (ReadBitmapFile)\n");
      fclose(fp);
      return false;
    }

    if (mBitmapInfo != NULL)
    {
      delete mBitmapInfo;
      if (mAllocatedImageBuffer != NULL)
        delete mAllocatedImageBuffer;
    }

    mBitmapInfoSize = (size_t )fileHeader.bfOffBits - sizeof(BITMAPFILEHEADER);
    mBitmapInfo = (BITMAPINFOHEADER *)(new unsigned char[mBitmapInfoSize]);
    if (mBitmapInfo == NULL)
    {
      printf("Error: Can't allocate mBitmapInfo (ReadBitmapFile)\n");
      ReleaseMutex(mMutexHandle);
      fclose(fp);
      return false;
    }

    mBitmapBitsSize = (size_t )fileHeader.bfSize - (size_t )fileHeader.bfOffBits;
    mAllocatedImageBuffer = new unsigned char[mBitmapBitsSize];
    if (mAllocatedImageBuffer == NULL)
    {
      printf("Error: Can't allocate mBitmapBits (ReadBitmapFile)\n");
      delete mBitmapInfo;
      mBitmapInfo = NULL;
      ReleaseMutex(mMutexHandle);
      fclose(fp);
      return false;
    }
    mBitmapBits = mAllocatedImageBuffer;

    if (fread(mBitmapInfo, mBitmapInfoSize, 1, fp) != 1)
    {
      printf("Error: fread failed (ReadBitmapFile)\n");
      ReleaseMutex(mMutexHandle);
      fclose(fp);
      return false;
    }
    if (fread(mBitmapBits, mBitmapBitsSize, 1, fp) != 1)
    {
      printf("Error: fread failed (ReadBitmapFile)\n");
      ReleaseMutex(mMutexHandle);
      fclose(fp);
      return false;
    }
    fclose(fp);
    ReleaseMutex(mMutexHandle);

    UpdateWindowSize();
    UpdateWindowDisp();
    return true;
  }
  // ---------------------------------------------------------------------------
  // WriteBitmapFile
  // ---------------------------------------------------------------------------
  // inIsUnicode = true will be used only when
  //  - The VC project setting is unicode
  //  - Need to pass the unicode string from WIN32 API (wide string version)
  bool  WriteBitmapFile(const char *inFileName = NULL, bool inIsUnicode = false)
  {
    if (mBitmapInfo == NULL || mBitmapBits == NULL)
    {
      printf("Error: mBitmapInfo == NULL || mBitmapBits == NULL SaveBitmap()\n");
      return false;
    }

    unsigned char *buf = CreateDIB(true);
    FILE  *fp = NULL;
    BITMAPFILEHEADER  fileHeader;
    ::ZeroMemory(&fileHeader, sizeof(fileHeader));
    fileHeader.bfType = 'MB';
    fileHeader.bfSize = (DWORD )(sizeof(BITMAPFILEHEADER) + mBitmapInfoSize + mBitmapBitsSize);
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;
    fileHeader.bfOffBits = (DWORD )(sizeof(BITMAPFILEHEADER) + mBitmapInfoSize);

    if (inIsUnicode == false)
    {
      char  fileName[IMAGE_WINDOW_FILE_NAME_BUF_LEN];
      const char  *fileNamePtr;
      if (inFileName != NULL)
      {
        fileNamePtr = inFileName;
      }
      else
      {
        sprintf_s(fileName, IMAGE_WINDOW_FILE_NAME_BUF_LEN, "%s%d.bmp", mWindowTitle, mFileNameIndex);
        fileNamePtr = fileName;
        mFileNameIndex++;
        printf("Save %s\n", fileName);
      }
      if (fopen_s(&fp, fileNamePtr, "wb") != 0)
      {
        printf("Error: Can't create file %s (WriteBitmapFile)\n", fileNamePtr);
        ReleaseMutex(mMutexHandle);
        return false;
      }
    }
    else
    {
      if (inFileName == NULL)
      {
        printf("Error: Can't create file (WriteBitmapFile(inIsUnicode = true))\n");
        ReleaseMutex(mMutexHandle);
        return false;
      }
      if (_wfopen_s(&fp, (wchar_t *)inFileName, L"wb") != 0)
      {
        printf("Error: Can't create file (WriteBitmapFile(inIsUnicode = true))\n");
        ReleaseMutex(mMutexHandle);
        return false;
      }
    }
    if (fp == NULL)
    {
      printf("Error: fp == NUL (WriteBitmapFile)\n");
      ReleaseMutex(mMutexHandle);
      return false;
    }

    if (fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fp) != 1)
    {
      printf("Error: Can't write file (WriteBitmapFile)\n");
      ReleaseMutex(mMutexHandle);
      fclose(fp);
      return false;
    }
    if (fwrite(buf, mBitmapInfoSize + mBitmapBitsSize, 1, fp) != 1)
    {
      printf("Error: Can't write file (WriteBitmapFile)\n");
      ReleaseMutex(mMutexHandle);
      fclose(fp);
      return false;
    }
    fclose(fp);
    delete buf;

    return true;
  }
  // ---------------------------------------------------------------------------
  // CopyToClipboard
  // ---------------------------------------------------------------------------
  bool  CopyToClipboard()
  {
    if (mBitmapInfo == NULL || mBitmapBits == NULL)
    {
      printf("Error: mBitmapInfo == NULL || mBitmapBits == NULL CopyToClipboard()\n");
      return false;
    }

    unsigned char *buf = CreateDIB(true);
    OpenClipboard(mWindowH);
    EmptyClipboard();
    SetClipboardData(CF_DIB, buf);
    CloseClipboard();
    delete buf;

    return true;
  }
  // ---------------------------------------------------------------------------
  // DumpBitmapInfo
  // ---------------------------------------------------------------------------
  void  DumpBitmapInfo()
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
    printf(" mBitmapInfoSize =  %d bytes\n", (int )mBitmapInfoSize);
    printf(" RGBQUAD number  =  %d\n", (int )((mBitmapInfoSize - mBitmapInfo->biSize) / (unsigned int )sizeof(RGBQUAD)));
    printf(" mBitmapBitsSize =  %zu bytes\n", mBitmapBitsSize);
  }
  // ---------------------------------------------------------------------------
  // Label & Text related member functions -------------------------------------
  // ---------------------------------------------------------------------------
  // AddLabel
  // ---------------------------------------------------------------------------
  void  AddLabel(int inLeft, int inTop, int inRight, int inBottom,
                 const char *inLabelStr,
                 unsigned char inR = 0, unsigned char inG = 0, unsigned char inB = 0)
  {
    LabelItem item = {{inLeft, inTop, inRight, inBottom},
                      inLabelStr,
                      RGB(inR, inG, inB)};
    mLabelList.push_back(item);
  }
  // ---------------------------------------------------------------------------
  // ClearLabels
  // ---------------------------------------------------------------------------
  void  ClearLabels()
  {
    mLabelList.clear();
  }
  // ---------------------------------------------------------------------------
  // ApplyColorMapToLabels
  // ---------------------------------------------------------------------------
  void  ApplyColorMapToLabels(ColorMap inColorMap = COLORMAP_RAINBOW)
  {
    int num = (int )mLabelList.size();
    if (num == 0)
      return;
    const unsigned char *map = GetColorMap(inColorMap);
    int div = num;
    if (div < 9)
      div = 9;
    if (div > 257)
      div = 257;
    int step = 256 / (div-1);
    for (int i = 0; i < num; i++)
    {
      int idx;
      if (i < 256)
        idx = i * step;
      else
        idx = i % 256;
      if (idx > 255)
        idx = 255;
      idx = 255 - idx;
      mLabelList[i].color = RGB( map[idx * 3 + 0],
                                 map[idx * 3 + 1],
                                 map[idx * 3 + 2]);
    }
  }
  // ---------------------------------------------------------------------------
  // SetOverlayText
  // ---------------------------------------------------------------------------
  void  SetOverlayText(const char *inText)
  {
    if (inText == NULL)
    {
      mOverlayText.clear();
      mOverlayTextLineList.clear();
      return;
    }
    mOverlayText = inText;
    mOverlayTextLineList.clear();
    int i = 0;
    TextLine line = {0, 0};
    while (inText[i] != 0)
    {
      if (inText[i] != '\n')
      {
        line.len++;
      }
      else
      {
        mOverlayTextLineList.push_back(line);
        line.start = i + 1;
        line.len = 0;
      }
      i++;
    }
    if (line.len != 0)
      mOverlayTextLineList.push_back(line);
  }
  // ---------------------------------------------------------------------------
  // Window related member functions -------------------------------------------
  // ---------------------------------------------------------------------------
  // ShowWindow
  // ---------------------------------------------------------------------------
  bool  ShowWindow()
  {
    if (IsWindowOpen())
      return false;

    if (mThreadHandle != NULL)
    {
      ::CloseHandle(mThreadHandle);
    }
    mThreadHandle = (HANDLE )_beginthreadex(  //  NOTE: need to use _beginthreadex instead of _beginthread, CreateThread
                                NULL,
                                0,
                                ThreadFunc,
                                this,
                                0,
                                NULL);
    if (mThreadHandle == NULL)
    {
      printf("Error: Can't create process thread\n");
      return false;
    }
    WaitForSingleObject(mEventHandle, INFINITE);
    if (IsWindowOpen() == false)
    {
      printf("Error: Can't create window\n");
      return false;
    }
    return true;
  }
  // ---------------------------------------------------------------------------
  // CloseWindow
  // ---------------------------------------------------------------------------
  void  CloseWindow()
  {
    if (IsWindowOpen())
      PostMessage(mWindowH, WM_CLOSE, 0, 0);
    WaitForWindowClose();
  }
  // ---------------------------------------------------------------------------
  // IsWindowOpen
  // ---------------------------------------------------------------------------
  bool  IsWindowOpen()
  {
    if (WaitForWindowClose(0) == false)
      return true;
    return false;
  }
  // ---------------------------------------------------------------------------
  // WaitForWindowClose
  // ---------------------------------------------------------------------------
  bool  WaitForWindowClose(DWORD inTimeout = INFINITE)
  {
    if (mThreadHandle == NULL)
      return true;
    if (WaitForSingleObject(mThreadHandle, inTimeout) == WAIT_OBJECT_0)
      return true;
  return false;
  }
  // ---------------------------------------------------------------------------
  // SaveAsBitmapFile
  // ---------------------------------------------------------------------------
  bool  SaveAsBitmapFile()
  {
    const int BUF_LEN = 512;
    OPENFILENAME  fileName;
    TCHAR buf[BUF_LEN] = TEXT("Untitled.bmp");
    ZeroMemory(&fileName, sizeof(fileName));
    fileName.lStructSize = sizeof(fileName);
    fileName.hwndOwner = mWindowH;
    fileName.lpstrFilter = TEXT("Bitmap File (*.bmp)\0*.bmp\0");
    fileName.nFilterIndex = 1;
    fileName.lpstrFile = buf;
    fileName.nMaxFile = BUF_LEN;
    fileName.lpstrTitle = TEXT("Save Image As");
    fileName.lpstrDefExt = TEXT("bmp");
    if (GetSaveFileName(&fileName) == 0)
      return false;
#ifdef _UNICODE
    return WriteBitmapFile((const char *)fileName.lpstrFile, true);
#else
    return WriteBitmapFile((const char*)fileName.lpstrFile, false);
#endif
  }
  // ---------------------------------------------------------------------------
  // GetDispScale
  // ---------------------------------------------------------------------------
  double  GetDispScale()
  {
    return mImageDispScale;
  }
  // ---------------------------------------------------------------------------
  // SetDispScale
  // ---------------------------------------------------------------------------
  void  SetDispScale(double inScale)
  {
    if (inScale <= 0.01)
      inScale = 0.01;
    mImageDispScale = inScale;
    CheckImageDispOffset();
    UpdateStatusBar();

    UpdateDispRect();
    UpdateMouseCursor();
  }
  // ---------------------------------------------------------------------------
  // ZoomIn
  // ---------------------------------------------------------------------------
  void  ZoomIn()
  {
    SetDispScale(CalcImageScale(IMAGE_WINDOW_ZOOM_STEP));
  }
  // ---------------------------------------------------------------------------
  // ZoomOut
  // ---------------------------------------------------------------------------
  void  ZoomOut()
  {
    SetDispScale(CalcImageScale(-IMAGE_WINDOW_ZOOM_STEP));
  }
  // ---------------------------------------------------------------------------
  // FitDispScaleToWindowSize
  // ---------------------------------------------------------------------------
  void  FitDispScaleToWindowSize()
  {
    SetDispScale(CalcWindowSizeFitScale());
  }
  // ---------------------------------------------------------------------------
  // IsFullScreenMode
  // ---------------------------------------------------------------------------
  bool  IsFullScreenMode()
  {
    return mIsFullScreenMode;
  }
  // ---------------------------------------------------------------------------
  // EnableFullScreenMode
  // ---------------------------------------------------------------------------
  void  EnableFullScreenMode(bool inFitDispScaleToWindowSize = true, int inMonitorIndex = -1)
  {
    if (mIsFullScreenMode)
      return;

    RECT  monitorRect = {0, 0, 0, 0};
    int   index = 0;

    mMonitorNum = 0;
    EnumDisplayMonitors(NULL, NULL, (MONITORENUMPROC )MyMonitorEnumProc, (LPARAM )this);
    GetWindowRect(mWindowH, &mLastWindowRect);

    switch (inMonitorIndex)
    {
      case -1:
        POINT pos;
        ::ZeroMemory(&pos, sizeof(pos));
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
    UpdateDispRect();
    if (inFitDispScaleToWindowSize)
      FitDispScaleToWindowSize();
  }
  // ---------------------------------------------------------------------------
  // DisableFullScreenMode
  // ---------------------------------------------------------------------------
  void  DisableFullScreenMode(bool inFitDispScaleToWindowSize = true)
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
    UpdateDispRect();
    if (inFitDispScaleToWindowSize)
      FitDispScaleToWindowSize();
  }
  // ---------------------------------------------------------------------------
  // IsMenubarEnabled
  // ---------------------------------------------------------------------------
  bool  IsMenubarEnabled()
  {
    return mIsMenubarEnabled;
  }
  // ---------------------------------------------------------------------------
  // EnableMenubar
  // ---------------------------------------------------------------------------
  void  EnableMenubar()
  {
    if (mIsMenubarEnabled)
      return;

    ::SetMenu(mWindowH, mMenuH);
    CheckMenuItem(mMenuH, IDM_MENUBAR, MF_CHECKED);
    mIsMenubarEnabled = true;
  }
  // ---------------------------------------------------------------------------
  // DisableMenubar
  // ---------------------------------------------------------------------------
  void  DisableMenubar()
  {
    if (!mIsMenubarEnabled)
      return;

    ::SetMenu(mWindowH, NULL);
    CheckMenuItem(mMenuH, IDM_MENUBAR, MF_UNCHECKED);
    mIsMenubarEnabled = false;
  }
  // ---------------------------------------------------------------------------
  // IsToolbarEnabled
  // ---------------------------------------------------------------------------
  bool  IsToolbarEnabled()
  {
    return mIsToolbarEnabled;
  }
  // ---------------------------------------------------------------------------
  // EnableToolbar
  // ---------------------------------------------------------------------------
  void  EnableToolbar()
  {
    if (mIsToolbarEnabled)
      return;

    ::ShowWindow(mRebarH, SW_SHOW);
    CheckMenuItem(mMenuH, IDM_TOOLBAR, MF_CHECKED);
    mIsToolbarEnabled = true;
    UpdateDispRect();
    UpdateWindowDisp(true);
  }
  // ---------------------------------------------------------------------------
  // DisableToolbar
  // ---------------------------------------------------------------------------
  void  DisableToolbar()
  {
    if (!mIsToolbarEnabled)
      return;
    ::ShowWindow(mRebarH, SW_HIDE);
    CheckMenuItem(mMenuH, IDM_TOOLBAR, MF_UNCHECKED);
    mIsToolbarEnabled = false;
    UpdateDispRect();
    UpdateWindowDisp(true);
  }
  // ---------------------------------------------------------------------------
  // IsStatusbarEnabled
  // ---------------------------------------------------------------------------
  bool  IsStatusbarEnabled()
  {
    return mIsStatusbarEnabled;
  }
  // ---------------------------------------------------------------------------
  // EnableStatusbar
  // ---------------------------------------------------------------------------
  void  EnableStatusbar()
  {
    if (mIsStatusbarEnabled)
      return;

    ::ShowWindow(mStatusbarH, SW_SHOW);
    CheckMenuItem(mMenuH, IDM_STATUSBAR, MF_CHECKED);
    mIsStatusbarEnabled = true;
  }
  // ---------------------------------------------------------------------------
  // DisableStatusbar
  // ---------------------------------------------------------------------------
  void  DisableStatusbar()
  {
    if (!mIsStatusbarEnabled)
      return;

    ::ShowWindow(mStatusbarH, SW_HIDE);
    CheckMenuItem(mMenuH, IDM_STATUSBAR, MF_UNCHECKED);
    mIsStatusbarEnabled = false;
  }
  // ---------------------------------------------------------------------------
  // WaitForWindowCloseMulti
  // ---------------------------------------------------------------------------
  static bool WaitForWindowCloseMulti(ImageWindow *inWindowArray[], int inArrayLen,
                      bool inWaitAll = false, DWORD inTimeout = INFINITE)
  {
    std::vector< HANDLE > handles;

    for (int i = 0; i < inArrayLen; i++)
      if (inWindowArray[i]->mThreadHandle != NULL)
        handles.push_back(inWindowArray[i]->mThreadHandle);

    if (WaitForMultipleObjects((DWORD )handles.size(), &(handles[0]), inWaitAll, inTimeout) == WAIT_TIMEOUT)
      return false;

    return true;
  }
  // ---------------------------------------------------------------------------
  // SetColorMap
  // ---------------------------------------------------------------------------
  void  SetColorMap(ColorMap inColorMap)
  {
    if (mBitmapInfo == NULL)
      return;
    if (mBitmapInfo->biBitCount != 8)
      return;

    ImageBitmapInfoMono8  *bitmapInfo = (ImageBitmapInfoMono8 *)mBitmapInfo;
    const unsigned char *map = GetColorMap(inColorMap);
    for (int i = 0; i < 256; i++)
    {
      bitmapInfo->RGBQuad[i].rgbRed         = map[i*3+0];
      bitmapInfo->RGBQuad[i].rgbGreen       = map[i*3+1];
      bitmapInfo->RGBQuad[i].rgbBlue        = map[i*3+2];
      bitmapInfo->RGBQuad[i].rgbReserved    = 0;
    }
  }

private:
  enum
  {
    IDM_SAVE  = 100,
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

  typedef struct
  {
    RECT        rect;
    std::string label;
    COLORREF    color;
  } LabelItem;
  typedef struct
  {
    int start;
    int len;
  } TextLine;

  std::vector<LabelItem>  mLabelList;
  std::string mOverlayText;
  std::vector<TextLine>  mOverlayTextLineList;

  BITMAPINFOHEADER  *mBitmapInfo;
  size_t            mBitmapInfoSize;
  unsigned char     *mBitmapBits;
  size_t            mBitmapBitsSize;
  unsigned char     *mAllocatedImageBuffer;
  int               mFileNameIndex;

  HWND              mWindowH;
  HWND              mToolbarH;
  HWND              mStatusbarH;
  HWND              mRebarH;
  HMENU             mMenuH;
  HMENU             mPopupMenuH;
  HANDLE            mThreadHandle;
  HANDLE            mMutexHandle;
  HANDLE            mEventHandle;
  HINSTANCE         mModuleH;
  bool              mIsHiDPI;

  HCURSOR           mArrowCursor;
  HCURSOR           mScrollCursor;
  HCURSOR           mZoomPlusCursor;
  HCURSOR           mZoomMinusCursor;
  HCURSOR           mInfoCursor;
  HICON             mAppIconH;
  HFONT             mPixValueFont;
  HFONT             mLabelFont;

  char              *mWindowTitle;
  bool              mIsMouseDragging;
  int               mCursorMode;
  bool              mIsFullScreenMode;
  bool              mIsMenubarEnabled;
  bool              mIsToolbarEnabled;
  bool              mIsStatusbarEnabled;
  int               mImageClickNum;

  int               mPosX, mPosY;
  SIZE              mImageSize;
  RECT              mImageDispRect;
  RECT              mImageClientRect;
  SIZE              mImageDispSize;
  SIZE              mImageDispOffset;
  double            mImageDispScale;
  double            mImagePrevScale;
  SIZE              mImageDispOffsetStart;
  RECT              mLastWindowRect;
  int               mMouseDownMode;
  POINT             mMouseDownPos;

  double            mFPSValue;
  double            mFPSData[IMAGE_WINDOW_FPS_DATA_NUM];
  int               mFPSDataCount;
  unsigned __int64  mFrequency;
  unsigned __int64  mPrevCount;

  int               mMonitorNum;
  RECT              mMonitorRect[IMAGE_WINDOW_MONITOR_ENUM_MAX];

  // ---------------------------------------------------------------------------
  // Buffer related member functions -------------------------------------------
  // ---------------------------------------------------------------------------
  // PrepareBuffer
  // ---------------------------------------------------------------------------
  bool  PrepareBuffer(int inWidth, int inHeight,
                      bool inIsColor, bool inIsBottomUp)
  {
    bool  doUpdateSize = CreateBitmapInfo(inWidth, inHeight, inIsColor, inIsBottomUp);

    if (mAllocatedImageBuffer != NULL && doUpdateSize != false)
    {
      delete mAllocatedImageBuffer;
      mAllocatedImageBuffer = NULL;
    }

    if (mAllocatedImageBuffer == NULL)
    {
      mAllocatedImageBuffer = new unsigned char[mBitmapBitsSize];
      mBitmapBits = mAllocatedImageBuffer;
      if (mAllocatedImageBuffer == NULL)
      {
        printf("Error: Can't allocate mBitmapBits (PrepareBuffer)\n");
        return false;
      }
      ZeroMemory(mAllocatedImageBuffer, mBitmapBitsSize);
    }
    return doUpdateSize;
  }
  // ---------------------------------------------------------------------------
  // CreateBitmapInfo
  // ---------------------------------------------------------------------------
  bool  CreateBitmapInfo(int inWidth, int inHeight,
                         bool inIsColor, bool inIsBottomUp)
  {
    if (inIsColor)
      return CreateColorBitmapInfo(inWidth, inHeight, inIsBottomUp);
    else
      return CreateMonoBitmapInfo(inWidth, inHeight, inIsBottomUp);
  }
  // ---------------------------------------------------------------------------
  // CreateColorBitmapInfo
  // ---------------------------------------------------------------------------
  bool  CreateColorBitmapInfo(int inWidth, int inHeight, bool inIsBottomUp)
  {
    bool  doCreateBitmapInfo = false;

    if (inIsBottomUp == true)
      inHeight = abs(inHeight);
    else
      inHeight = -1 * abs(inHeight);

    if (mBitmapInfo == NULL)
    {
      doCreateBitmapInfo = true;
    }
    else
    {
      if (mBitmapInfo->biBitCount   != 24 ||
        mBitmapInfo->biWidth        != inWidth ||
        mBitmapInfo->biHeight       != inHeight)
      {
        doCreateBitmapInfo = true;
      }
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
      mBitmapInfo->biSize           = sizeof(BITMAPINFOHEADER);
      mBitmapInfo->biWidth          = inWidth;
      mBitmapInfo->biHeight         = inHeight;
      mBitmapInfo->biPlanes         = 1;
      mBitmapInfo->biBitCount       = 24;
      mBitmapInfo->biCompression    = BI_RGB;
      mBitmapInfo->biSizeImage      = 0;
      mBitmapInfo->biXPelsPerMeter  = 100;
      mBitmapInfo->biYPelsPerMeter  = 100;
      mBitmapInfo->biClrUsed        = 0;
      mBitmapInfo->biClrImportant   = 0;

      mBitmapBitsSize = (size_t)abs(inWidth * inHeight * 3);

      mImageSize.cx = mBitmapInfo->biWidth;
      mImageSize.cy = abs(mBitmapInfo->biHeight);
    }

    return doCreateBitmapInfo;
  }
  // ---------------------------------------------------------------------------
  // CreateMonoBitmapInfo
  // ---------------------------------------------------------------------------
  bool  CreateMonoBitmapInfo(int inWidth, int inHeight, bool inIsBottomUp)
  {
    bool  doCreateBitmapInfo = false;

    if (inIsBottomUp == true)
      inHeight = abs(inHeight);
    else
      inHeight = -1 * abs(inHeight);

    if (mBitmapInfo == NULL)
    {
      doCreateBitmapInfo = true;
    }
    else
    {
      if (mBitmapInfo->biBitCount   != 8 ||
        mBitmapInfo->biWidth        != inWidth ||
        mBitmapInfo->biHeight       != inHeight)
      {
        doCreateBitmapInfo = true;
      }
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
      ImageBitmapInfoMono8  *bitmapInfo = (ImageBitmapInfoMono8 *)mBitmapInfo;
      bitmapInfo->Header.biSize     = sizeof(BITMAPINFOHEADER);
      bitmapInfo->Header.biWidth      = inWidth;
      bitmapInfo->Header.biHeight     = inHeight;
      bitmapInfo->Header.biPlanes     = 1;
      bitmapInfo->Header.biBitCount   = 8;
      bitmapInfo->Header.biCompression  = BI_RGB;
      bitmapInfo->Header.biSizeImage    = 0;
      bitmapInfo->Header.biXPelsPerMeter  = 100;
      bitmapInfo->Header.biYPelsPerMeter  = 100;
      bitmapInfo->Header.biClrUsed    = IMAGE_WINDOW_PALLET_SIZE_8BIT;
      bitmapInfo->Header.biClrImportant = IMAGE_WINDOW_PALLET_SIZE_8BIT;

      for (int i = 0; i < IMAGE_WINDOW_PALLET_SIZE_8BIT; i++)
      {
        bitmapInfo->RGBQuad[i].rgbBlue      = i;
        bitmapInfo->RGBQuad[i].rgbGreen     = i;
        bitmapInfo->RGBQuad[i].rgbRed       = i;
        bitmapInfo->RGBQuad[i].rgbReserved  = 0;
      }

      mBitmapBitsSize = (size_t )abs(inWidth * inHeight);

      mImageSize.cx = mBitmapInfo->biWidth;
      mImageSize.cy = abs(mBitmapInfo->biHeight);
    }

    return doCreateBitmapInfo;
  }
  // ---------------------------------------------------------------------------
  // CreateDIB
  // ---------------------------------------------------------------------------
  unsigned char *CreateDIB(bool inForceConvertToBottomUp = true)
  {
    if (mBitmapInfo == NULL)
      return NULL;

    unsigned char *buf = new unsigned char[mBitmapInfoSize + mBitmapBitsSize];
    CopyMemory(buf, mBitmapInfo, mBitmapInfoSize);
    CopyMemory(&(buf[mBitmapInfoSize]), mBitmapBits, mBitmapBitsSize);

    if (inForceConvertToBottomUp == true)
      if (mBitmapInfo->biHeight < 0)
        FlipBitmap((BITMAPINFOHEADER *)buf, &(buf[mBitmapInfoSize]));

    return buf;
  }
  // ---------------------------------------------------------------------------
  // FlipBitmap (static)
  // ---------------------------------------------------------------------------
  static void FlipBitmap(BITMAPINFOHEADER *inBitmapInfo, unsigned char *inBitmapBits)
  {
    if (inBitmapInfo == NULL)
      return;
    if (inBitmapBits == NULL)
      return;

    unsigned char *srcPtr, *dstPtr, tmp;
    int width = inBitmapInfo->biWidth;
    int height = abs(inBitmapInfo->biHeight);

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
  // ---------------------------------------------------------------------------
  // Window related member functions -------------------------------------------
  // ---------------------------------------------------------------------------
  // ThreadFunc
  // ---------------------------------------------------------------------------
  static unsigned int _stdcall  ThreadFunc(void *arg)
  {
    ImageWindow *imageDisp = (ImageWindow *)arg;
    LPCTSTR windowName;

    //  Initialze important vairables
    imageDisp->mImageDispScale = 1.0;
    imageDisp->mImagePrevScale = 1.0;
    imageDisp->InitFPS();

    imageDisp->InitMenu();

#ifdef _UNICODE
    int wcharsize = MultiByteToWideChar(CP_ACP, 0, imageDisp->mWindowTitle, -1, NULL, 0);
    windowName = new WCHAR[wcharsize];
    MultiByteToWideChar(CP_ACP, 0, imageDisp->mWindowTitle, -1, (LPWSTR )windowName, wcharsize);
#else
    windowName = (LPCSTR )imageDisp->mWindowTitle;
#endif
    //  Create WinDisp window
    imageDisp->mWindowH = ::CreateWindow(
                              IMAGE_WINDOW_CLASS_NAME,    //  window class name
                              windowName,                 //  window title
                              WS_OVERLAPPEDWINDOW,        //  normal window style
                              imageDisp->mPosX,           //  x
                              imageDisp->mPosY,           //  y
                              IMAGE_WINDOW_DEFAULT_SIZE,  //  width
                              IMAGE_WINDOW_DEFAULT_SIZE,  //  height
                              HWND_DESKTOP,               //  no parent window
                              imageDisp->mMenuH,          //  no menus
                              GetModuleHandle(NULL),      //  handle to this module
                              NULL);                      //  no lpParam
    if (imageDisp->mWindowH == NULL)
    {
      SetEvent(imageDisp->mEventHandle);
      return 0;
    }

#ifdef _UNICODE
    delete[] windowName;
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
    if (fx >= 1.5 || fy >= 1.5)
      imageDisp->mIsHiDPI = true;

    imageDisp->InitToolbar();
    imageDisp->InitCursor();

    imageDisp->mPixValueFont = ::CreateFont(
      10, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, ANSI_CHARSET,
      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
      DEFAULT_QUALITY,
      FIXED_PITCH | FF_MODERN,
      IMAGE_WINDOW_VALUE_FONT);

    imageDisp->mLabelFont = ::CreateFont(
      20, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, ANSI_CHARSET,
      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
      DEFAULT_QUALITY,
      FIXED_PITCH | FF_MODERN,
      IMAGE_WINDOW_VALUE_FONT);

    imageDisp->UpdateWindowSize();
    ::ShowWindow(imageDisp->mWindowH, SW_SHOW);
    ::SetEvent(imageDisp->mEventHandle);
    MSG msg;
    while (::GetMessage(&msg, NULL, 0, 0) != 0)
    {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
    }
    //
    ::DestroyWindow(imageDisp->mWindowH);
    imageDisp->mWindowH = NULL;
    return 0;
  }
  // ---------------------------------------------------------------------------
  // WindowFunc
  // ---------------------------------------------------------------------------
  static LRESULT CALLBACK WindowFunc(HWND hwnd, UINT inMessage, WPARAM inWParam, LPARAM inLParam)
  {
    ImageWindow *imageDisp;
    HDC hdc;
    PAINTSTRUCT paintstruct;
    DWORD result;

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
  // ---------------------------------------------------------------------------
  // DrawImage
  // ---------------------------------------------------------------------------
  void  DrawImage(HDC inHDC)
  {
    if (mImageDispScale == 1.0)
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
      SetStretchBltMode(inHDC, COLORONCOLOR);
      StretchDIBits(inHDC,
        mImageDispRect.left, mImageDispRect.top,
        (int )(mImageSize.cx * mImageDispScale),
        (int )(mImageSize.cy * mImageDispScale),
        mImageDispOffset.cx, -1 * mImageDispOffset.cy,
        mImageSize.cx, mImageSize.cy,
        mBitmapBits, (BITMAPINFO *)mBitmapInfo, DIB_RGB_COLORS, SRCCOPY);

      if (mImageDispScale >= 30 && mBitmapInfo->biBitCount == 8)
      {
        int numX = (int )ceil((mImageDispRect.right - mImageDispRect.left) / mImageDispScale);
        int numY = (int )ceil((mImageDispRect.bottom - mImageDispRect.top) / mImageDispScale);
        int x, y;

        HGDIOBJ prevFont = ::SelectObject(inHDC, mPixValueFont);
        ::SetBkMode(inHDC, TRANSPARENT);
        for (y = 0; y < numY; y++)
        {
          unsigned char *pixelPtr = GetPixelPtr(mImageDispOffset.cx, mImageDispOffset.cy + y);
          for (x = 0; x < numX; x++)
          {
            unsigned char pixelValue = pixelPtr[x];
#ifdef _UNICODE
            wchar_t buf[IMAGE_WINDOW_STR_BUF_SIZE];
            swprintf_s(buf, IMAGE_WINDOW_STR_BUF_SIZE, TEXT("%03d"), pixelValue);
#else
            char buf[IMAGE_WINDOW_STR_BUF_SIZE];
            sprintf_s(buf, IMAGE_WINDOW_STR_BUF_SIZE, TEXT("%03d"), pixelValue);
#endif
            if (pixelValue > 0x80)
              ::SetTextColor(inHDC, RGB(0x00, 0x00, 0x00));
            else
              ::SetTextColor(inHDC, RGB(0xFF, 0xFF, 0xFF));
            ::TextOut(inHDC,
              (int )((double )x * mImageDispScale + mImageDispScale * 0.5 - 10) + mImageDispRect.left,
              (int )((double )y * mImageDispScale + mImageDispScale * 0.5 - 5) + mImageDispRect.top,
              buf, 3);
          }
        }
        ::SelectObject(inHDC, prevFont);
      }
    }

    if (mLabelList.size() != 0)
    {
      HGDIOBJ prevFont = ::SelectObject(inHDC, mLabelFont);
      for (size_t i = 0; i < mLabelList.size(); i++)
      {
        size_t  idx = mLabelList.size() - i - 1;
        RECT  rect = mLabelList[idx].rect;
        const char *labelStr = mLabelList[idx].label.c_str();
        int  labelStrLen = (int )strlen(labelStr);
        COLORREF color = mLabelList[idx].color;
        SIZE  size;
#ifdef _UNICODE
        wchar_t buf[IMAGE_WINDOW_STR_BUF_SIZE];
        MultiByteToWideChar(CP_ACP, 0, labelStr, -1, buf, IMAGE_WINDOW_STR_BUF_SIZE);
        ::GetTextExtentPoint32W(inHDC, buf, labelStrLen, &size);
#else
        const char *buf = labelStr;
        ::GetTextExtentPoint32A(inHDC, buf, labelStrLen, &size);
#endif
        double y =  0.299 * (double )((color >>  0) & 0xFF) +
                    0.587 * (double )((color >>  8) & 0xFF) +
                    0.114 * (double )((color >> 16) & 0xFF);
  
        ImageToDispRect(&rect);
        HBRUSH  hbr = ::CreateSolidBrush(color);
        ::FrameRect(inHDC, &rect, hbr);
    
        ::SetBkMode(inHDC, TRANSPARENT);
        rect.right = rect.left + size.cx + 8;
        if ((rect.top - mImageDispRect.top)>= size.cy)
        {
          rect.bottom = rect.top;
          rect.top -= size.cy;
        }
        else
        {
          rect.bottom = rect.top + size.cy;
        }
        ::FillRect(inHDC, &rect, hbr);
  
        if (y > 200.0)
          ::SetTextColor(inHDC, RGB(0x00, 0x00, 0x00));
        else
          ::SetTextColor(inHDC, RGB(0xFF, 0xFF, 0xFF));
        //::BeginPath(inHDC);
        ::TextOut(inHDC,
                  rect.left+4, rect.top,
                  buf, labelStrLen);
        //::EndPath(inHDC);
        //::StrokeAndFillPath(inHDC);
        ::DeleteObject(hbr);
      }
      ::SelectObject(inHDC, prevFont);
    }

    int width = 0, height = 0;
    if (mOverlayText.size() != 0)
    {
      HGDIOBJ prevFont = ::SelectObject(inHDC, mLabelFont);
      HBRUSH  hbr = ::CreateSolidBrush(RGB(0xFF, 0xFF, 0xFF));
      ::SetTextColor(inHDC, RGB(0x00, 0x00, 0x00));

#ifdef _UNICODE
      wchar_t buf[IMAGE_WINDOW_STR_BUF_SIZE];
#else
      const char *buf;
#endif
      const char *text = mOverlayText.c_str();
      int lineNum = (int )mOverlayTextLineList.size();
      for (int i = 0; i < lineNum; i++)
      {
        const char *lineStr;
        int len = mOverlayTextLineList[i].len;
        if (len != 0)
        {
          lineStr = &(text[mOverlayTextLineList[i].start]);
        }
        else
        {
          lineStr = " ";
          len = 1;
        }
        SIZE  size;
#ifdef _UNICODE
        MultiByteToWideChar(CP_ACP, 0, lineStr, len, buf, IMAGE_WINDOW_STR_BUF_SIZE);
        ::GetTextExtentPoint32W(inHDC, buf, len, &size);
#else
        buf = lineStr;
        ::GetTextExtentPoint32A(inHDC, buf, len, &size);
#endif
        if (size.cx > width)
          width = size.cx;
        if (size.cy > height)
          height = size.cy;
      }

      RECT rect = mImageDispRect;
      rect.left += 10;
      rect.top += 10;
      rect.right = rect.left + width + 8;
      rect.bottom = rect.top + height * lineNum;
      ::FillRect(inHDC, &rect, hbr);

      for (int i = 0; i < lineNum; i++)
      {
        const char *lineStr;
        int len = mOverlayTextLineList[i].len;
        if (len != 0)
        {
          lineStr = &(text[mOverlayTextLineList[i].start]);
#ifdef _UNICODE
          MultiByteToWideChar(CP_ACP, 0, lineStr, len, buf, IMAGE_WINDOW_STR_BUF_SIZE);
#else
          buf = lineStr;
#endif
          ::TextOut(inHDC,
                    rect.left+4, rect.top + i * height,
                    buf, len);
        }
      }
      ::SelectObject(inHDC, prevFont);
      ::DeleteObject(hbr);
    }
  }
  // ---------------------------------------------------------------------------
  // OnCommand
  // ---------------------------------------------------------------------------
  void  OnCommand(WPARAM inWParam, LPARAM inLParam)
  {
    switch (LOWORD(inWParam))
    {
      case IDM_SAVE:
        WriteBitmapFile();
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
          UpdateWindowDisp();
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
          IMAGE_WINDOW_ABOUT_STR,
          IMAGE_WINDOW_ABOUT_TITLE, MB_OK);
        break;
    }
  }
  // ---------------------------------------------------------------------------
  // OnKeyDown
  // ---------------------------------------------------------------------------
  void  OnKeyDown(WPARAM ininWParam, LPARAM inLParam)
  {
    switch (ininWParam)
    {
      case VK_SHIFT:
        UpdateMouseCursor();
        break;
    }
  }
  // ---------------------------------------------------------------------------
  // OnKeyUp
  // ---------------------------------------------------------------------------
  void  OnKeyUp(WPARAM ininWParam, LPARAM inLParam)
  {
    switch (ininWParam)
    {
      case VK_SHIFT:
        UpdateMouseCursor();
        break;
    }
  }
  // ---------------------------------------------------------------------------
  // OnChar
  // ---------------------------------------------------------------------------
  virtual void  OnChar(WPARAM ininWParam, LPARAM inLParam)
  {
    switch (ininWParam)
    {
      case VK_ESCAPE:
        if (IsFullScreenMode())
          DisableFullScreenMode();
        break;
      case 's':
      case 'S':
        WriteBitmapFile();
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
  // ---------------------------------------------------------------------------
  // OnLButtonDown
  // ---------------------------------------------------------------------------
  void  OnLButtonDown(UINT inMessage, WPARAM inWParam, LPARAM inLParam)
  {
    POINT localPos;
    int x, y;

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
        x = (int )(localPos.x / mImageDispScale);
        y = (int )(localPos.y / mImageDispScale);
        x += mImageDispOffset.cx;
        y += mImageDispOffset.cy;
        double scale;
        if ((inWParam & MK_SHIFT) == 0)
          scale = CalcImageScale(IMAGE_WINDOW_ZOOM_STEP);
        else
          scale = CalcImageScale(-IMAGE_WINDOW_ZOOM_STEP);
        mImageDispOffset.cx = x - (int )(localPos.x / scale);
        mImageDispOffset.cy = y - (int )(localPos.y / scale);
        SetDispScale(scale);
        break;
      case CURSOR_MODE_INFO_TOOL:
        localPos = mMouseDownPos;
        localPos.x -= mImageDispRect.left;
        localPos.y -= mImageDispRect.top;
        x = (int )(localPos.x / mImageDispScale);
        y = (int )(localPos.y / mImageDispScale);
        x += mImageDispOffset.cx;
        y += mImageDispOffset.cy;
        if (x <0 || y < 0 ||
          x >= mImageSize.cx || y >= mImageSize.cy)
          break;
        mImageClickNum++;
        if (IsColor())
        {
          int r = mBitmapBits[mImageSize.cx * y + x + 0];
          int g = mBitmapBits[mImageSize.cx * y + x + 1];
          int b = mBitmapBits[mImageSize.cx * y + x + 2];
          printf("%.4d: X:%.4d Y:%.4d VALUE:%.3d %.3d %.3d\n", mImageClickNum, x, y, r, g, b);
        }
        else
        {
          unsigned char v = mBitmapBits[mImageSize.cx * y + x];
          char  buf[256] = "00000000";
          for (int i = 0; i < 8; i++)
            if (((v << i) & 0x80) != 0)
              buf[i] = '1';
          printf("%.4d: X:%.4d Y:%.4d VALUE:%.3d %.2X %s\n", mImageClickNum, x, y, (int )v, (int )v, buf);
        }
        break;
    }
  }
  // ---------------------------------------------------------------------------
  // OnMButtonDown
  // ---------------------------------------------------------------------------
  void  OnMButtonDown(UINT inMessage, WPARAM inWParam, LPARAM inLParam)
  {
    POINT localPos;
    int x, y;

    ::ZeroMemory(&localPos, sizeof(localPos));
    localPos.x = (short )LOWORD(inLParam);
    localPos.y = (short )HIWORD(inLParam);

    localPos.x -= mImageDispRect.left;
    localPos.y -= mImageDispRect.top;
    x = (int )(localPos.x / mImageDispScale);
    y = (int )(localPos.y / mImageDispScale);
    x += mImageDispOffset.cx;
    y += mImageDispOffset.cy;

    double scale = mImageDispScale;
    if ((inWParam & (MK_SHIFT + MK_CONTROL)) == 0)
      scale = 1.0;
    else
    {
      if ((inWParam & MK_SHIFT) == 0)
        scale = 30.0;
      else
        scale = CalcWindowSizeFitScale();
    }

    if (mImageDispScale == scale)
      scale = mImagePrevScale;
    mImagePrevScale = mImageDispScale;

    mImageDispOffset.cx = x - (int )(localPos.x / scale);
    mImageDispOffset.cy = y - (int )(localPos.y / scale);
    SetDispScale(scale);
  }
  // ---------------------------------------------------------------------------
  // OnMouseMove
  // ---------------------------------------------------------------------------
  void  OnMouseMove(WPARAM inWParam, LPARAM inLParam)
  {
    if (mIsMouseDragging == false)
      return;

    POINT currentPos;

    ::ZeroMemory(&currentPos, sizeof(currentPos));
    currentPos.x = (short )LOWORD(inLParam);
    currentPos.y = (short )HIWORD(inLParam);

    switch (mMouseDownMode)
    {
      case CURSOR_MODE_SCROLL_TOOL:
        mImageDispOffset = mImageDispOffsetStart;
        mImageDispOffset.cx -= (int )((currentPos.x - mMouseDownPos.x) / mImageDispScale);
        mImageDispOffset.cy -= (int )((currentPos.y - mMouseDownPos.y) / mImageDispScale);
        CheckImageDispOffset();
        UpdateWindowDisp();
        break;
      case CURSOR_MODE_ZOOM_TOOL:
        break;
      case CURSOR_MODE_INFO_TOOL:
        break;
    }
  }
  // ---------------------------------------------------------------------------
  // OnLButtonUp
  // ---------------------------------------------------------------------------
  void  OnLButtonUp(WPARAM inWParam, LPARAM inLParam)
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
  // ---------------------------------------------------------------------------
  // OnRButtonDown
  // ---------------------------------------------------------------------------
  void  OnRButtonDown(UINT inMessage, WPARAM inWParam, LPARAM inLParam)
  {
    POINT pos;

    ::ZeroMemory(&pos, sizeof(pos));
    pos.x = (short )LOWORD(inLParam);
    pos.y = (short )HIWORD(inLParam);

    HMENU menu = GetSubMenu(mPopupMenuH, 0);
    ClientToScreen(mWindowH, &pos);
    TrackPopupMenu(menu, TPM_LEFTALIGN, pos.x, pos.y, 0, mWindowH, NULL);
  }
  // ---------------------------------------------------------------------------
  // OnMouseWheel
  // ---------------------------------------------------------------------------
  virtual void  OnMouseWheel(WPARAM inWParam, LPARAM inLParam)
  {
    POINT mousePos;
    int x, y, zDelta;

    ::ZeroMemory(&mousePos, sizeof(mousePos));
    mousePos.x = (short )LOWORD(inLParam);
    mousePos.y = (short )HIWORD(inLParam);
    ScreenToClient(mWindowH, &mousePos);
    if (PtInRect(&mImageDispRect, mousePos) == 0)
      return;

    zDelta = GET_WHEEL_DELTA_WPARAM(inWParam);

    mousePos.x -= mImageDispRect.left;
    mousePos.y -= mImageDispRect.top;
    x = (int )(mousePos.x / mImageDispScale);
    y = (int )(mousePos.y / mImageDispScale);
    x += mImageDispOffset.cx;
    y += mImageDispOffset.cy;

    double scale = mImageDispScale;
    if ((inWParam & (MK_SHIFT + MK_CONTROL)) == 0)
      scale = CalcImageScale(zDelta / IMAGE_WINDOW_MOUSE_WHEEL_STEP);
    else
    {
      if ((inWParam & MK_SHIFT) == 0)
        scale = CalcImageScale(zDelta / IMAGE_WINDOW_MOUSE_WHEEL_STEP * 4);
      else
        scale = CalcImageScale(zDelta / IMAGE_WINDOW_MOUSE_WHEEL_STEP / 2);
    }

    mImageDispOffset.cx = x - (int )(mousePos.x / scale);
    mImageDispOffset.cy = y - (int )(mousePos.y / scale);
    SetDispScale(scale);
  }
  // ---------------------------------------------------------------------------
  // OnSize
  // ---------------------------------------------------------------------------
  void  OnSize(WPARAM inWParam, LPARAM inLParam)
  {
    UpdateDispRect();
    if (GetKeyState(VK_SHIFT) < 0)
    {
      FitDispScaleToWindowSize();
      return;
    }

    if (CheckImageDispOffset())
      UpdateWindowDisp();

    UpdateStatusBar();
  }
  // ---------------------------------------------------------------------------
  // OnSetCursor
  // ---------------------------------------------------------------------------
  bool  OnSetCursor(WPARAM inWParam, LPARAM inLParam)
  {
    return UpdateMouseCursor();
  }
  // ---------------------------------------------------------------------------
  // UpdateMouseCursor
  // ---------------------------------------------------------------------------
  bool  UpdateMouseCursor()
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
  // ---------------------------------------------------------------------------
  // SetCursorMode
  // ---------------------------------------------------------------------------
  virtual void  SetCursorMode(int inCursorMode)
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
  // ---------------------------------------------------------------------------
  // UpdateWindowDisp
  // ---------------------------------------------------------------------------
  void  UpdateWindowDisp(bool inErase = false)
  {
    if (IsWindowOpen() == false)
      return;
    ::InvalidateRect(mWindowH, &mImageClientRect, inErase);
  }
  // ---------------------------------------------------------------------------
  // CalcImageScale
  // ---------------------------------------------------------------------------
  double  CalcImageScale(int inStep)
  {
    double  val, scale;

    val = log10(mImageDispScale * 100.0) + inStep / 100.0;
    scale = pow(10, val);

    // 100% snap & 1% limit (just in case...)
    if (fabs(scale - 100.0) <= 1.0)
      scale = 100;
    if (scale <= 1.0)
      scale = 1.0;

    return scale / 100.0;
  }
  // ---------------------------------------------------------------------------
  // CalcWindowSizeFitScale
  // ---------------------------------------------------------------------------
  double  CalcWindowSizeFitScale()
  {
    double  imageRatio = (double )mImageSize.cy / (double )mImageSize.cx;
    double width = mImageClientRect.right - mImageClientRect.left;
    double height = mImageClientRect.bottom - mImageClientRect.top;
    double  dispRatio = (double )mImageClientRect.bottom / width;
    double  scale;

    if (imageRatio > dispRatio)
      scale = height / (double )mImageSize.cy;
    else
      scale = width / (double )mImageSize.cx;

    return scale * 100.0;
  }
  // ---------------------------------------------------------------------------
  // IsScrollable
  // ---------------------------------------------------------------------------
  bool  IsScrollable()
  {
      int imageWidth = (int )(mImageSize.cx * mImageDispScale);
      int imageHeight = (int )(mImageSize.cy * mImageDispScale);

      if (imageWidth > mImageDispSize.cx)
        return true;
      if (imageHeight > mImageDispSize.cy)
        return true;

      return false;
  }
  // ---------------------------------------------------------------------------
  // InitFPS
  // ---------------------------------------------------------------------------
  void  InitFPS()
  {
    ::QueryPerformanceFrequency((LARGE_INTEGER *)&mFrequency);
    ::QueryPerformanceCounter((LARGE_INTEGER *)&mPrevCount);

    mFPSValue = 0;
    mFPSDataCount = 0;
  }
  // ---------------------------------------------------------------------------
  // UpdateFPS
  // ---------------------------------------------------------------------------
  void  UpdateFPS()
  {
    if (IsWindowOpen() == false || mBitmapInfo == NULL)
      return;

    unsigned __int64  currentCount = 0;
    ::QueryPerformanceCounter((LARGE_INTEGER *)&currentCount);

    mFPSValue = 1.0 * (double )mFrequency / (double )(currentCount - mPrevCount);
    mPrevCount = currentCount;

    if (mFPSDataCount < IMAGE_WINDOW_FPS_DATA_NUM)
      mFPSDataCount++;
    else
      ::MoveMemory(mFPSData, &(mFPSData[1]), sizeof(double) * (IMAGE_WINDOW_FPS_DATA_NUM - 1));
    mFPSData[mFPSDataCount - 1] = mFPSValue;

    double  averageValue = 0;
    for (int i = 0; i < mFPSDataCount; i++)
      averageValue += mFPSData[i];
    averageValue /= mFPSDataCount;

#ifdef _UNICODE
    wchar_t buf[IMAGE_WINDOW_STR_BUF_SIZE];

    swprintf_s(buf, IMAGE_WINDOW_STR_BUF_SIZE, TEXT("FPS: %.1f (avg.=%.1f)"), mFPSValue, averageValue);
    SendMessage(mStatusbarH, SB_SETTEXT, (WPARAM )3, (LPARAM )buf);
#else
    char  buf[IMAGE_WINDOW_STR_BUF_SIZE];

    sprintf_s(buf, IMAGE_WINDOW_STR_BUF_SIZE, TEXT("FPS: %.1f (avg.=%.1f)"), mFPSValue, averageValue);
    SendMessage(mStatusbarH, SB_SETTEXT, (WPARAM )3, (LPARAM )buf);
#endif
  }
  // ---------------------------------------------------------------------------
  // UpdateMousePixelReadout
  // ---------------------------------------------------------------------------
  bool  UpdateMousePixelReadout()
  {
    POINT pos;
    int x, y;
    bool  result;

    GetCursorPos(&pos);
    ScreenToClient(mWindowH, &pos);
    if (PtInRect(&mImageDispRect, pos) == 0)
      result = false;
    else
      result = true;

    pos.x -= mImageDispRect.left;
    pos.y -= mImageDispRect.top;
    x = (int )(pos.x / mImageDispScale);
    y = (int )(pos.y / mImageDispScale);
    x += mImageDispOffset.cx;
    y += mImageDispOffset.cy;

    unsigned char *pixelPtr = GetPixelPtr(x, y);

#ifdef _UNICODE
    wchar_t buf[IMAGE_WINDOW_STR_BUF_SIZE];

    if (result == false || pixelPtr == NULL)
      swprintf_s(buf, IMAGE_WINDOW_STR_BUF_SIZE, TEXT(""));
    else
    {
      if (mBitmapInfo->biBitCount == 8)
        swprintf_s(buf, IMAGE_WINDOW_STR_BUF_SIZE, TEXT("%03d (%d,%d)"), pixelPtr[0], x, y);
      else
        swprintf_s(buf, IMAGE_WINDOW_STR_BUF_SIZE, TEXT("%03d %03d %03d (%d,%d)"),
              (int )pixelPtr[2], (int )pixelPtr[1], (int )pixelPtr[0], x, y);
    }
    SendMessage(mStatusbarH, SB_SETTEXT, (WPARAM )2, (LPARAM )buf);
#else
    char  buf[IMAGE_WINDOW_STR_BUF_SIZE];

    if (result == false || pixelPtr == NULL)
      sprintf_s(buf, IMAGE_WINDOW_STR_BUF_SIZE, TEXT(""));
    else
    {
      if (mBitmapInfo->biBitCount == 8)
        sprintf_s(buf, IMAGE_WINDOW_STR_BUF_SIZE, TEXT("%03d (%d,%d)"), pixelPtr[0], x, y);
      else
        sprintf_s(buf, IMAGE_WINDOW_STR_BUF_SIZE, TEXT("%03d %03d %03d (%d,%d)"),
              (int )pixelPtr[2], (int )pixelPtr[1], (int )pixelPtr[0], x, y);
    }
    SendMessage(mStatusbarH, SB_SETTEXT, (WPARAM )2, (LPARAM )buf);
#endif
    return result;
  }
  // ---------------------------------------------------------------------------
  // UpdateWindowSize
  // ---------------------------------------------------------------------------
  void  UpdateWindowSize(bool inKeepDispScale = false)
  {
    if (IsWindowOpen() == false || mBitmapInfo == NULL)
      return;

    RECT  rebarRect, statusbarRect, rect;
    int   imageWidth, imageHeight;

    ::ZeroMemory(&rect, sizeof(rect));
    GetWindowRect(mRebarH, &rebarRect);
    int rebarHeight = rebarRect.bottom - rebarRect.top;
    if (!IsToolbarEnabled())
      rebarHeight = 0;
    GetWindowRect(mStatusbarH, &statusbarRect);
    int statusbarHeight = statusbarRect.bottom - statusbarRect.top;
    if (!IsStatusbarEnabled())
      statusbarHeight = 0;

    if (inKeepDispScale == false)
    {
      mImageDispScale = 1.0;
      imageWidth = mImageSize.cx;
      imageHeight = mImageSize.cy;
    }
    else
    {
      imageWidth = (int )(mImageSize.cx * mImageDispScale);
      imageHeight = (int )(mImageSize.cy * mImageDispScale);
    }

    int displayWidth = GetSystemMetrics(SM_CXSCREEN);
    int displayHEIGHT = GetSystemMetrics(SM_CYSCREEN);

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
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, TRUE); // if menu is enabled

    SetWindowPos(mWindowH, NULL, 0, 0,
      rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOOWNERZORDER);

    UpdateStatusBar();
  }
  // ---------------------------------------------------------------------------
  // UpdateDispRect
  // ---------------------------------------------------------------------------
  void  UpdateDispRect()
  {
    if (IsWindowOpen() == false || mBitmapInfo == NULL)
      return;

    RECT  rebarRect, statusbarRect, rect;

    GetWindowRect(mRebarH, &rebarRect);
    int rebarHeight = rebarRect.bottom - rebarRect.top;
    if (!IsToolbarEnabled())
      rebarHeight = 0;
    GetWindowRect(mStatusbarH, &statusbarRect);
    int statusbarHeight = statusbarRect.bottom - statusbarRect.top;
    if (!IsStatusbarEnabled())
      statusbarHeight = 0;

    GetClientRect(mWindowH, &rect);
    rect.top += rebarHeight;
    rect.bottom -= statusbarHeight;
    mImageClientRect = rect;
    bool  update = false;
    {
      int imageWidth = (int )(mImageSize.cx * mImageDispScale);
      int imageHeight = (int )(mImageSize.cy * mImageDispScale);
      if (imageWidth < rect.right - rect.left)
      {
        rect.left += (rect.right - rect.left - imageWidth) / 2;
        rect.right  += rect.left + imageWidth;
        update = true;
      }
      if (imageHeight < rect.bottom - rect.top)
      {
        rect.top  += (rect.bottom - rect.top - imageHeight) / 2;
        rect.right  += rect.top + imageHeight;
        update = true;
      }
    }
    mImageDispRect = rect;
    mImageDispSize.cx = mImageDispRect.right - mImageDispRect.left;
    mImageDispSize.cy = mImageDispRect.bottom - mImageDispRect.top;
    if (update)
      UpdateWindowDisp(true);
    else
      UpdateWindowDisp();
  }
  // ---------------------------------------------------------------------------
  // UpdateStatusBar
  // ---------------------------------------------------------------------------
  void  UpdateStatusBar()
  {
    RECT  rect;
    GetClientRect(mWindowH, &rect);

    int statusbarSize[] = {100, 300, rect.right - 200, rect.right};
    SendMessage(mStatusbarH, SB_SETPARTS, (WPARAM )4, (LPARAM )(LPINT)statusbarSize);

#ifdef _UNICODE
    wchar_t buf[IMAGE_WINDOW_STR_BUF_SIZE];

    swprintf_s(buf, IMAGE_WINDOW_STR_BUF_SIZE, TEXT("Zoom: %.0f%%"), mImageDispScale * 100.0);
    SendMessage(mStatusbarH, SB_SETTEXT, (WPARAM )0, (LPARAM )buf);

    if (mBitmapInfo != NULL)
    {
      swprintf_s(buf, IMAGE_WINDOW_STR_BUF_SIZE,
        TEXT("Image: %dx%dx%d"),
        mBitmapInfo->biWidth, abs(mBitmapInfo->biHeight), mBitmapInfo->biBitCount);
      SendMessage(mStatusbarH, SB_SETTEXT, (WPARAM )1, (LPARAM )buf);
    }

    swprintf_s(buf, IMAGE_WINDOW_STR_BUF_SIZE, TEXT("FPS: %.1f"), mFPSValue);
    SendMessage(mStatusbarH, SB_SETTEXT, (WPARAM )3, (LPARAM )buf);
#else
    char  buf[IMAGE_WINDOW_STR_BUF_SIZE];

    sprintf_s(buf, IMAGE_WINDOW_STR_BUF_SIZE, TEXT("Zoom: %.0f%%"), mImageDispScale * 100.0);
    SendMessage(mStatusbarH, SB_SETTEXT, (WPARAM )0, (LPARAM )buf);

    if (mBitmapInfo != NULL)
    {
      sprintf_s(buf, IMAGE_WINDOW_STR_BUF_SIZE,
        TEXT("Image: %dx%dx%d"),
        mBitmapInfo->biWidth, abs(mBitmapInfo->biHeight), mBitmapInfo->biBitCount);
      SendMessage(mStatusbarH, SB_SETTEXT, (WPARAM )1, (LPARAM )buf);
    }

    sprintf_s(buf, IMAGE_WINDOW_STR_BUF_SIZE, TEXT("FPS: %.1f"), mFPSValue);
    SendMessage(mStatusbarH, SB_SETTEXT, (WPARAM )3, (LPARAM )buf);
#endif
  }
  // ---------------------------------------------------------------------------
  // CheckImageDispOffset
  // ---------------------------------------------------------------------------
  bool  CheckImageDispOffset()
  {
    int limit;
    bool  update = false;
    SIZE  prevOffset = mImageDispOffset;

    limit = (int )(mImageSize.cx - mImageDispSize.cx / mImageDispScale);
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
    limit = (int )(mImageSize.cy - mImageDispSize.cy / mImageDispScale);
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
  // ---------------------------------------------------------------------------
  // ImageToDispPoint
  // ---------------------------------------------------------------------------
  void  ImageToDispPoint(LONG *ioX, LONG *ioY)
  {
    if (mImageDispScale == 1.0)
    {
      *ioX = *ioX - mImageDispOffset.cx + mImageDispRect.left;
      *ioY = *ioY - mImageDispOffset.cy + mImageDispRect.top;
    }
    else
    {
      *ioX = (int )((*ioX - mImageDispOffset.cx) * mImageDispScale) + mImageDispRect.left;
      *ioY = (int )((*ioY - mImageDispOffset.cy) * mImageDispScale) + mImageDispRect.top;
    }
  }
  // ---------------------------------------------------------------------------
  // ImageToDispRect
  // ---------------------------------------------------------------------------
  void  ImageToDispRect(RECT *ioRect)
  {
    ImageToDispPoint(&(ioRect->left), &(ioRect->top));
    ImageToDispPoint(&(ioRect->right), &(ioRect->bottom));
  }
  // ---------------------------------------------------------------------------
  // MyMonitorEnumProc
  // ---------------------------------------------------------------------------
  static BOOL CALLBACK  MyMonitorEnumProc(HMONITOR hMon, HDC hdcMon, LPRECT inMonRect, LPARAM inLParam)
  {
    ImageWindow *imageWindow = (ImageWindow *)inLParam;

    printf("Monitor(%d) %d, %d, %d, %d\n", imageWindow->mMonitorNum,
      inMonRect->left, inMonRect->top, inMonRect->right, inMonRect->bottom);
    imageWindow->mMonitorRect[imageWindow->mMonitorNum].left = inMonRect->left;
    imageWindow->mMonitorRect[imageWindow->mMonitorNum].top = inMonRect->top;
    imageWindow->mMonitorRect[imageWindow->mMonitorNum].right = inMonRect->right;
    imageWindow->mMonitorRect[imageWindow->mMonitorNum].bottom = inMonRect->bottom;
    if (imageWindow->mMonitorNum == IMAGE_WINDOW_MONITOR_ENUM_MAX - 1)
      return FALSE;
    imageWindow->mMonitorNum++;
    return TRUE;
  }
  // ---------------------------------------------------------------------------
  // RegisterWindowClass
  // ---------------------------------------------------------------------------
  int RegisterWindowClass()
  {
    WNDCLASSEX  wcx;

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
  // ---------------------------------------------------------------------------
  // InitMenu
  // ---------------------------------------------------------------------------
  void  InitMenu()
  {
    mMenuH = CreateMenu();
    mPopupMenuH = CreateMenu();
    HMENU popupSubMenuH = CreateMenu();
    HMENU fileMenuH = CreateMenu();
    HMENU editMenuH = CreateMenu();
    HMENU viewMenuH = CreateMenu();
    HMENU zoomMenuH = CreateMenu();
    HMENU windowMenuH = CreateMenu();
    HMENU helpMenuH = CreateMenu();

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
  // ---------------------------------------------------------------------------
  // InitToolbar
  // ---------------------------------------------------------------------------
  void  InitToolbar()
  {
    typedef BOOL    (WINAPI *_InitCommonControlsEx)(LPINITCOMMONCONTROLSEX);
    typedef HIMAGELIST  (WINAPI *_ImageList_Create)(int cx, int cy, UINT flags, int cInitial, int cGrow);
    typedef int     (WINAPI *_ImageList_AddMasked)(HIMAGELIST himl, HBITMAP hbmImage, COLORREF crMask);

    static HINSTANCE        hComctl32 = NULL;
    static _InitCommonControlsEx  _iw_InitCommonControlsEx = NULL;
    static _ImageList_Create    _iw_ImageList_Create = NULL;
    static _ImageList_AddMasked _iw_ImageList_AddMasked = NULL;

    //  to bypass "comctl32.lib" linking, do some hack...
    if (hComctl32 == NULL)
    {
      HINSTANCE hComctl32 = LoadLibrary(TEXT("comctl32.dll"));
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
      ::ZeroMemory(&icx, sizeof(icx));
      icx.dwSize = sizeof(INITCOMMONCONTROLSEX);
      icx.dwICC = ICC_BAR_CLASSES | ICC_COOL_CLASSES; // Specify BAR classes
      (*_iw_InitCommonControlsEx)(&icx); // Load the common control DLL
    }

    mRebarH = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL,
      WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS
        | RBS_BANDBORDERS | WS_CLIPCHILDREN | CCS_NODIVIDER,
      0, 0, 0, 0, mWindowH, NULL, GetModuleHandle(NULL), NULL);

    REBARINFO rInfo;
    ZeroMemory(&rInfo, sizeof(REBARINFO));
    rInfo.cbSize = sizeof(REBARINFO);
    SendMessage(mRebarH, RB_SETBARINFO, 0, (LPARAM )&rInfo);

    TBBUTTON  tbb[IMAGE_WINDOW_TOOLBAR_BUTTON_NUM];
    ZeroMemory(&tbb, sizeof(tbb));
    for (int i = 0; i < IMAGE_WINDOW_TOOLBAR_BUTTON_NUM; i++)
    {
      tbb[i].fsState = TBSTATE_ENABLED;
      tbb[i].fsStyle = TBSTYLE_BUTTON | BTNS_AUTOSIZE;
      tbb[i].dwData = 0L;
      tbb[i].iString = 0;
      tbb[i].iBitmap = MAKELONG(0, 0);
    }

    tbb[0].idCommand  = IDM_SAVE;
    tbb[0].iBitmap    = MAKELONG(0, 0);
    tbb[1].idCommand  = IDM_COPY;
    tbb[1].iBitmap    = MAKELONG(1, 0);
    tbb[2].fsStyle    = TBSTYLE_SEP;
    tbb[3].idCommand  = IDM_SCROLL_TOOL;
    tbb[3].iBitmap    = MAKELONG(2, 0);
    tbb[4].idCommand  = IDM_ZOOM_TOOL;
    tbb[4].iBitmap    = MAKELONG(3, 0);
    tbb[5].idCommand  = IDM_INFO_TOOL;
    tbb[5].iBitmap    = MAKELONG(4, 0);
    tbb[6].fsStyle    = TBSTYLE_SEP;
    tbb[7].idCommand  = IDM_FULL_SCREEN;
    tbb[7].iBitmap    = MAKELONG(5, 0);
    tbb[8].fsStyle    = TBSTYLE_SEP;
    tbb[9].idCommand  = IDM_FREEZE;
    tbb[9].iBitmap    = MAKELONG(6, 0);
    tbb[10].fsStyle   = TBSTYLE_SEP;
    tbb[11].idCommand = IDM_ZOOM_IN;
    tbb[11].iBitmap   = MAKELONG(7, 0);
    tbb[12].idCommand = IDM_ZOOM_OUT;
    tbb[12].iBitmap   = MAKELONG(8, 0);
    tbb[13].idCommand = IDM_ACTUAL_SIZE;
    tbb[13].iBitmap   = MAKELONG(9, 0);
    tbb[14].idCommand = IDM_FIT_WINDOW;
    tbb[14].iBitmap   = MAKELONG(10, 0);
    tbb[15].idCommand = IDM_ADJUST_WINDOW_SIZE;
    tbb[15].iBitmap   = MAKELONG(11, 0);

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

    static const unsigned char imageData[12][128] = {{  // Save Icon
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xF0, 0x0F, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0xF0, 0x0F, 0x88, 0x88, 0x88, 0x88, 0x88,
      0x88, 0xF0, 0x0F, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0xF0, 0x0F, 0x88, 0x88, 0x88, 0x88,
      0x88, 0x88, 0xF0, 0x0F, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0xF0, 0x0F, 0x88, 0x88, 0x88,
      0x88, 0x88, 0x88, 0xF0, 0x0F, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0xF0, 0x0F, 0x88, 0x8F,
      0xFF, 0xFF, 0xF8, 0x88, 0xF0, 0x0F, 0x88, 0x8F, 0xFF, 0xFF, 0xF8, 0x88, 0xF0, 0x0F, 0x88,
      0x8F, 0xFF, 0x88, 0xF8, 0x88, 0xF0, 0x0F, 0x88, 0x8F, 0xFF, 0x88, 0xF8, 0x8D, 0xF0, 0x0F,
      0x88, 0x8F, 0xFF, 0x88, 0xF8, 0xDF, 0xF0, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }, {    // Copy Icon
      0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x00, 0x0F, 0xBB, 0xBB, 0xBB, 0xBB, 0xF0, 0x00,
      0x00, 0x0F, 0xBF, 0xFF, 0xFF, 0xFB, 0xF0, 0x00, 0x00, 0x0F, 0xBF, 0xFF, 0xFF, 0xFB, 0xF0,
      0x00, 0x00, 0x0F, 0xBF, 0xFF, 0xFF, 0xFB, 0xFF, 0xFF, 0xF0, 0x0F, 0xBF, 0xFF, 0xFF, 0xFB,
      0xFB, 0xBB, 0xF0, 0x0F, 0xBF, 0xFF, 0xFF, 0xFB, 0xFF, 0xFB, 0xF0, 0x0F, 0xBF, 0xFF, 0xFF,
      0xFB, 0xFF, 0xFB, 0xF0, 0x0F, 0xBF, 0xFF, 0xFF, 0xFB, 0xFF, 0xFB, 0xF0, 0x0F, 0xBF, 0xFF,
      0xFF, 0xFB, 0xFF, 0xFB, 0xF0, 0x0F, 0xBB, 0xBB, 0xBB, 0xBB, 0xFF, 0xFB, 0xF0, 0x0F, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFB, 0xF0, 0x00, 0x00, 0x0F, 0xBF, 0xFF, 0xFF, 0xFB, 0xF0, 0x00,
      0x00, 0x0F, 0xBF, 0xFF, 0xFF, 0xFB, 0xF0, 0x00, 0x00, 0x0F, 0xBB, 0xBB, 0xBB, 0xBB, 0xF0,
      0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0
    }, {  //  Scroll Icon (Translate Icon)
      0x00, 0x00, 0x0F, 0xFD, 0xDF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xDB, 0xBD, 0xFF, 0x00,
      0x00, 0x00, 0x00, 0xFD, 0xBB, 0xBB, 0xDF, 0x00, 0x00, 0x00, 0x00, 0xFB, 0xDB, 0xBD, 0xBF,
      0x00, 0x00, 0x0F, 0xFF, 0xFD, 0xFB, 0xBF, 0xDF, 0xFF, 0xF0, 0xFF, 0xDB, 0xDF, 0xFB, 0xBF,
      0xFD, 0xBD, 0xFF, 0xFD, 0xBD, 0xFF, 0xFF, 0xFF, 0xFF, 0xDB, 0xDF, 0xDB, 0xBB, 0xBB, 0xFB,
      0xBF, 0xBB, 0xBB, 0xBD, 0xDB, 0xBB, 0xBB, 0xFB, 0xBF, 0xBB, 0xBB, 0xBD, 0xFD, 0xBD, 0xFF,
      0xFF, 0xFF, 0xFF, 0xDB, 0xDF, 0xFF, 0xDB, 0xDF, 0xFB, 0xBF, 0xFD, 0xBD, 0xFF, 0x0F, 0xFF,
      0xFD, 0xFB, 0xBF, 0xDF, 0xFF, 0xF0, 0x00, 0x00, 0xFB, 0xDB, 0xBD, 0xBF, 0x00, 0x00, 0x00,
      0x00, 0xFD, 0xBB, 0xBB, 0xDF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xDB, 0xBD, 0xFF, 0x00, 0x00,
      0x00, 0x00, 0x0F, 0xFD, 0xDF, 0xF0, 0x00, 0x00
    }, {  //  Zoom Icon
      0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x3C,
      0xEF, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF3, 0xBB, 0xCF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x3B,
      0xBB, 0x9F, 0x00, 0x0F, 0xFF, 0xFF, 0xF3, 0xBB, 0xB9, 0xFF, 0x00, 0xFE, 0xCB, 0xBB, 0xDB,
      0xBB, 0x9F, 0xF0, 0x0F, 0xEB, 0x9F, 0xFF, 0x9B, 0xB9, 0xFF, 0x00, 0x0F, 0xC9, 0xFF, 0xFF,
      0xF9, 0xCF, 0xF0, 0x00, 0x0F, 0xBF, 0xFF, 0xFF, 0xFF, 0xBF, 0xF0, 0x00, 0x0F, 0xBF, 0xFF,
      0xFF, 0xFF, 0xBE, 0xF0, 0x00, 0x0F, 0xBF, 0xFF, 0xFF, 0xFF, 0xBF, 0xF0, 0x00, 0x0F, 0xC9,
      0xFF, 0xFF, 0xF9, 0xCF, 0x00, 0x00, 0x0F, 0xEB, 0x9F, 0xFF, 0x9B, 0xEF, 0x00, 0x00, 0x00,
      0xFE, 0xCB, 0xBB, 0xC3, 0xF0, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }, {  //  Cross Icon
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0xB0, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x0B, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0xB0, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0xB0,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0xB0, 0x00, 0x00, 0x00, 0x0B, 0xBB, 0xBB, 0xBB,
      0xBB, 0xBB, 0xBB, 0xB0, 0x0B, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xB0, 0x00, 0x00, 0x00,
      0x0B, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x0B, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0xB0, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x0B, 0xB0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0xB0, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }, {  //  Full Icon
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB,
      0xBF, 0xFB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0xFB, 0xFB, 0xBB, 0xBB, 0xBB, 0xFF,
      0xFF, 0xBF, 0xFB, 0xFB, 0xFF, 0xFF, 0xFB, 0xFF, 0xFF, 0xBF, 0xFB, 0xFB, 0xFF, 0xFF, 0xFB,
      0xFF, 0xFF, 0xBF, 0xFB, 0xFB, 0xFF, 0xFF, 0xFB, 0xFF, 0xFF, 0xBF, 0xFB, 0xFB, 0xFF, 0xFF,
      0xFB, 0xFF, 0xFF, 0xBF, 0xFB, 0xFB, 0xBB, 0xBB, 0xB9, 0xFF, 0xFF, 0xBF, 0xFB, 0xFB, 0xBB,
      0xBB, 0x9D, 0xDF, 0xDF, 0xBF, 0xFB, 0xFF, 0xFF, 0xFF, 0xFD, 0xBD, 0xBF, 0xBF, 0xFB, 0xFF,
      0xFF, 0xFF, 0xFF, 0xDB, 0xBF, 0xBF, 0xFB, 0xFF, 0xFF, 0xFF, 0xFD, 0xBB, 0xBF, 0xBF, 0xFB,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBF, 0xFB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    }, {  //  Pause Icon
      0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x3C, 0xBB, 0xBB, 0xC3, 0xFF,
      0x00, 0x0F, 0xFD, 0xBB, 0xBB, 0xBB, 0xBB, 0xDF, 0xF0, 0x0F, 0xDB, 0xBB, 0xBB, 0xBB, 0xBB,
      0xBD, 0xF0, 0xF3, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0x3F, 0xFC, 0xBB, 0xBF, 0xFB, 0xBF,
      0xFB, 0xBB, 0xCF, 0xFB, 0xBB, 0xBF, 0xFB, 0xBF, 0xFB, 0xBB, 0xBF, 0xFB, 0xBB, 0xBF, 0xFB,
      0xBF, 0xFB, 0xBB, 0xBF, 0xFB, 0xBB, 0xBF, 0xFB, 0xBF, 0xFB, 0xBB, 0xBF, 0xFB, 0xBB, 0xBF,
      0xFB, 0xBF, 0xFB, 0xBB, 0xBF, 0xFC, 0xBB, 0xBF, 0xFB, 0xBF, 0xFB, 0xBB, 0xCF, 0xF3, 0xBB,
      0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0x3F, 0x0F, 0xDB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBD, 0xF0, 0x0F,
      0xFD, 0xBB, 0xBB, 0xBB, 0xBB, 0xDF, 0xF0, 0x00, 0xFF, 0x3C, 0xBB, 0xBB, 0xC3, 0xFF, 0x00,
      0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00
    }, {  // Zoom In Icon
      0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x3C,
      0xEF, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF3, 0xBB, 0xCF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x3B,
      0xBB, 0x9F, 0x00, 0x0F, 0xFF, 0xFF, 0xF3, 0xBB, 0xB9, 0xFF, 0x00, 0xFE, 0xCB, 0xBB, 0xDB,
      0xBB, 0x9F, 0xF0, 0x0F, 0xEB, 0x9F, 0xFF, 0x9B, 0xB9, 0xFF, 0x00, 0x0F, 0xC9, 0xFF, 0x8F,
      0xF9, 0xCF, 0xF0, 0x00, 0x0F, 0xBF, 0xFF, 0x8F, 0xFF, 0xBF, 0xF0, 0x00, 0x0F, 0xBF, 0x88,
      0x88, 0x8F, 0xBE, 0xF0, 0x00, 0x0F, 0xBF, 0xFF, 0x8F, 0xFF, 0xBF, 0xF0, 0x00, 0x0F, 0xC9,
      0xFF, 0x8F, 0xF9, 0xCF, 0x00, 0x00, 0x0F, 0xEB, 0x9F, 0xFF, 0x9B, 0xEF, 0x00, 0x00, 0x00,
      0xFE, 0xCB, 0xBB, 0xC3, 0xF0, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }, {  //  Zoom Out Icon
      0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x3C,
      0xEF, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF3, 0xBB, 0xCF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x3B,
      0xBB, 0x9F, 0x00, 0x0F, 0xFF, 0xFF, 0xF3, 0xBB, 0xB9, 0xFF, 0x00, 0xFE, 0xCB, 0xBB, 0xDB,
      0xBB, 0x9F, 0xF0, 0x0F, 0xEB, 0x9F, 0xFF, 0x9B, 0xB9, 0xFF, 0x00, 0x0F, 0xC9, 0xFF, 0xFF,
      0xF9, 0xCF, 0xF0, 0x00, 0x0F, 0xBF, 0xFF, 0xFF, 0xFF, 0xBF, 0xF0, 0x00, 0x0F, 0xBF, 0x88,
      0x88, 0x8F, 0xBF, 0xF0, 0x00, 0x0F, 0xBF, 0xFF, 0xFF, 0xFF, 0xBF, 0xF0, 0x00, 0x0F, 0xC9,
      0xFF, 0xFF, 0xF9, 0xCF, 0x00, 0x00, 0x0F, 0xEB, 0x9F, 0xFF, 0x9B, 0xEF, 0x00, 0x00, 0x00,
      0xFE, 0xCB, 0xBB, 0xC3, 0xF0, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }, {  //  Actual Size Icon
      0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x3C,
      0xEF, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF3, 0xBB, 0xCF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x3B,
      0xBB, 0x9F, 0x00, 0x0F, 0xFF, 0xFF, 0xF3, 0xBB, 0xB9, 0xFF, 0x00, 0xFE, 0xCB, 0xBB, 0xDB,
      0xBB, 0x9F, 0xF0, 0x0F, 0xEB, 0x9F, 0xFF, 0x9B, 0xB9, 0xFF, 0x00, 0x0F, 0xCB, 0xBB, 0xBB,
      0xBB, 0xBB, 0xBB, 0xBB, 0x0F, 0xBB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFB, 0x0F, 0xBB, 0xFB,
      0xFF, 0xBF, 0xFF, 0xBF, 0xFB, 0x0F, 0xBB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0x0F, 0xCB,
      0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0x0F, 0xEB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0x00,
      0xFB, 0xFB, 0xFF, 0xBF, 0xFF, 0xBF, 0xFB, 0x00, 0x0B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFB,
      0x00, 0x0B, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB
    }, {  //  Fit to Window Icon
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0x00, 0x00, 0xFB, 0xBB, 0xBB, 0xBB, 0xFD, 0xDF, 0x00, 0x00, 0xFB, 0xBB, 0xBB, 0xB9,
      0xDB, 0xDF, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFD, 0xBD, 0xFF, 0x0F, 0xBB, 0xFF, 0xDB, 0xBB,
      0xCB, 0xD9, 0xBF, 0x0F, 0xBB, 0xFD, 0xCE, 0xFE, 0xCC, 0xFB, 0xBF, 0x0F, 0xBB, 0xFB, 0xEF,
      0xBF, 0xEB, 0xFB, 0xBF, 0x0F, 0xBB, 0xFB, 0xFB, 0xBB, 0xFB, 0xFB, 0xBF, 0x0F, 0xBB, 0xFB,
      0xEF, 0xBF, 0xEB, 0xFB, 0xBF, 0x0F, 0xBB, 0xFD, 0xCE, 0xFE, 0xCD, 0xFB, 0xBF, 0x0F, 0xBB,
      0xFF, 0xDB, 0xBB, 0xDF, 0xFB, 0xBF, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
      0x00, 0xFB, 0xBB, 0xBB, 0xBB, 0xF0, 0x00, 0x00, 0x00, 0xFB, 0xBB, 0xBB, 0xBB, 0xF0, 0x00,
      0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x00
    }, {  //  Adjust Window Icon
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
    ::ZeroMemory(data, sizeof(data));
    BITMAPINFOHEADER  *bitmapHeader = (BITMAPINFOHEADER *)header;
    if (mIsHiDPI)
    {
      bitmapHeader->biHeight *= 2;
      bitmapHeader->biWidth  *= 2;
      bitmapHeader->biSizeImage *= 4;
    }
    HIMAGELIST  imageList = (*_iw_ImageList_Create)(
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
    SendMessage(mToolbarH, TB_ADDBUTTONS, (WPARAM )IMAGE_WINDOW_TOOLBAR_BUTTON_NUM, (LPARAM )tbb);

    REBARBANDINFO rbInfo;
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
  // ---------------------------------------------------------------------------
  // InitCursor
  // ---------------------------------------------------------------------------
  void  InitCursor()
  {
    static const unsigned char andPlane[3][128] = {{  // Scroll Hand
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    },{ //  Zoom Plus
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    },{ //  Zoom Minus
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

    static const unsigned char xorPlane[3][128] = {{  //  Scroll
      0x01, 0x80, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x07, 0xE0, 0x00, 0x00, 0x05, 0xA0, 0x00,
      0x00, 0x01, 0x80, 0x00, 0x00, 0x31, 0x8C, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0xFD, 0xBF,
      0x00, 0x00, 0xFD, 0xBF, 0x00, 0x00, 0x60, 0x06, 0x00, 0x00, 0x31, 0x8C, 0x00, 0x00, 0x01,
      0x80, 0x00, 0x00, 0x05, 0xA0, 0x00, 0x00, 0x07, 0xE0, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00,
      0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    },{ //  Zoom Plus
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x80, 0x00, 0x00, 0x10, 0x40, 0x00,
      0x00, 0x22, 0x20, 0x00, 0x00, 0x22, 0x20, 0x00, 0x00, 0x2F, 0xA0, 0x00, 0x00, 0x22, 0x20,
      0x00, 0x00, 0x22, 0x20, 0x00, 0x00, 0x10, 0x60, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x00,
      0x38, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    },{ //  Zoom Minus
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
      mScrollCursor   = CreateCursor(mModuleH, 0, 0, 32, 32, andPlane[0], xorPlane[0]);
      mZoomPlusCursor   = CreateCursor(mModuleH, 0, 0, 32, 32, andPlane[1], xorPlane[1]);
      mZoomMinusCursor  = CreateCursor(mModuleH, 0, 0, 32, 32, andPlane[2], xorPlane[2]);
    }
    else
    {
      // ToDo : Improve this workaround.
      // At least, mZoomPlusCursor and mZoomMinusCursor
      mScrollCursor   = LoadCursor(NULL, IDC_SIZEALL);
      mZoomPlusCursor   = LoadCursor(NULL, IDC_ARROW);
      mZoomMinusCursor  = LoadCursor(NULL, IDC_ARROW);
    }

    mArrowCursor  = LoadCursor(NULL, IDC_ARROW);
    mInfoCursor   = LoadCursor(NULL, IDC_CROSS);
  }
  // ---------------------------------------------------------------------------
  // InitIcon
  // ---------------------------------------------------------------------------
  void  InitIcon()
  {
    static const unsigned char  data[] = {
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
  // ---------------------------------------------------------------------------
  // GetColorMap
  // ---------------------------------------------------------------------------
  static const unsigned char *GetColorMap(ColorMap inColorMap)
  {
    static const unsigned char s_grayscale_map[256*3] =
    {
      0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05,
      0x05, 0x05, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x0A, 0x0A,
      0x0A, 0x0B, 0x0B, 0x0B, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0x0F,
      0x10, 0x10, 0x10, 0x11, 0x11, 0x11, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x14, 0x14, 0x14, 0x15,
      0x15, 0x15, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17, 0x18, 0x18, 0x18, 0x19, 0x19, 0x19, 0x1A, 0x1A,
      0x1A, 0x1B, 0x1B, 0x1B, 0x1C, 0x1C, 0x1C, 0x1D, 0x1D, 0x1D, 0x1E, 0x1E, 0x1E, 0x1F, 0x1F, 0x1F,
      0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x22, 0x22, 0x22, 0x23, 0x23, 0x23, 0x24, 0x24, 0x24, 0x24,
      0x24, 0x24, 0x26, 0x26, 0x26, 0x27, 0x27, 0x27, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x2A, 0x2A,
      0x2A, 0x2B, 0x2B, 0x2B, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2E, 0x2E, 0x2E, 0x2F, 0x2F, 0x2F,
      0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x32, 0x32, 0x32, 0x33, 0x33, 0x33, 0x34, 0x34, 0x34, 0x34,
      0x34, 0x34, 0x36, 0x36, 0x36, 0x37, 0x37, 0x37, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x3A, 0x3A,
      0x3A, 0x3B, 0x3B, 0x3B, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C, 0x3E, 0x3E, 0x3E, 0x3F, 0x3F, 0x3F,
      0x40, 0x40, 0x40, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x43, 0x43, 0x43, 0x44, 0x44, 0x44, 0x45,
      0x45, 0x45, 0x46, 0x46, 0x46, 0x47, 0x47, 0x47, 0x48, 0x48, 0x48, 0x49, 0x49, 0x49, 0x49, 0x49,
      0x49, 0x4B, 0x4B, 0x4B, 0x4C, 0x4C, 0x4C, 0x4D, 0x4D, 0x4D, 0x4E, 0x4E, 0x4E, 0x4F, 0x4F, 0x4F,
      0x50, 0x50, 0x50, 0x51, 0x51, 0x51, 0x51, 0x51, 0x51, 0x53, 0x53, 0x53, 0x54, 0x54, 0x54, 0x55,
      0x55, 0x55, 0x56, 0x56, 0x56, 0x57, 0x57, 0x57, 0x58, 0x58, 0x58, 0x59, 0x59, 0x59, 0x59, 0x59,
      0x59, 0x5B, 0x5B, 0x5B, 0x5C, 0x5C, 0x5C, 0x5D, 0x5D, 0x5D, 0x5E, 0x5E, 0x5E, 0x5F, 0x5F, 0x5F,
      0x60, 0x60, 0x60, 0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x63, 0x63, 0x63, 0x64, 0x64, 0x64, 0x65,
      0x65, 0x65, 0x66, 0x66, 0x66, 0x67, 0x67, 0x67, 0x68, 0x68, 0x68, 0x69, 0x69, 0x69, 0x69, 0x69,
      0x69, 0x6B, 0x6B, 0x6B, 0x6C, 0x6C, 0x6C, 0x6D, 0x6D, 0x6D, 0x6E, 0x6E, 0x6E, 0x6F, 0x6F, 0x6F,
      0x70, 0x70, 0x70, 0x71, 0x71, 0x71, 0x71, 0x71, 0x71, 0x73, 0x73, 0x73, 0x74, 0x74, 0x74, 0x75,
      0x75, 0x75, 0x76, 0x76, 0x76, 0x77, 0x77, 0x77, 0x78, 0x78, 0x78, 0x79, 0x79, 0x79, 0x79, 0x79,
      0x79, 0x7B, 0x7B, 0x7B, 0x7C, 0x7C, 0x7C, 0x7D, 0x7D, 0x7D, 0x7E, 0x7E, 0x7E, 0x7F, 0x7F, 0x7F,
      0x80, 0x80, 0x80, 0x81, 0x81, 0x81, 0x82, 0x82, 0x82, 0x83, 0x83, 0x83, 0x83, 0x83, 0x83, 0x85,
      0x85, 0x85, 0x86, 0x86, 0x86, 0x87, 0x87, 0x87, 0x88, 0x88, 0x88, 0x89, 0x89, 0x89, 0x8A, 0x8A,
      0x8A, 0x8B, 0x8B, 0x8B, 0x8C, 0x8C, 0x8C, 0x8D, 0x8D, 0x8D, 0x8E, 0x8E, 0x8E, 0x8F, 0x8F, 0x8F,
      0x90, 0x90, 0x90, 0x91, 0x91, 0x91, 0x92, 0x92, 0x92, 0x93, 0x93, 0x93, 0x93, 0x93, 0x93, 0x95,
      0x95, 0x95, 0x96, 0x96, 0x96, 0x97, 0x97, 0x97, 0x98, 0x98, 0x98, 0x99, 0x99, 0x99, 0x9A, 0x9A,
      0x9A, 0x9B, 0x9B, 0x9B, 0x9C, 0x9C, 0x9C, 0x9D, 0x9D, 0x9D, 0x9E, 0x9E, 0x9E, 0x9F, 0x9F, 0x9F,
      0xA0, 0xA0, 0xA0, 0xA1, 0xA1, 0xA1, 0xA2, 0xA2, 0xA2, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA5,
      0xA5, 0xA5, 0xA6, 0xA6, 0xA6, 0xA7, 0xA7, 0xA7, 0xA8, 0xA8, 0xA8, 0xA9, 0xA9, 0xA9, 0xAA, 0xAA,
      0xAA, 0xAB, 0xAB, 0xAB, 0xAC, 0xAC, 0xAC, 0xAD, 0xAD, 0xAD, 0xAE, 0xAE, 0xAE, 0xAF, 0xAF, 0xAF,
      0xB0, 0xB0, 0xB0, 0xB1, 0xB1, 0xB1, 0xB2, 0xB2, 0xB2, 0xB3, 0xB3, 0xB3, 0xB3, 0xB3, 0xB3, 0xB5,
      0xB5, 0xB5, 0xB6, 0xB6, 0xB6, 0xB7, 0xB7, 0xB7, 0xB8, 0xB8, 0xB8, 0xB9, 0xB9, 0xB9, 0xBA, 0xBA,
      0xBA, 0xBB, 0xBB, 0xBB, 0xBC, 0xBC, 0xBC, 0xBD, 0xBD, 0xBD, 0xBE, 0xBE, 0xBE, 0xBF, 0xBF, 0xBF,
      0xC0, 0xC0, 0xC0, 0xC1, 0xC1, 0xC1, 0xC2, 0xC2, 0xC2, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC5,
      0xC5, 0xC5, 0xC6, 0xC6, 0xC6, 0xC7, 0xC7, 0xC7, 0xC8, 0xC8, 0xC8, 0xC9, 0xC9, 0xC9, 0xCA, 0xCA,
      0xCA, 0xCB, 0xCB, 0xCB, 0xCC, 0xCC, 0xCC, 0xCD, 0xCD, 0xCD, 0xCE, 0xCE, 0xCE, 0xCF, 0xCF, 0xCF,
      0xD0, 0xD0, 0xD0, 0xD1, 0xD1, 0xD1, 0xD2, 0xD2, 0xD2, 0xD3, 0xD3, 0xD3, 0xD3, 0xD3, 0xD3, 0xD5,
      0xD5, 0xD5, 0xD6, 0xD6, 0xD6, 0xD7, 0xD7, 0xD7, 0xD8, 0xD8, 0xD8, 0xD9, 0xD9, 0xD9, 0xDA, 0xDA,
      0xDA, 0xDB, 0xDB, 0xDB, 0xDC, 0xDC, 0xDC, 0xDD, 0xDD, 0xDD, 0xDE, 0xDE, 0xDE, 0xDF, 0xDF, 0xDF,
      0xE0, 0xE0, 0xE0, 0xE1, 0xE1, 0xE1, 0xE2, 0xE2, 0xE2, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE5,
      0xE5, 0xE5, 0xE6, 0xE6, 0xE6, 0xE7, 0xE7, 0xE7, 0xE8, 0xE8, 0xE8, 0xE9, 0xE9, 0xE9, 0xEA, 0xEA,
      0xEA, 0xEB, 0xEB, 0xEB, 0xEC, 0xEC, 0xEC, 0xED, 0xED, 0xED, 0xEE, 0xEE, 0xEE, 0xEF, 0xEF, 0xEF,
      0xF0, 0xF0, 0xF0, 0xF1, 0xF1, 0xF1, 0xF2, 0xF2, 0xF2, 0xF3, 0xF3, 0xF3, 0xF3, 0xF3, 0xF3, 0xF5,
      0xF5, 0xF5, 0xF6, 0xF6, 0xF6, 0xF7, 0xF7, 0xF7, 0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xFA, 0xFA,
      0xFA, 0xFB, 0xFB, 0xFB, 0xFC, 0xFC, 0xFC, 0xFD, 0xFD, 0xFD, 0xFE, 0xFE, 0xFE, 0xFF, 0xFF, 0xFF
    };
    static const unsigned char s_jet_map[256*3] =
    {
      0x00, 0x00, 0x7F, 0x00, 0x00, 0x84, 0x00, 0x00, 0x89, 0x00, 0x00, 0x8F, 0x00, 0x00, 0x94, 0x00,
      0x00, 0x99, 0x00, 0x00, 0x9F, 0x00, 0x00, 0xA4, 0x00, 0x00, 0xA9, 0x00, 0x00, 0xAF, 0x00, 0x00,
      0xB4, 0x00, 0x00, 0xB9, 0x00, 0x00, 0xBF, 0x00, 0x00, 0xC4, 0x00, 0x00, 0xC9, 0x00, 0x00, 0xCF,
      0x00, 0x00, 0xD4, 0x00, 0x00, 0xD9, 0x00, 0x00, 0xDF, 0x00, 0x00, 0xE4, 0x00, 0x00, 0xE9, 0x00,
      0x00, 0xEF, 0x00, 0x00, 0xF4, 0x00, 0x00, 0xF9, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x04,
      0xFF, 0x00, 0x08, 0xFF, 0x00, 0x0C, 0xFF, 0x00, 0x10, 0xFF, 0x00, 0x14, 0xFF, 0x00, 0x18, 0xFF,
      0x00, 0x1C, 0xFF, 0x00, 0x20, 0xFF, 0x00, 0x25, 0xFF, 0x00, 0x29, 0xFF, 0x00, 0x2D, 0xFF, 0x00,
      0x31, 0xFE, 0x00, 0x35, 0xFF, 0x00, 0x39, 0xFF, 0x00, 0x3D, 0xFF, 0x00, 0x41, 0xFF, 0x00, 0x45,
      0xFF, 0x00, 0x4A, 0xFF, 0x00, 0x4E, 0xFF, 0x00, 0x52, 0xFF, 0x00, 0x56, 0xFF, 0x00, 0x5A, 0xFF,
      0x00, 0x5E, 0xFF, 0x00, 0x62, 0xFF, 0x00, 0x66, 0xFF, 0x00, 0x6A, 0xFF, 0x00, 0x6F, 0xFF, 0x00,
      0x73, 0xFE, 0x00, 0x77, 0xFF, 0x00, 0x7B, 0xFF, 0x00, 0x7F, 0xFF, 0x00, 0x83, 0xFF, 0x00, 0x87,
      0xFF, 0x00, 0x8B, 0xFF, 0x00, 0x8F, 0xFF, 0x00, 0x94, 0xFF, 0x00, 0x98, 0xFF, 0x00, 0x9C, 0xFF,
      0x00, 0xA0, 0xFF, 0x00, 0xA4, 0xFF, 0x00, 0xA8, 0xFF, 0x00, 0xAC, 0xFF, 0x00, 0xB0, 0xFF, 0x00,
      0xB4, 0xFF, 0x00, 0xB9, 0xFF, 0x00, 0xBD, 0xFF, 0x00, 0xC1, 0xFF, 0x00, 0xC5, 0xFF, 0x00, 0xC9,
      0xFF, 0x00, 0xCD, 0xFF, 0x00, 0xD1, 0xFF, 0x00, 0xD5, 0xFF, 0x00, 0xD9, 0xFF, 0x00, 0xDE, 0xFF,
      0x00, 0xE2, 0xFF, 0x00, 0xE6, 0xFF, 0x00, 0xEA, 0xFF, 0x00, 0xEE, 0xFF, 0x00, 0xF2, 0xFF, 0x00,
      0xF6, 0xFF, 0x00, 0xFA, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xF8, 0x00, 0xFF,
      0xF1, 0x00, 0xFF, 0xEA, 0x00, 0xFF, 0xE3, 0x00, 0xFF, 0xDC, 0x00, 0xFF, 0xD5, 0x00, 0xFF, 0xCE,
      0x00, 0xFF, 0xC7, 0x00, 0xFF, 0xC0, 0x00, 0xFF, 0xBA, 0x00, 0xFF, 0xB3, 0x00, 0xFF, 0xAC, 0x00,
      0xFF, 0xA5, 0x00, 0xFF, 0x9E, 0x00, 0xFF, 0x97, 0x00, 0xFF, 0x90, 0x00, 0xFF, 0x89, 0x00, 0xFF,
      0x82, 0x00, 0xFF, 0x7C, 0x00, 0xFF, 0x75, 0x00, 0xFF, 0x6E, 0x00, 0xFF, 0x67, 0x00, 0xFF, 0x60,
      0x00, 0xFF, 0x59, 0x00, 0xFF, 0x52, 0x00, 0xFF, 0x4B, 0x00, 0xFF, 0x44, 0x00, 0xFF, 0x3E, 0x00,
      0xFF, 0x37, 0x00, 0xFF, 0x30, 0x00, 0xFF, 0x29, 0x00, 0xFF, 0x22, 0x00, 0xFF, 0x1B, 0x00, 0xFF,
      0x14, 0x00, 0xFF, 0x0D, 0x00, 0xFF, 0x06, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x06, 0xFF, 0x00,
      0x0D, 0xFF, 0x00, 0x14, 0xFF, 0x00, 0x1B, 0xFF, 0x00, 0x22, 0xFF, 0x00, 0x29, 0xFF, 0x00, 0x30,
      0xFF, 0x00, 0x37, 0xFF, 0x00, 0x3E, 0xFF, 0x00, 0x44, 0xFF, 0x00, 0x4B, 0xFF, 0x00, 0x52, 0xFF,
      0x00, 0x59, 0xFF, 0x00, 0x60, 0xFF, 0x00, 0x67, 0xFF, 0x00, 0x6E, 0xFF, 0x00, 0x75, 0xFF, 0x00,
      0x7C, 0xFF, 0x00, 0x82, 0xFF, 0x00, 0x89, 0xFF, 0x00, 0x90, 0xFF, 0x00, 0x97, 0xFF, 0x00, 0x9E,
      0xFF, 0x00, 0xA5, 0xFF, 0x00, 0xAC, 0xFF, 0x00, 0xB3, 0xFF, 0x00, 0xBA, 0xFF, 0x00, 0xC0, 0xFF,
      0x00, 0xC7, 0xFF, 0x00, 0xCE, 0xFF, 0x00, 0xD5, 0xFF, 0x00, 0xDC, 0xFF, 0x00, 0xE3, 0xFF, 0x00,
      0xEA, 0xFF, 0x00, 0xF1, 0xFF, 0x00, 0xF8, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF,
      0xFA, 0x00, 0xFF, 0xF6, 0x00, 0xFE, 0xF2, 0x00, 0xFF, 0xEE, 0x00, 0xFF, 0xEA, 0x00, 0xFF, 0xE6,
      0x00, 0xFF, 0xE2, 0x00, 0xFF, 0xDE, 0x00, 0xFF, 0xDA, 0x00, 0xFF, 0xD6, 0x00, 0xFE, 0xD2, 0x00,
      0xFF, 0xCE, 0x00, 0xFF, 0xCA, 0x00, 0xFF, 0xC6, 0x00, 0xFF, 0xC2, 0x00, 0xFF, 0xBE, 0x00, 0xFF,
      0xBA, 0x00, 0xFF, 0xB6, 0x00, 0xFE, 0xB2, 0x00, 0xFF, 0xAE, 0x00, 0xFF, 0xAA, 0x00, 0xFF, 0xA5,
      0x00, 0xFF, 0xA1, 0x00, 0xFF, 0x9D, 0x00, 0xFF, 0x99, 0x00, 0xFF, 0x95, 0x00, 0xFE, 0x91, 0x00,
      0xFF, 0x8D, 0x00, 0xFF, 0x89, 0x00, 0xFF, 0x85, 0x00, 0xFF, 0x81, 0x00, 0xFF, 0x7D, 0x00, 0xFF,
      0x79, 0x00, 0xFF, 0x75, 0x00, 0xFF, 0x71, 0x00, 0xFF, 0x6D, 0x00, 0xFF, 0x69, 0x00, 0xFF, 0x65,
      0x00, 0xFF, 0x61, 0x00, 0xFF, 0x5D, 0x00, 0xFF, 0x59, 0x00, 0xFF, 0x55, 0x00, 0xFF, 0x50, 0x00,
      0xFF, 0x4C, 0x00, 0xFF, 0x48, 0x00, 0xFF, 0x44, 0x00, 0xFF, 0x40, 0x00, 0xFF, 0x3C, 0x00, 0xFF,
      0x38, 0x00, 0xFF, 0x34, 0x00, 0xFF, 0x30, 0x00, 0xFF, 0x2C, 0x00, 0xFF, 0x28, 0x00, 0xFF, 0x24,
      0x00, 0xFF, 0x20, 0x00, 0xFF, 0x1C, 0x00, 0xFF, 0x18, 0x00, 0xFF, 0x14, 0x00, 0xFF, 0x10, 0x00,
      0xFF, 0x0C, 0x00, 0xFF, 0x08, 0x00, 0xFF, 0x04, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFA,
      0x00, 0x00, 0xF5, 0x00, 0x00, 0xF0, 0x00, 0x00, 0xEC, 0x00, 0x00, 0xE7, 0x00, 0x00, 0xE2, 0x00,
      0x00, 0xDD, 0x00, 0x00, 0xD9, 0x00, 0x00, 0xD4, 0x00, 0x00, 0xCF, 0x00, 0x00, 0xCA, 0x00, 0x00,
      0xC6, 0x00, 0x00, 0xC1, 0x00, 0x00, 0xBC, 0x00, 0x00, 0xB7, 0x00, 0x00, 0xB3, 0x00, 0x00, 0xAE,
      0x00, 0x00, 0xA9, 0x00, 0x00, 0xA4, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x9B, 0x00, 0x00, 0x96, 0x00,
      0x00, 0x91, 0x00, 0x00, 0x8D, 0x00, 0x00, 0x88, 0x00, 0x00, 0x83, 0x00, 0x00, 0x7F, 0x00, 0x00
    };
    static const unsigned char s_rainbow_map[256*3] =
    {
      0x00, 0x00, 0xFF, 0x00, 0x04, 0xFF, 0x00, 0x08, 0xFF, 0x00, 0x0C, 0xFE, 0x00, 0x10, 0xFF, 0x00,
      0x14, 0xFF, 0x00, 0x18, 0xFF, 0x00, 0x1C, 0xFF, 0x00, 0x20, 0xFF, 0x00, 0x24, 0xFF, 0x00, 0x28,
      0xFF, 0x00, 0x2C, 0xFE, 0x00, 0x30, 0xFF, 0x00, 0x34, 0xFF, 0x00, 0x38, 0xFF, 0x00, 0x3C, 0xFF,
      0x00, 0x40, 0xFF, 0x00, 0x44, 0xFF, 0x00, 0x48, 0xFF, 0x00, 0x4C, 0xFE, 0x00, 0x50, 0xFF, 0x00,
      0x55, 0xFF, 0x00, 0x59, 0xFF, 0x00, 0x5D, 0xFF, 0x00, 0x61, 0xFF, 0x00, 0x65, 0xFF, 0x00, 0x69,
      0xFF, 0x00, 0x6D, 0xFE, 0x00, 0x71, 0xFF, 0x00, 0x75, 0xFF, 0x00, 0x79, 0xFF, 0x00, 0x7D, 0xFF,
      0x00, 0x81, 0xFF, 0x00, 0x85, 0xFF, 0x00, 0x89, 0xFF, 0x00, 0x8D, 0xFF, 0x00, 0x91, 0xFF, 0x00,
      0x95, 0xFF, 0x00, 0x99, 0xFF, 0x00, 0x9D, 0xFF, 0x00, 0xA1, 0xFF, 0x00, 0xA5, 0xFF, 0x00, 0xAA,
      0xFF, 0x00, 0xAE, 0xFF, 0x00, 0xB2, 0xFF, 0x00, 0xB6, 0xFF, 0x00, 0xBA, 0xFF, 0x00, 0xBE, 0xFF,
      0x00, 0xC2, 0xFF, 0x00, 0xC6, 0xFF, 0x00, 0xCA, 0xFF, 0x00, 0xCE, 0xFF, 0x00, 0xD2, 0xFF, 0x00,
      0xD6, 0xFF, 0x00, 0xDA, 0xFF, 0x00, 0xDE, 0xFF, 0x00, 0xE2, 0xFF, 0x00, 0xE6, 0xFF, 0x00, 0xEA,
      0xFF, 0x00, 0xEE, 0xFF, 0x00, 0xF2, 0xFF, 0x00, 0xF6, 0xFF, 0x00, 0xFA, 0xFF, 0x00, 0xFF, 0xFF,
      0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFA, 0x00, 0xFF, 0xF6, 0x00, 0xFE, 0xF2, 0x00, 0xFF, 0xEE, 0x00,
      0xFF, 0xEA, 0x00, 0xFF, 0xE6, 0x00, 0xFF, 0xE2, 0x00, 0xFF, 0xDE, 0x00, 0xFF, 0xDA, 0x00, 0xFF,
      0xD6, 0x00, 0xFE, 0xD2, 0x00, 0xFF, 0xCE, 0x00, 0xFF, 0xCA, 0x00, 0xFF, 0xC6, 0x00, 0xFF, 0xC2,
      0x00, 0xFF, 0xBE, 0x00, 0xFF, 0xBA, 0x00, 0xFF, 0xB6, 0x00, 0xFE, 0xB2, 0x00, 0xFF, 0xAE, 0x00,
      0xFF, 0xAA, 0x00, 0xFF, 0xA5, 0x00, 0xFF, 0xA1, 0x00, 0xFF, 0x9D, 0x00, 0xFF, 0x99, 0x00, 0xFF,
      0x95, 0x00, 0xFE, 0x91, 0x00, 0xFF, 0x8D, 0x00, 0xFF, 0x89, 0x00, 0xFF, 0x85, 0x00, 0xFF, 0x81,
      0x00, 0xFF, 0x7D, 0x00, 0xFF, 0x79, 0x00, 0xFF, 0x75, 0x00, 0xFF, 0x71, 0x00, 0xFF, 0x6D, 0x00,
      0xFF, 0x69, 0x00, 0xFF, 0x65, 0x00, 0xFF, 0x61, 0x00, 0xFF, 0x5D, 0x00, 0xFF, 0x59, 0x00, 0xFF,
      0x55, 0x00, 0xFF, 0x50, 0x00, 0xFF, 0x4C, 0x00, 0xFF, 0x48, 0x00, 0xFF, 0x44, 0x00, 0xFF, 0x40,
      0x00, 0xFF, 0x3C, 0x00, 0xFF, 0x38, 0x00, 0xFF, 0x34, 0x00, 0xFF, 0x30, 0x00, 0xFF, 0x2C, 0x00,
      0xFF, 0x28, 0x00, 0xFF, 0x24, 0x00, 0xFF, 0x20, 0x00, 0xFF, 0x1C, 0x00, 0xFF, 0x18, 0x00, 0xFF,
      0x14, 0x00, 0xFF, 0x10, 0x00, 0xFF, 0x0C, 0x00, 0xFF, 0x08, 0x00, 0xFF, 0x04, 0x00, 0xFF, 0x00,
      0x00, 0xFF, 0x00, 0x04, 0xFF, 0x00, 0x08, 0xFF, 0x00, 0x0C, 0xFE, 0x00, 0x10, 0xFF, 0x00, 0x14,
      0xFF, 0x00, 0x18, 0xFF, 0x00, 0x1C, 0xFF, 0x00, 0x20, 0xFF, 0x00, 0x24, 0xFF, 0x00, 0x28, 0xFF,
      0x00, 0x2C, 0xFE, 0x00, 0x30, 0xFF, 0x00, 0x34, 0xFF, 0x00, 0x38, 0xFF, 0x00, 0x3C, 0xFF, 0x00,
      0x40, 0xFF, 0x00, 0x44, 0xFF, 0x00, 0x48, 0xFF, 0x00, 0x4C, 0xFE, 0x00, 0x50, 0xFF, 0x00, 0x55,
      0xFF, 0x00, 0x59, 0xFF, 0x00, 0x5D, 0xFF, 0x00, 0x61, 0xFF, 0x00, 0x65, 0xFF, 0x00, 0x69, 0xFF,
      0x00, 0x6D, 0xFE, 0x00, 0x71, 0xFF, 0x00, 0x75, 0xFF, 0x00, 0x79, 0xFF, 0x00, 0x7D, 0xFF, 0x00,
      0x81, 0xFF, 0x00, 0x85, 0xFF, 0x00, 0x89, 0xFF, 0x00, 0x8D, 0xFF, 0x00, 0x91, 0xFF, 0x00, 0x95,
      0xFF, 0x00, 0x99, 0xFF, 0x00, 0x9D, 0xFF, 0x00, 0xA1, 0xFF, 0x00, 0xA5, 0xFF, 0x00, 0xAA, 0xFF,
      0x00, 0xAE, 0xFF, 0x00, 0xB2, 0xFF, 0x00, 0xB6, 0xFF, 0x00, 0xBA, 0xFF, 0x00, 0xBE, 0xFF, 0x00,
      0xC2, 0xFF, 0x00, 0xC6, 0xFF, 0x00, 0xCA, 0xFF, 0x00, 0xCE, 0xFF, 0x00, 0xD2, 0xFF, 0x00, 0xD6,
      0xFF, 0x00, 0xDA, 0xFF, 0x00, 0xDE, 0xFF, 0x00, 0xE2, 0xFF, 0x00, 0xE6, 0xFF, 0x00, 0xEA, 0xFF,
      0x00, 0xEE, 0xFF, 0x00, 0xF2, 0xFF, 0x00, 0xF6, 0xFF, 0x00, 0xFA, 0xFF, 0x00, 0xFF, 0xFF, 0x00,
      0xFF, 0xFF, 0x00, 0xFF, 0xFA, 0x00, 0xFF, 0xF6, 0x00, 0xFE, 0xF2, 0x00, 0xFF, 0xEE, 0x00, 0xFF,
      0xEA, 0x00, 0xFF, 0xE6, 0x00, 0xFF, 0xE2, 0x00, 0xFF, 0xDE, 0x00, 0xFF, 0xDA, 0x00, 0xFF, 0xD6,
      0x00, 0xFE, 0xD2, 0x00, 0xFF, 0xCE, 0x00, 0xFF, 0xCA, 0x00, 0xFF, 0xC6, 0x00, 0xFF, 0xC2, 0x00,
      0xFF, 0xBE, 0x00, 0xFF, 0xBA, 0x00, 0xFF, 0xB6, 0x00, 0xFE, 0xB2, 0x00, 0xFF, 0xAE, 0x00, 0xFF,
      0xAA, 0x00, 0xFF, 0xA5, 0x00, 0xFF, 0xA1, 0x00, 0xFF, 0x9D, 0x00, 0xFF, 0x99, 0x00, 0xFF, 0x95,
      0x00, 0xFE, 0x91, 0x00, 0xFF, 0x8D, 0x00, 0xFF, 0x89, 0x00, 0xFF, 0x85, 0x00, 0xFF, 0x81, 0x00,
      0xFF, 0x7D, 0x00, 0xFF, 0x79, 0x00, 0xFF, 0x75, 0x00, 0xFF, 0x71, 0x00, 0xFF, 0x6D, 0x00, 0xFF,
      0x69, 0x00, 0xFF, 0x65, 0x00, 0xFF, 0x61, 0x00, 0xFF, 0x5D, 0x00, 0xFF, 0x59, 0x00, 0xFF, 0x55,
      0x00, 0xFF, 0x50, 0x00, 0xFF, 0x4C, 0x00, 0xFF, 0x48, 0x00, 0xFF, 0x44, 0x00, 0xFF, 0x40, 0x00,
      0xFF, 0x3C, 0x00, 0xFF, 0x38, 0x00, 0xFF, 0x34, 0x00, 0xFF, 0x30, 0x00, 0xFF, 0x2C, 0x00, 0xFF,
      0x28, 0x00, 0xFF, 0x24, 0x00, 0xFF, 0x20, 0x00, 0xFF, 0x1C, 0x00, 0xFF, 0x18, 0x00, 0xFF, 0x14,
      0x00, 0xFF, 0x10, 0x00, 0xFF, 0x0C, 0x00, 0xFF, 0x08, 0x00, 0xFF, 0x04, 0x00, 0xFF, 0x00, 0x00
    };
    static const unsigned char s_spectrum_map[256*3] =
    {
      0xFF, 0x00, 0xFF, 0xF4, 0x00, 0xFF, 0xE9, 0x00, 0xFF, 0xDF, 0x00, 0xFF, 0xD4, 0x00, 0xFF, 0xC9,
      0x00, 0xFF, 0xBF, 0x00, 0xFF, 0xB4, 0x00, 0xFF, 0xAA, 0x00, 0xFF, 0x9F, 0x00, 0xFF, 0x94, 0x00,
      0xFF, 0x8A, 0x00, 0xFF, 0x7F, 0x00, 0xFF, 0x74, 0x00, 0xFF, 0x6A, 0x00, 0xFF, 0x5F, 0x00, 0xFF,
      0x55, 0x00, 0xFF, 0x4A, 0x00, 0xFF, 0x3F, 0x00, 0xFF, 0x35, 0x00, 0xFF, 0x2A, 0x00, 0xFF, 0x1F,
      0x00, 0xFF, 0x15, 0x00, 0xFF, 0x0A, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x05,
      0xFF, 0x00, 0x0A, 0xFE, 0x00, 0x0F, 0xFF, 0x00, 0x14, 0xFF, 0x00, 0x19, 0xFF, 0x00, 0x1E, 0xFF,
      0x00, 0x23, 0xFF, 0x00, 0x28, 0xFF, 0x00, 0x2D, 0xFF, 0x00, 0x33, 0xFF, 0x00, 0x38, 0xFF, 0x00,
      0x3D, 0xFF, 0x00, 0x42, 0xFF, 0x00, 0x47, 0xFF, 0x00, 0x4C, 0xFF, 0x00, 0x51, 0xFF, 0x00, 0x56,
      0xFF, 0x00, 0x5B, 0xFF, 0x00, 0x60, 0xFF, 0x00, 0x66, 0xFF, 0x00, 0x6B, 0xFF, 0x00, 0x70, 0xFF,
      0x00, 0x75, 0xFF, 0x00, 0x7A, 0xFF, 0x00, 0x7F, 0xFF, 0x00, 0x84, 0xFF, 0x00, 0x89, 0xFF, 0x00,
      0x8E, 0xFF, 0x00, 0x93, 0xFF, 0x00, 0x99, 0xFF, 0x00, 0x9E, 0xFF, 0x00, 0xA3, 0xFF, 0x00, 0xA8,
      0xFF, 0x00, 0xAD, 0xFF, 0x00, 0xB2, 0xFF, 0x00, 0xB7, 0xFF, 0x00, 0xBC, 0xFF, 0x00, 0xC1, 0xFF,
      0x00, 0xC6, 0xFF, 0x00, 0xCC, 0xFF, 0x00, 0xD1, 0xFF, 0x00, 0xD6, 0xFF, 0x00, 0xDB, 0xFF, 0x00,
      0xE0, 0xFF, 0x00, 0xE5, 0xFF, 0x00, 0xEA, 0xFF, 0x00, 0xEF, 0xFF, 0x00, 0xF4, 0xFF, 0x00, 0xF9,
      0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xF8, 0x00, 0xFF, 0xF1, 0x00, 0xFF, 0xEA,
      0x00, 0xFF, 0xE3, 0x00, 0xFF, 0xDC, 0x00, 0xFF, 0xD5, 0x00, 0xFF, 0xCE, 0x00, 0xFF, 0xC7, 0x00,
      0xFF, 0xC0, 0x00, 0xFF, 0xBA, 0x00, 0xFF, 0xB3, 0x00, 0xFF, 0xAC, 0x00, 0xFF, 0xA5, 0x00, 0xFF,
      0x9E, 0x00, 0xFF, 0x97, 0x00, 0xFF, 0x90, 0x00, 0xFF, 0x89, 0x00, 0xFF, 0x82, 0x00, 0xFF, 0x7C,
      0x00, 0xFF, 0x75, 0x00, 0xFF, 0x6E, 0x00, 0xFF, 0x67, 0x00, 0xFF, 0x60, 0x00, 0xFF, 0x59, 0x00,
      0xFF, 0x52, 0x00, 0xFF, 0x4B, 0x00, 0xFF, 0x44, 0x00, 0xFF, 0x3E, 0x00, 0xFF, 0x37, 0x00, 0xFF,
      0x30, 0x00, 0xFF, 0x29, 0x00, 0xFF, 0x22, 0x00, 0xFF, 0x1B, 0x00, 0xFF, 0x14, 0x00, 0xFF, 0x0D,
      0x00, 0xFF, 0x06, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x06, 0xFF, 0x00, 0x0D, 0xFF, 0x00, 0x14,
      0xFF, 0x00, 0x1B, 0xFF, 0x00, 0x22, 0xFF, 0x00, 0x29, 0xFF, 0x00, 0x30, 0xFF, 0x00, 0x37, 0xFF,
      0x00, 0x3E, 0xFF, 0x00, 0x44, 0xFF, 0x00, 0x4B, 0xFF, 0x00, 0x52, 0xFF, 0x00, 0x59, 0xFF, 0x00,
      0x60, 0xFF, 0x00, 0x67, 0xFF, 0x00, 0x6E, 0xFF, 0x00, 0x75, 0xFF, 0x00, 0x7C, 0xFF, 0x00, 0x82,
      0xFF, 0x00, 0x89, 0xFF, 0x00, 0x90, 0xFF, 0x00, 0x97, 0xFF, 0x00, 0x9E, 0xFF, 0x00, 0xA5, 0xFF,
      0x00, 0xAC, 0xFF, 0x00, 0xB3, 0xFF, 0x00, 0xBA, 0xFF, 0x00, 0xC0, 0xFF, 0x00, 0xC7, 0xFF, 0x00,
      0xCE, 0xFF, 0x00, 0xD5, 0xFF, 0x00, 0xDC, 0xFF, 0x00, 0xE3, 0xFF, 0x00, 0xEA, 0xFF, 0x00, 0xF1,
      0xFF, 0x00, 0xF8, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFC, 0x00, 0xFF, 0xFA,
      0x00, 0xFF, 0xF7, 0x00, 0xFF, 0xF5, 0x00, 0xFF, 0xF2, 0x00, 0xFF, 0xF0, 0x00, 0xFF, 0xED, 0x00,
      0xFF, 0xEB, 0x00, 0xFF, 0xE8, 0x00, 0xFF, 0xE6, 0x00, 0xFF, 0xE3, 0x00, 0xFF, 0xE1, 0x00, 0xFF,
      0xDE, 0x00, 0xFF, 0xDC, 0x00, 0xFF, 0xD9, 0x00, 0xFF, 0xD7, 0x00, 0xFF, 0xD4, 0x00, 0xFF, 0xD2,
      0x00, 0xFF, 0xCF, 0x00, 0xFF, 0xCD, 0x00, 0xFE, 0xCB, 0x00, 0xFF, 0xC8, 0x00, 0xFF, 0xC6, 0x00,
      0xFE, 0xC3, 0x00, 0xFF, 0xC1, 0x00, 0xFF, 0xBE, 0x00, 0xFF, 0xBC, 0x00, 0xFF, 0xB9, 0x00, 0xFF,
      0xB7, 0x00, 0xFF, 0xB4, 0x00, 0xFF, 0xB2, 0x00, 0xFF, 0xAF, 0x00, 0xFF, 0xAD, 0x00, 0xFF, 0xAA,
      0x00, 0xFF, 0xA8, 0x00, 0xFF, 0xA5, 0x00, 0xFF, 0xA3, 0x00, 0xFF, 0xA0, 0x00, 0xFF, 0x9E, 0x00,
      0xFF, 0x9B, 0x00, 0xFE, 0x99, 0x00, 0xFF, 0x97, 0x00, 0xFF, 0x94, 0x00, 0xFF, 0x92, 0x00, 0xFF,
      0x8F, 0x00, 0xFE, 0x8D, 0x00, 0xFF, 0x8A, 0x00, 0xFF, 0x88, 0x00, 0xFE, 0x85, 0x00, 0xFF, 0x83,
      0x00, 0xFF, 0x80, 0x00, 0xFF, 0x7E, 0x00, 0xFF, 0x7B, 0x00, 0xFF, 0x79, 0x00, 0xFF, 0x76, 0x00,
      0xFF, 0x74, 0x00, 0xFF, 0x71, 0x00, 0xFF, 0x6F, 0x00, 0xFF, 0x6C, 0x00, 0xFF, 0x6A, 0x00, 0xFF,
      0x67, 0x00, 0xFF, 0x65, 0x00, 0xFF, 0x63, 0x00, 0xFF, 0x60, 0x00, 0xFF, 0x5E, 0x00, 0xFF, 0x5B,
      0x00, 0xFF, 0x59, 0x00, 0xFF, 0x56, 0x00, 0xFF, 0x54, 0x00, 0xFF, 0x51, 0x00, 0xFF, 0x4F, 0x00,
      0xFF, 0x4C, 0x00, 0xFF, 0x4A, 0x00, 0xFF, 0x47, 0x00, 0xFF, 0x45, 0x00, 0xFF, 0x42, 0x00, 0xFF,
      0x40, 0x00, 0xFF, 0x3D, 0x00, 0xFF, 0x3B, 0x00, 0xFF, 0x38, 0x00, 0xFF, 0x36, 0x00, 0xFF, 0x33,
      0x00, 0xFF, 0x31, 0x00, 0xFF, 0x2F, 0x00, 0xFF, 0x2C, 0x00, 0xFF, 0x2A, 0x00, 0xFF, 0x27, 0x00,
      0xFF, 0x25, 0x00, 0xFF, 0x22, 0x00, 0xFF, 0x20, 0x00, 0xFF, 0x1D, 0x00, 0xFF, 0x1B, 0x00, 0xFF,
      0x18, 0x00, 0xFF, 0x16, 0x00, 0xFF, 0x13, 0x00, 0xFF, 0x11, 0x00, 0xFF, 0x0E, 0x00, 0xFF, 0x0C,
      0x00, 0xFF, 0x09, 0x00, 0xFF, 0x07, 0x00, 0xFF, 0x04, 0x00, 0xFF, 0x02, 0x00, 0xFF, 0x00, 0x00
    };
    static const unsigned char s_coolwarm_map[256*3] =
    {
      0x3B, 0x4B, 0xC0, 0x3C, 0x4D, 0xC1, 0x3D, 0x4F, 0xC3, 0x3E, 0x51, 0xC4, 0x3F, 0x52, 0xC6, 0x40,
      0x54, 0xC7, 0x41, 0x56, 0xC9, 0x43, 0x58, 0xCA, 0x44, 0x59, 0xCC, 0x45, 0x5B, 0xCD, 0x46, 0x5D,
      0xCF, 0x47, 0x5F, 0xD0, 0x49, 0x60, 0xD1, 0x4A, 0x62, 0xD3, 0x4B, 0x64, 0xD4, 0x4C, 0x65, 0xD6,
      0x4D, 0x67, 0xD7, 0x4F, 0x69, 0xD8, 0x50, 0x6B, 0xD9, 0x51, 0x6C, 0xDB, 0x52, 0x6E, 0xDC, 0x54,
      0x70, 0xDD, 0x55, 0x71, 0xDE, 0x56, 0x73, 0xE0, 0x57, 0x75, 0xE1, 0x59, 0x76, 0xE2, 0x5A, 0x78,
      0xE3, 0x5B, 0x79, 0xE4, 0x5C, 0x7B, 0xE5, 0x5E, 0x7D, 0xE6, 0x5F, 0x7E, 0xE7, 0x60, 0x80, 0xE8,
      0x62, 0x81, 0xE9, 0x63, 0x83, 0xEA, 0x64, 0x85, 0xEB, 0x66, 0x86, 0xEC, 0x67, 0x88, 0xED, 0x68,
      0x89, 0xEE, 0x69, 0x8B, 0xEF, 0x6B, 0x8C, 0xF0, 0x6C, 0x8E, 0xF1, 0x6D, 0x8F, 0xF1, 0x6F, 0x91,
      0xF2, 0x70, 0x92, 0xF3, 0x72, 0x94, 0xF4, 0x73, 0x95, 0xF4, 0x74, 0x97, 0xF5, 0x76, 0x98, 0xF6,
      0x77, 0x9A, 0xF6, 0x78, 0x9B, 0xF7, 0x7A, 0x9D, 0xF8, 0x7B, 0x9E, 0xF8, 0x7C, 0xA0, 0xF9, 0x7E,
      0xA1, 0xF9, 0x7F, 0xA2, 0xFA, 0x81, 0xA4, 0xFA, 0x82, 0xA5, 0xFB, 0x83, 0xA6, 0xFB, 0x85, 0xA8,
      0xFB, 0x86, 0xA9, 0xFC, 0x87, 0xAA, 0xFC, 0x89, 0xAC, 0xFD, 0x8A, 0xAD, 0xFD, 0x8C, 0xAE, 0xFD,
      0x8D, 0xAF, 0xFD, 0x8E, 0xB1, 0xFE, 0x90, 0xB2, 0xFE, 0x91, 0xB3, 0xFE, 0x93, 0xB4, 0xFE, 0x94,
      0xB5, 0xFE, 0x95, 0xB7, 0xFE, 0x97, 0xB8, 0xFE, 0x98, 0xB9, 0xFE, 0x99, 0xBA, 0xFE, 0x9B, 0xBB,
      0xFE, 0x9C, 0xBC, 0xFE, 0x9E, 0xBD, 0xFE, 0x9F, 0xBE, 0xFE, 0xA0, 0xBF, 0xFE, 0xA2, 0xC0, 0xFE,
      0xA3, 0xC1, 0xFE, 0xA4, 0xC2, 0xFE, 0xA6, 0xC3, 0xFE, 0xA7, 0xC4, 0xFD, 0xA8, 0xC5, 0xFD, 0xAA,
      0xC6, 0xFD, 0xAB, 0xC7, 0xFD, 0xAC, 0xC8, 0xFC, 0xAE, 0xC9, 0xFC, 0xAF, 0xCA, 0xFC, 0xB0, 0xCB,
      0xFB, 0xB2, 0xCB, 0xFB, 0xB3, 0xCC, 0xFA, 0xB4, 0xCD, 0xFA, 0xB6, 0xCE, 0xF9, 0xB7, 0xCF, 0xF9,
      0xB8, 0xCF, 0xF8, 0xB9, 0xD0, 0xF8, 0xBB, 0xD1, 0xF7, 0xBC, 0xD1, 0xF7, 0xBD, 0xD2, 0xF6, 0xBF,
      0xD3, 0xF5, 0xC0, 0xD3, 0xF5, 0xC1, 0xD4, 0xF4, 0xC2, 0xD4, 0xF3, 0xC3, 0xD5, 0xF2, 0xC5, 0xD6,
      0xF2, 0xC6, 0xD6, 0xF1, 0xC7, 0xD7, 0xF0, 0xC8, 0xD7, 0xEF, 0xC9, 0xD8, 0xEE, 0xCB, 0xD8, 0xEE,
      0xCC, 0xD8, 0xED, 0xCD, 0xD9, 0xEC, 0xCE, 0xD9, 0xEB, 0xCF, 0xD9, 0xEA, 0xD0, 0xDA, 0xE9, 0xD1,
      0xDA, 0xE8, 0xD2, 0xDA, 0xE7, 0xD3, 0xDB, 0xE6, 0xD5, 0xDB, 0xE5, 0xD6, 0xDB, 0xE4, 0xD7, 0xDB,
      0xE3, 0xD8, 0xDC, 0xE1, 0xD9, 0xDC, 0xE0, 0xDA, 0xDC, 0xDF, 0xDB, 0xDC, 0xDE, 0xDC, 0xDC, 0xDD,
      0xDD, 0xDC, 0xDB, 0xDE, 0xDB, 0xDA, 0xDF, 0xDB, 0xD9, 0xE0, 0xDA, 0xD7, 0xE1, 0xDA, 0xD6, 0xE2,
      0xD9, 0xD4, 0xE3, 0xD9, 0xD3, 0xE4, 0xD8, 0xD1, 0xE5, 0xD8, 0xD0, 0xE6, 0xD7, 0xCF, 0xE7, 0xD6,
      0xCD, 0xE8, 0xD6, 0xCC, 0xE8, 0xD5, 0xCA, 0xE9, 0xD4, 0xC9, 0xEA, 0xD4, 0xC7, 0xEB, 0xD3, 0xC6,
      0xEC, 0xD2, 0xC4, 0xEC, 0xD1, 0xC3, 0xED, 0xD0, 0xC1, 0xEE, 0xD0, 0xC0, 0xEE, 0xCF, 0xBE, 0xEF,
      0xCE, 0xBD, 0xF0, 0xCD, 0xBB, 0xF0, 0xCC, 0xB9, 0xF1, 0xCB, 0xB8, 0xF1, 0xCA, 0xB6, 0xF2, 0xC9,
      0xB5, 0xF2, 0xC8, 0xB3, 0xF3, 0xC7, 0xB2, 0xF3, 0xC6, 0xB0, 0xF4, 0xC5, 0xAF, 0xF4, 0xC4, 0xAD,
      0xF4, 0xC3, 0xAB, 0xF5, 0xC2, 0xAA, 0xF5, 0xC1, 0xA8, 0xF5, 0xC0, 0xA7, 0xF6, 0xBF, 0xA5, 0xF6,
      0xBE, 0xA4, 0xF6, 0xBC, 0xA2, 0xF6, 0xBB, 0xA0, 0xF6, 0xBA, 0x9F, 0xF7, 0xB9, 0x9D, 0xF7, 0xB7,
      0x9C, 0xF7, 0xB6, 0x9A, 0xF7, 0xB5, 0x98, 0xF7, 0xB4, 0x97, 0xF7, 0xB2, 0x95, 0xF7, 0xB1, 0x94,
      0xF7, 0xB0, 0x92, 0xF7, 0xAE, 0x91, 0xF7, 0xAD, 0x8F, 0xF7, 0xAC, 0x8D, 0xF7, 0xAA, 0x8C, 0xF7,
      0xA9, 0x8A, 0xF6, 0xA7, 0x89, 0xF6, 0xA6, 0x87, 0xF6, 0xA4, 0x86, 0xF6, 0xA3, 0x84, 0xF6, 0xA1,
      0x82, 0xF5, 0xA0, 0x81, 0xF5, 0x9E, 0x7F, 0xF5, 0x9D, 0x7E, 0xF4, 0x9B, 0x7C, 0xF4, 0x9A, 0x7B,
      0xF4, 0x98, 0x79, 0xF3, 0x96, 0x78, 0xF3, 0x95, 0x76, 0xF2, 0x93, 0x75, 0xF2, 0x92, 0x73, 0xF1,
      0x90, 0x72, 0xF1, 0x8E, 0x70, 0xF0, 0x8D, 0x6E, 0xF0, 0x8B, 0x6D, 0xEF, 0x89, 0x6B, 0xEF, 0x88,
      0x6A, 0xEE, 0x86, 0x68, 0xED, 0x84, 0x67, 0xED, 0x82, 0x66, 0xEC, 0x80, 0x64, 0xEB, 0x7F, 0x63,
      0xEB, 0x7D, 0x61, 0xEA, 0x7B, 0x60, 0xE9, 0x79, 0x5E, 0xE8, 0x77, 0x5D, 0xE8, 0x76, 0x5B, 0xE7,
      0x74, 0x5A, 0xE6, 0x72, 0x59, 0xE5, 0x70, 0x57, 0xE4, 0x6E, 0x56, 0xE3, 0x6C, 0x54, 0xE2, 0x6A,
      0x53, 0xE1, 0x68, 0x52, 0xE0, 0x66, 0x50, 0xDF, 0x64, 0x4F, 0xDE, 0x62, 0x4D, 0xDD, 0x60, 0x4C,
      0xDC, 0x5E, 0x4B, 0xDB, 0x5C, 0x49, 0xDA, 0x5A, 0x48, 0xD9, 0x58, 0x47, 0xD8, 0x56, 0x45, 0xD7,
      0x54, 0x44, 0xD6, 0x52, 0x43, 0xD5, 0x50, 0x42, 0xD3, 0x4D, 0x40, 0xD2, 0x4B, 0x3F, 0xD1, 0x49,
      0x3E, 0xD0, 0x47, 0x3C, 0xCE, 0x44, 0x3B, 0xCD, 0x42, 0x3A, 0xCC, 0x40, 0x39, 0xCB, 0x3D, 0x38,
      0xC9, 0x3B, 0x36, 0xC8, 0x38, 0x35, 0xC7, 0x36, 0x34, 0xC5, 0x33, 0x33, 0xC4, 0x30, 0x32, 0xC2,
      0x2E, 0x30, 0xC1, 0x2B, 0x2F, 0xC0, 0x28, 0x2E, 0xBE, 0x25, 0x2D, 0xBD, 0x21, 0x2C, 0xBB, 0x1E,
      0x2B, 0xBA, 0x1A, 0x2A, 0xB8, 0x16, 0x29, 0xB7, 0x11, 0x28, 0xB5, 0x0B, 0x27, 0xB4, 0x03, 0x26
    };

    const unsigned char *map = s_grayscale_map;
    switch (inColorMap)
    {
      case COLORMAP_GRAYSCALE:
        map = s_grayscale_map;
        break;
      case COLORMAP_JET:
        map = s_jet_map;
        break;
      case COLORMAP_RAINBOW:
        map = s_rainbow_map;
        break;
      case COLORMAP_SEPECTRUM:
        map = s_spectrum_map;
        break;
      case COLORMAP_COOLWARM:
        map = s_coolwarm_map;
        break;
    }
    return map;
  }
};
#endif  // #ifdef IMAGE_WINDOW_H
