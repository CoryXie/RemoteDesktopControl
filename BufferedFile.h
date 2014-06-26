#pragma once

#include <iostream>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h> 

#define CREAT _O_CREAT
#define TRUNC _O_TRUNC
#define RDONLY _O_RDONLY
#define WRONLY _O_WRONLY
#define RDWR _O_RDWR
#define BINARY _O_BINARY

// Optional call back functions for when the file is opened and closed
typedef void (__stdcall * pFNOpenFile)(int hFile);
typedef void (__stdcall * pFNCloseFile)(int hFile);

// Memory Allocator Class
class CBufferedMemory
{
public:
	CBufferedMemory();
	virtual ~CBufferedMemory();

public:
	bool AllocBuffer(unsigned long ulBytes);
	bool ReallocBuffer(unsigned long ulBytes,unsigned long ulPos);
	void FreeBuffer();
	const char * GetBuffer() {return (const char *)m_lpBuffer;}

protected:
	char * Alloc(unsigned long ulBytes);
	void Free(char * pbMem = 0);

protected:
	char * m_lpBuffer;
	unsigned long m_ulBytes;
};

// Buffered I/O File Class
class CBufferedFile : virtual public CBufferedMemory
{
public:
	CBufferedFile(const char * pszFilename = 0,unsigned int uiInitial = 1024,unsigned int uiMaxBufferSize = 65536,unsigned int uiGrowBy = 4096,int iMode = BINARY|RDONLY,pFNOpenFile lpfnOpenFile = 0,pFNCloseFile lpfnCloseFile = 0);
	virtual ~CBufferedFile();

public:
	void SetFilename(const char * pszFilename = 0);
	void SetMode(int iMode = BINARY|RDONLY);
	void Open(const char * pszFilename = 0,int iMode = 0);
	void Close();
	unsigned long GetLength();
	unsigned long Rewind();
	unsigned long FastForward();
	bool MovePosition(long lBytes,bool bRelative);
	void ResetBuffer();

public:
	template <typename TYPE> CBufferedFile & operator >> (TYPE & rhs);
	unsigned int Read(const void * lpBuffer,unsigned int uiCount);
	template <typename TYPE> CBufferedFile & operator << (const TYPE & rhs);
	unsigned int Write(const void * lpBuffer,unsigned int uiCount);

public:
	bool IsOpen() const {return m_bOpened;}
	operator bool () const {return !m_bEOF;}
	bool IsEOF() const {return m_bEOF;}
	bool IsErr() const {return m_bErr;}

protected:
	bool IsReading() const {return ((m_iMode & WRONLY) == 0) || ((m_iMode & RDWR) == RDWR);}
	bool IsWriting() const {return ((m_iMode & WRONLY) == WRONLY) || ((m_iMode & RDWR) == RDWR);}

protected:
	int Flush();
	void FillBuffer();

protected:
	// File and Access mode
	char * m_pszFilename;
	int m_hFile,m_iMode;

	// Flags
	bool m_bOpened,m_bEOF,m_bErr;

	// File position
	long m_lFilePosBeg,m_lFilePosEnd;

	// The length of the file
	unsigned long m_ulLength;

	// Buffering
	unsigned int m_uiDefBufferSize,m_uiBufferPosition,m_uiBufferSize,m_uiMaxBufferSize,m_uiGrowBytes;

	// Read buffering
	bool m_bReadExceeded;
	unsigned int m_uiEOBPosition;

	// Write buffering
	bool m_bWriteExceeded,m_bOpenWriteFile;

protected:
   pFNOpenFile m_lpfnOpenFile;
   pFNCloseFile m_lpfnCloseFile;
};

// Read data (overloaded right shift)
template <typename TYPE> CBufferedFile & CBufferedFile::operator >> (TYPE & rhs)
{
	// Do a buffered read
	memset(&rhs,0,sizeof(TYPE));
	Read(&rhs,sizeof(TYPE));
	return *this;
}

// Write data (overloaded left shift)
template <typename TYPE> CBufferedFile & CBufferedFile::operator << (const TYPE & rhs)
{
	Write(&rhs,sizeof(TYPE));
	return *this;
}

// Read Buffered I/O File Class
class CReadBufferedFile : virtual public CBufferedFile
{
public:
	CReadBufferedFile(const char * pszFilename = 0,unsigned int uiInitial = 1024,unsigned int uiMaxBufferSize = 65536,unsigned int uiGrowBy = 4096,pFNOpenFile lpfnOpenFile = 0,pFNCloseFile lpfnCloseFile = 0);
	void Open(const char * pszFilename = 0);
};

// Write Buffered I/O File Class
class CWriteBufferedFile : virtual public CBufferedFile
{
public:
	CWriteBufferedFile(const char * pszFilename = 0,unsigned int uiInitial = 1024,unsigned int uiMaxBufferSize = 65536,unsigned int uiGrowBy = 4096,pFNOpenFile lpfnOpenFile = 0,pFNCloseFile lpfnCloseFile = 0);
	void Open(const char * pszFilename = 0);
};

// Write Read and Write Buffered I/O File Class
class CReadWriteBufferedFile : virtual public CReadBufferedFile, virtual public CWriteBufferedFile
{
public:
	CReadWriteBufferedFile(const char * pszFilename = 0,unsigned int uiInitial = 1024,unsigned int uiMaxBufferSize = 65536,unsigned int uiGrowBy = 4096,pFNOpenFile lpfnOpenFile = 0,pFNCloseFile lpfnCloseFile = 0);
	void Open(const char * pszFilename = 0);
	void TruncOpen(const char * pszFilename = 0);
};