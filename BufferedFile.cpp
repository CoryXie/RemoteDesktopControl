#include "stdafx.h"
#include "BufferedFile.h"
using namespace std;

// Memory Allocator Class
CBufferedMemory::CBufferedMemory() : m_lpBuffer(0), m_ulBytes(0)
{
}

CBufferedMemory::~CBufferedMemory()
{
	Free();
}

bool CBufferedMemory::AllocBuffer(unsigned long ulBytes)
{
	if (m_lpBuffer)
		Free();
	m_lpBuffer = Alloc(ulBytes);
	if (m_lpBuffer)
	{
		m_ulBytes = ulBytes;
		return true;
	}
	return false;
}

// Reallocate the buffer memory
bool CBufferedMemory::ReallocBuffer(unsigned long ulBytes,unsigned long ulPos)
{
	// Allocate a new buffer
	char * pMem = Alloc(ulBytes);
	if (pMem)
	{
		// Copy the current buffer
		ulPos = (ulPos < m_ulBytes ? ulPos : m_ulBytes);
		unsigned long ulCopy = (ulBytes < ulPos ? ulBytes : ulPos);
		memcpy(pMem,m_lpBuffer,ulCopy);

		// Free the current memory
		Free();

		// Reassign
		m_lpBuffer = pMem;
		m_ulBytes = ulBytes;
		return true;
	}
	return false;
}

void CBufferedMemory::FreeBuffer()
{
	Free();
}

// Allocate new buffer memory
char * CBufferedMemory::Alloc(unsigned long ulBytes)
{
	try
	{
		return new char[ulBytes];
	}
	catch (...)
	{
	}
	return 0;
}

// Free the buffer memory
void CBufferedMemory::Free(char * lpMem)
{
	if (lpMem)
	{
		delete [] lpMem;
		lpMem = 0;
	}
	else if (m_lpBuffer)
	{
		delete [] m_lpBuffer;
		m_lpBuffer = 0;
		m_ulBytes = 0;
	}
}

// Buffered I/O File Class
// Constructor
CBufferedFile::CBufferedFile(const char * pszFilename,unsigned int uiInitial,unsigned int uiMaxBufferSize,unsigned int uiGrowBy,int iMode,pFNOpenFile lpfnOpenFile,pFNCloseFile lpfnCloseFile) : 
	CBufferedMemory(),
	m_pszFilename(0),m_lpfnOpenFile(lpfnOpenFile),m_lpfnCloseFile(lpfnCloseFile),
	m_hFile(0),m_bOpened(false),m_bEOF(true),m_bErr(false),
	m_bWriteExceeded(false),m_bOpenWriteFile(false),m_bReadExceeded(false),m_uiEOBPosition(0),
	m_uiMaxBufferSize(uiMaxBufferSize),m_uiGrowBytes(uiGrowBy),m_uiBufferPosition(0),m_uiBufferSize(0),
	m_ulLength(0),m_lFilePosBeg(0),m_lFilePosEnd(0)
{
	// Set the mode
	SetMode(iMode);

	// Calculate the initial buffer (don't allocate until 1st use)
	m_uiDefBufferSize = (uiInitial < m_uiMaxBufferSize ? uiInitial : m_uiMaxBufferSize);

	// Store the filename
	if (pszFilename)
		m_pszFilename = _strdup(pszFilename);
}

// Destructor
CBufferedFile::~CBufferedFile()
{
	// Flush the buffer in case the file was never opened
	Flush();

	// Close the file
	Close();

	// Free the buffer for I/O
	ResetBuffer();

	// Free the filename
	if (m_pszFilename)
	{
		free(m_pszFilename);
		m_pszFilename = 0;
	}
}

// Read buffered I/O
unsigned int CBufferedFile::Read(const void * lpBuffer,unsigned int uiCount)
{
	if (!IsReading())
		return 0;
	unsigned char * lpBuf = (unsigned char * )lpBuffer;
	unsigned int uiReadLeft = uiCount;

	// Test for allocating the buffer for the first time
	if (m_uiBufferSize == 0)
	{
		// Allocate the buffer for the first time
		bool bAlloc = AllocBuffer(m_uiDefBufferSize);

		// Test for allocated memory
		if (!bAlloc)
		{
			// Can't allocate memory, read directly to ouput unless error
			int iRead = _read(m_hFile,lpBuf,uiCount);
			if (iRead <= 0)
			{
				if (iRead == 0)
					m_bEOF = true;
				else
					m_bErr = true;
				return 0;
			}
			return (unsigned int)iRead;
		}

		// Set the buffer sizes
		m_uiBufferSize = m_uiDefBufferSize;
	}

	// Main loop to read data into the return buffer
	do
	{
		// Test the read buffer size for maximum capacity
		if (m_uiBufferPosition == m_uiEOBPosition)
		{
			// Flush the current write buffer
			Flush();

			// Only read more if the EOF hasn't occurred
			if (!IsEOF())
			{
				// Test for growing the read buffer beyond the current maximum capacity
				if (!m_bReadExceeded && m_uiEOBPosition)
				{
					// Calculate new buffer size
					unsigned int uiBytes = m_uiBufferSize + m_uiGrowBytes;
					if (uiBytes >= m_uiMaxBufferSize)
						uiBytes = m_uiMaxBufferSize;

					// Test for growing the buffer
					if (uiBytes != m_uiBufferSize)
					{
						// Grow the buffer
						bool bRealloc = ReallocBuffer(uiBytes,m_uiEOBPosition);
						if (bRealloc)
						{
							m_uiDefBufferSize = uiBytes;
							m_uiBufferSize = uiBytes;
						}
					}
					else
						m_bReadExceeded = true;
				}

				// Reset the maximum buffer capacity
				m_uiEOBPosition = 0;

				// Get the current position
				m_lFilePosBeg = _lseek(m_hFile,0L,SEEK_CUR);
				m_lFilePosEnd = m_lFilePosBeg;

				// Read upto the maximum amount available to the buffer
				unsigned int uiRead = _read(m_hFile,m_lpBuffer,m_uiBufferSize);

				// Update the read buffer position
				if (uiRead > 0)
				{
					// Reset the current buffer position
					m_uiBufferPosition = 0;

					// Update the maximum buffer capacity
					m_uiEOBPosition = uiRead;

					// Update the actual buffer size if there wasn't enough data to fill it
					if (uiRead < m_uiBufferSize)
						m_uiBufferSize = uiRead;

					// Test for the condition where more was requested than available
					if (uiRead < uiReadLeft)
					{
						uiCount = uiRead;
						uiReadLeft = uiRead;
						m_bEOF = true;
					}
				}
				else
				{
					// End of file
					m_bEOF = true;
					return 0;
				}

				// Get the current position
				m_lFilePosEnd = _lseek(m_hFile,0L,SEEK_CUR);
			}
		}

		// Compute output amount
		unsigned int uiReadAmount = m_uiBufferSize - m_uiBufferPosition;

		// Test for conditions where reading is attempted when the EOF has been reached
		if (uiReadAmount == 0)
			return 0;
		unsigned int uiWriteAmount = (uiReadLeft < uiReadAmount ? uiReadLeft : uiReadAmount);
		
		// Fill in the output buffer
		memcpy(lpBuf,m_lpBuffer + m_uiBufferPosition,uiWriteAmount);

		// Update the output buffer
		lpBuf += uiWriteAmount;

		// Update buffer positions
		m_uiBufferPosition += uiWriteAmount;
		uiReadLeft -= uiWriteAmount;
	} while (uiReadLeft);

	// Return the amount read
	return uiCount;
}

// Write buffered I/O
unsigned int CBufferedFile::Write(const void * lpBuffer,unsigned int uiCount)
{
	if (!IsWriting())
		return 0;
	unsigned char * lpBuf = (unsigned char * )lpBuffer;
	unsigned int uiLeft = uiCount;

	// Test for allocating the buffer for the first time
	if (m_uiBufferSize == 0)
	{
		// For read/write files, the buffer needs to be initialized from the file
		if (IsReading())
		{
			// Allocate by reading the block size
			FillBuffer();
		}
		else
		{
			// Allocate the buffer for the first time
			bool bAlloc = AllocBuffer(m_uiDefBufferSize);

			// Update the buffer sizes
			if (bAlloc)
				m_uiBufferSize = m_uiDefBufferSize;
		}

		// Test for allocated memory
		if (m_lpBuffer == 0)
		{
			// Can't allocate memory, write directly to ouput file unless error
			return _write(m_hFile,lpBuf,uiCount);
		}
	}

	// Process the output
	do
	{
		// Test for buffer overflow
		unsigned int uiAmount = uiLeft + m_uiBufferPosition;
		if (uiAmount > m_uiBufferSize)
		{
			// If the file is not open test for growing the buffer
			if (!IsOpen() || (m_uiBufferSize < m_uiMaxBufferSize))
			{
				// Test the buffer size for growth
				unsigned int uiLeftOver = 0;
				unsigned int uiBytes = m_uiBufferSize + m_uiGrowBytes;

				// Test to see if we are outputting more than we are growing the buffer by
				if (uiBytes < uiAmount)
					uiBytes = uiAmount;

				// Test to see if we are growing the buffer past the maximum capacity
				if (uiBytes >= m_uiMaxBufferSize)
				{
					if (m_bWriteExceeded)
						m_bOpenWriteFile = true;
					else
					{
						uiBytes = m_uiMaxBufferSize;
						if (uiAmount > uiBytes)
							uiLeftOver = uiAmount - uiBytes;
						m_bWriteExceeded = true;
					}
				}

				// Go to file mode if the maximum buffer has been exceeded otherwise grow the buffer some more
				if (m_bOpenWriteFile)
				{
					// The file may have been opened already
					if (!IsOpen())
						Open();
				}
				else
				{
					// Grow the buffer
					bool bRealloc = ReallocBuffer(uiBytes,m_uiBufferPosition);
					if (bRealloc)
					{
						m_uiDefBufferSize = uiBytes;
						m_uiBufferSize = uiBytes;

						// Open the temp file if there would be leftover data after the memory write
						if (uiLeftOver > 0)
						{
							// The file may have been opened already
							if (!IsOpen())
								Open();
						}
					}
					else
					{
						// Failed to reallocate, continue using existing memory block, go to file mode
						m_bWriteExceeded = true;
						Open();
					}
				}
			}

			// If we went to file mode then flush the current buffer to disk
			if (IsOpen())
			{
				// For read/write files, the buffer needs to be initialized from the file
				if (IsReading())
				{
					// Allocate by reading the block size, Flush occurs before read
					FillBuffer();
				}
				else
				{
					// Flush the current contents to disk
					Flush();
				}
			}
		}

		// Calculate the amount to write
		unsigned int uiRemBuf = m_uiDefBufferSize - m_uiBufferPosition;
		unsigned int uiWrite = (uiLeft < uiRemBuf ? uiLeft : uiRemBuf);

		// Write to the buffer
		memcpy(m_lpBuffer + m_uiBufferPosition,lpBuf,uiWrite);

		// Update the input buffer position
		lpBuf += uiWrite;

		// Update the output buffer position
		m_uiBufferPosition += uiWrite;
		uiLeft -= uiWrite;
	} while (uiLeft);
	return uiCount;
}

// Set a filename or generate a temporary name
void CBufferedFile::SetFilename(const char * pszFilename)
{
	if (pszFilename)
	{
		if (m_pszFilename)
			free(m_pszFilename);
		m_pszFilename = _strdup(pszFilename);
	}

	if (!m_pszFilename)
		m_pszFilename = _tempnam(0,0);
}

// Set the file access mode
void CBufferedFile::SetMode(int iMode)
{
	if (!IsOpen())
	{
		// Set the file access mode
		m_iMode = iMode;
	}
}

// Open the file for reading or writing
void CBufferedFile::Open(const char * pszFilename,int iMode)
{
	// Close any opened files
	Close();

	// Set the internal file name
	if (pszFilename)
	{
		if (m_pszFilename)
			free(m_pszFilename);
		m_pszFilename = _strdup(pszFilename);
	}

	// Generate a temporary file name
	if (!m_pszFilename)
		m_pszFilename = _tempnam(0,0);

	// Set the file access and creation flags
	if (iMode)
		m_iMode = iMode;

	// Open the file
	m_hFile = _open(m_pszFilename,m_iMode,_S_IREAD|_S_IWRITE);
	if (m_hFile > 0)
	{
		// Update the file status flags
		m_bOpened = true;
		m_bEOF = false;
		m_bErr = false;

		try
		{
			// Callback function for when file is opened
			if (m_lpfnOpenFile)
				(m_lpfnOpenFile)(m_hFile);
		}
		catch(...)
		{
		}

		// Get the length of the file
		if (IsReading())
			GetLength();
	}
}

// Close the file and flush the write buffer
void CBufferedFile::Close()
{
	// Close any opened files
	if (IsOpen())
	{
		// Flush the buffer to file
		Flush();

		try
		{
			// Callback function for when file is close
			if (m_lpfnCloseFile)
				(m_lpfnCloseFile)(m_hFile);
		}
		catch (...)
		{
		}

		// Close the file handle
		_close(m_hFile);
	}

	// Reset the indicators
	m_bEOF = true;
	m_bErr = false;
	m_bOpened = false;

	// Reset the file length
	m_ulLength = 0;

	// Reset the open write file flag
	m_bOpenWriteFile = false;
}

// Get the length of the file
unsigned long CBufferedFile::GetLength()
{
	if (IsOpen() && !IsErr())
	{
		// O/S function for low level file handles
		m_ulLength = _filelength(m_hFile);
	}

	if (IsWriting())
		m_ulLength += m_uiBufferPosition;

	return m_ulLength;
}

// Rewind the file to the beginning
unsigned long CBufferedFile::Rewind()
{
	// Rewind the file
	unsigned long ret = 0L;
	if (IsOpen() && !IsErr())
	{
		// Reset the buffers and indices
		ResetBuffer();

		// Seek to the beginning of the file
		ret = _lseek(m_hFile,0L,SEEK_SET);
		if (ret == 0)
		{
			// Reset the EOF flag
			if (IsEOF())
				m_bEOF = false;

			// Cause the buffer to be read
			FillBuffer();
		}
	}
	return ret;
}

// Fast forward to the end of the file
unsigned long CBufferedFile::FastForward()
{
	unsigned long ret = 0L;
	if (IsOpen() && !IsErr())
	{
		// Reset the EOF flag
		if (IsEOF())
			m_bEOF = false;

		// Seek to the end of the file
		long ret = _lseek(m_hFile,0L,SEEK_END);
		if (ret == -1L)
			return 0;

		// Reset the buffers and indices
		ResetBuffer();

		// Rewind the file the amount bytes that the buffer can hold
		long lBufferSize = m_uiDefBufferSize;
		ret = _lseek(m_hFile,-lBufferSize,SEEK_CUR);

		// Cause the buffer to be read
		FillBuffer();

		// Set the EOF flag
		m_bEOF = true;

		// Move the read header to the EOF
		m_uiBufferPosition = m_uiDefBufferSize;
	}
	return ret;
}

// Move the virtual read position either relative to the current position or absolutely using bytes as the position
bool CBufferedFile::MovePosition(long lBytes,bool bRelative)
{
	if (IsOpen() && !IsErr())
	{
		// Test for a relative read or an absolute read
		if (bRelative)
		{
			// Test if the number of bytes is going to push us outside the boundary of the buffer
			long lUnreadPosition = (long)m_uiBufferPosition;
			long lReadBufferSize = (long)m_uiBufferSize;
			long lNewPos = lUnreadPosition + lBytes;

			// Test for a Buffer page fault to need to read a new buffer
			if (lNewPos < 0 || lNewPos >= lReadBufferSize)
			{
				// Reset the buffer
				ResetBuffer();

				// Get the virtual file position (the actual position is at the end of the block in memory)
				long lExtra = lReadBufferSize - lUnreadPosition;
				long lFilePos = m_lFilePosEnd - lExtra;

				// Offset the current position with the number of bytes to move
				long lNewFilePos = lFilePos + lBytes;

				// Test for moving beyond the BOF or EOF
				if (lNewFilePos > (long)m_ulLength || lNewFilePos < 0)
					return false;
				else
				{
					// Seek to the new position
					long lActualBytes = lBytes - lExtra;
					lFilePos = _lseek(m_hFile,lActualBytes,SEEK_CUR);

					// Reset the EOF flag
					if (IsEOF() && lActualBytes < 0)
						m_bEOF = false;

					// Cause the buffer to be read
					FillBuffer();
				}
			}
			else
			{
				// Reset the EOF flag
				if (IsEOF() && (unsigned int)lNewPos < m_uiBufferSize)
					m_bEOF = false;

				// Set the new buffer position
				m_uiBufferPosition = lNewPos;
			}
		}
		else
		{
			// Move the position to the absolute file position, unless it is outside of the file
			if (!m_ulLength || lBytes < 0 || (unsigned long)lBytes > (m_ulLength - 1))
				return false;

			// Test if the new position is still within the current buffer
			if (lBytes >= m_lFilePosBeg && lBytes < m_lFilePosEnd)
			{
				// Do a relative read instead
				m_uiBufferPosition = lBytes - m_lFilePosBeg;
				return true;
			}

			// Move to the absolute position
			_lseek(m_hFile,lBytes,SEEK_SET);

			// Reset the buffer
			ResetBuffer();

			// Reset the EOF flag
			if (IsEOF())
				m_bEOF = false;

			// Cause the buffer to be read
			FillBuffer();
		}
	}
	return true;
}

// Cause the buffer to be filled by a read
void CBufferedFile::FillBuffer()
{
	// Cause the buffer to be read
	unsigned char tmp;
	Read(&tmp,sizeof(tmp));

	// Set the new buffer position
	m_uiBufferPosition = 0;
}

// Reset the buffer by freeing it and resetting the indices
void CBufferedFile::ResetBuffer()
{
	// Free the buffer for I/O
	Free();

	// Reset the buffer positions
	m_uiEOBPosition = 0;
	m_uiBufferPosition = 0;
	m_uiBufferSize = 0;
}

// Flush the buffer to file
int CBufferedFile::Flush()
{
	int ret = 0;
	if (IsWriting() && m_uiBufferPosition)
	{
		if (!IsOpen())
			Open();
		if (IsOpen() && !IsErr())
		{
			// Move to the beginning of the buffers disk positon
			if (IsReading() && m_lFilePosEnd > m_lFilePosBeg)
				_lseek(m_hFile,m_lFilePosBeg,SEEK_SET);
			
			// Write the buffer to disk
			ret = _write(m_hFile,m_lpBuffer,m_uiBufferPosition);

			// Reset the buffer position
			m_uiBufferPosition = 0;
		}
	}
	return ret;
}

// Read Buffered I/O File Class
CReadBufferedFile::CReadBufferedFile(const char * pszFilename,unsigned int uiInitial,unsigned int uiMaxBufferSize,unsigned int uiGrowBy,pFNOpenFile lpfnOpenFile,pFNCloseFile lpfnCloseFile) : 
	CBufferedFile(pszFilename,uiInitial,uiMaxBufferSize,uiGrowBy,BINARY|RDONLY,lpfnOpenFile,lpfnCloseFile)
{
}

void CReadBufferedFile::Open(const char * pszFilename)
{
	CBufferedFile::Open(pszFilename);
}

// Write Buffered I/O File Class
CWriteBufferedFile::CWriteBufferedFile(const char * pszFilename,unsigned int uiInitial,unsigned int uiMaxBufferSize,unsigned int uiGrowBy,pFNOpenFile lpfnOpenFile,pFNCloseFile lpfnCloseFile) : 
	CBufferedFile(pszFilename,uiInitial,uiMaxBufferSize,uiGrowBy,CREAT|TRUNC|WRONLY|BINARY,lpfnOpenFile,lpfnCloseFile)
{
}

void CWriteBufferedFile::Open(const char * pszFilename)
{
	CBufferedFile::Open(pszFilename);
}

// Write Read and Write Buffered I/O File Class
CReadWriteBufferedFile::CReadWriteBufferedFile(const char * pszFilename,unsigned int uiInitial,unsigned int uiMaxBufferSize,unsigned int uiGrowBy,pFNOpenFile lpfnOpenFile,pFNCloseFile lpfnCloseFile) : 
	CBufferedFile(pszFilename,uiInitial,uiMaxBufferSize,uiGrowBy,BINARY|RDWR,lpfnOpenFile,lpfnCloseFile)
{
}

void CReadWriteBufferedFile::Open(const char * pszFilename)
{
	CBufferedFile::Open(pszFilename);
}

void CReadWriteBufferedFile::TruncOpen(const char * pszFilename)
{
	CBufferedFile::Open(pszFilename,CREAT|TRUNC|RDWR|BINARY);
}