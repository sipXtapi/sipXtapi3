//  
// Copyright (C) 2006 SIPez LLC. 
// Licensed to SIPfoundry under a Contributor Agreement. 
//
// Copyright (C) 2004-2006 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
//
// Copyright (C) 2004-2006 Pingtel Corp.  All rights reserved.
// Licensed to SIPfoundry under a Contributor Agreement.
//
// $$
///////////////////////////////////////////////////////////////////////////////

#ifndef DISABLE_STREAM_PLAYER // [

// SYSTEM INCLUDES
#include <assert.h>

// APPLICATION INCLUDES
#include "os/OsIntTypes.h"
#include "os/OsSysLog.h"
#include "mp/StreamWAVFormatDecoder.h"
#include "mp/StreamDataSource.h"
#include "mp/StreamBufferDataSource.h"
#include "mp/MpTypes.h"
#include "mp/MpAudioUtils.h"


// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// DEFINES
#define NUM_SAMPLES 80
#define DESIRED_SAMPLE_RATE 8000

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
StreamWAVFormatDecoder::StreamWAVFormatDecoder(StreamDataSource* pDataSource)
   : StreamQueueingFormatDecoder(pDataSource, 1600)
   , OsTask("WAVDecoder-%d")
   , mSemExited(OsBSem::Q_PRIORITY, OsBSem::FULL)
{
}


// Destructor
StreamWAVFormatDecoder::~StreamWAVFormatDecoder()
{
}

/* ============================ MANIPULATORS ============================== */

// Initializes the decoder
OsStatus StreamWAVFormatDecoder::init()
{
   return OS_SUCCESS ;
}


// Frees all resources consumed by the decoder
OsStatus StreamWAVFormatDecoder::free()
{
   return OS_SUCCESS ;
}


// Begins decoding
OsStatus StreamWAVFormatDecoder::begin()
{
   mbEnd = FALSE ;

   mSemExited.acquire() ;
   fireEvent(DecodingStartedEvent) ;
   if (start() == FALSE)
   {
      syslog(FAC_STREAMING, PRI_CRIT, "Failed to create thread for StreamWAVFormatDecoder") ;

      // If we fail to create the thread, send out failure events
      // and clean up
      mbEnd = TRUE ;
      fireEvent(DecodingErrorEvent) ;
      fireEvent(DecodingCompletedEvent) ;
      mSemExited.release() ;
   }

   return OS_SUCCESS ;
}


// Ends decoding
OsStatus StreamWAVFormatDecoder::end()
{      
   mbEnd = TRUE ;

   // Interrupt any inprocess reads/seeks.  This speeds up the end.
   StreamDataSource* pSrc = getDataSource() ;
   if (pSrc != NULL)
   {
       pSrc->interrupt() ;
   }
   
   // Draw the decoded queue
   drain() ;

   // Wait for the run method to exit.
   mSemExited.acquire() ;

   // Draw the decoded queue again to verify that nothing is left.
   drain() ;
   
   mSemExited.release() ;

   return OS_SUCCESS ;
}


/* ============================ ACCESSORS ================================= */

// Renders a string describing this decoder.  
OsStatus StreamWAVFormatDecoder::toString(UtlString& string)
{
   string.append("WAV") ;

   return OS_SUCCESS ;
}

/* ============================ INQUIRY =================================== */


// Gets the decoding status.
UtlBoolean StreamWAVFormatDecoder::isDecoding()
{
   return (isStarted() || isShuttingDown());
}


// Determines if this is a valid decoder given the associated data source.
UtlBoolean StreamWAVFormatDecoder::validDecoder()
{
   UtlBoolean bRC = FALSE ;
   
   StreamDataSource* pSrc = getDataSource() ;
   if (pSrc != NULL)
   {
      WAVChunkID id ; 
      int read = 0 ;
      if (pSrc->peek((char*) &id, sizeof(WAVChunkID), read) == OS_SUCCESS)
      {
         if (memcmp(id.ckID, "RIFF", 4) == 0)
         {
            bRC = TRUE ;
         }
         else
         {
            syslog(FAC_STREAMING, PRI_ERR, "StreamWAVFormatDecoder::validDecoder (RIFF not detected.)");
         }
      }
   }
   return bRC  ;
}


/* //////////////////////////// PROTECTED ///////////////////////////////// */

// Copy constructor (not supported)
StreamWAVFormatDecoder::StreamWAVFormatDecoder(const StreamWAVFormatDecoder& rStreamWAVFormatDecoder)
   : StreamQueueingFormatDecoder(NULL, 1600)
   , mSemExited(OsBSem::Q_PRIORITY, OsBSem::FULL)
{
   assert(FALSE) ;
}

// Assignment operator (not supported)
StreamWAVFormatDecoder& 
StreamWAVFormatDecoder::operator=(const StreamWAVFormatDecoder& rhs)
{

   if (this == &rhs)            // handle the assignment to self case
      return *this;

   return *this;
}

/* FIXME: We need to read a number of samples appropriate to being converted to
 * 80 16-bit signed mono samples at 8kHz. (That is, we want 1/100 seconds
 * worth.) For 16-bit signed stereo at 44.1kHz, for example, this is 882
 * samples, or 1764 bytes. Then the algorithm below will work properly. For now
 * it only works properly when the input is "similar enough" to the output that
 * it accidentally doesn't distort the audio. */

// Thread entry point
int StreamWAVFormatDecoder::run(void* pArgs)
{
   int iSamplesInOutBuffer = 0;
   MpAudioSample partialFrame[80] ;
   int nSamplesPartialFrame = 0;   
   int numOutSamples = 0;
   int iDataLength ;

   //used if the files are aLaw or uLaw encoded
   InitG711Tables();
 
   StreamDataSource* pSrc = getDataSource() ;

   if (pSrc != NULL)
   {
      // pSrc->open() ;
      while (!mbEnd && nextDataChunk(iDataLength))
      {
         //we really want 80 SAMPLES not 80 bytes
         unsigned char  InBuffer[NUM_SAMPLES*2] ;
         MpAudioSample OutBuffer[4000] ;  //make room for lots of decompression
         
         memset(&OutBuffer,0,sizeof(OutBuffer));
         iSamplesInOutBuffer = 0;
         
         while ((iDataLength > 0) && !mbEnd)
         {
            int iRead = 0;

            UtlBoolean retval = OS_INVALID;

            if (mFormatChunk.formatTag == 1 && mFormatChunk.nBitsPerSample == 8) //8bit unsigned
            {
                //we need to read 80 samples
                iRead = __min(iDataLength, NUM_SAMPLES);
                 
                retval = (pSrc->read((char *)InBuffer, iRead, iRead) == OS_SUCCESS);
                
                //now convert to 16bit unsigned, which is what we use internally
                ConvertUnsigned8ToSigned16(InBuffer,OutBuffer,iRead);
                numOutSamples = iRead;
            }
            else
            if (mFormatChunk.formatTag == 1 && mFormatChunk.nBitsPerSample == 16) //16 bit signed
            {
                iRead = __min(iDataLength, NUM_SAMPLES*2);
                
                //just read in the data, because it's the format we need
                retval = (pSrc->read((char *)OutBuffer, iRead, iRead) == OS_SUCCESS);
                numOutSamples = iRead/2;
            }
            else
            if (mFormatChunk.formatTag == 6 || mFormatChunk.formatTag == 7) //16 bit signed
            {
                //we need to read 80 samples
                iRead = __min(iDataLength, NUM_SAMPLES);
                 
                retval = (pSrc->read((char *)OutBuffer, iRead, iRead) == OS_SUCCESS);
                //no conversion to 16bit will take place because we need to decompress this
            }
            else
            {
                syslog(FAC_STREAMING, PRI_ERR, "StreamWAVFormatDecoder::run Unsupport bit per MpAudioSample rate!");
            }

            iDataLength -= iRead;

            if (retval == OS_SUCCESS)
            {
                int bytes;
               switch (mFormatChunk.formatTag)
               {
                  case 1:     // PCM                     
                      //NO CONVERSION NEEDED
                        break ;
                  case 6:     // G711 alaw
                     bytes = DecompressG711ALaw(OutBuffer, iRead);
                     numOutSamples = iRead;
                     break ;
                  case 7:     // G711 ulaw
                     bytes = DecompressG711MuLaw(OutBuffer,iRead);
                     numOutSamples = iRead;
                     break ;
                }


                //we now should have a buffer filled with Samples, not bytes
                
                int numBytes = numOutSamples * sizeof(MpAudioSample);
                
                //next we check if the sound file is stereo...at this point in our lives
                //we only want to support mono
                //takes bytes in and gets bytes out.  NOT samples
                if (mFormatChunk.nChannels > 1)
                {
                    numBytes = mergeChannels((char *)OutBuffer, numBytes, mFormatChunk.nChannels);
                    
                    //now calculate how many MpAudioSample we have
                    numOutSamples = numBytes/sizeof(MpAudioSample);
                }
                
                //in the next fucntion we must pass bytes, NOT samples as second param
                numBytes = reSample((char *)OutBuffer, numBytes, mFormatChunk.nSamplesPerSec, DESIRED_SAMPLE_RATE);
                
                //now calculate how many MpAudioSample we have
                numOutSamples = numBytes/sizeof(MpAudioSample);
                
                //this next part will buffer the samples if under 80 samples
                if (numOutSamples > 0)
                {
                    int iCount = 0 ;
                    while ((iCount < numOutSamples) && !mbEnd)
                    {
                        int iToCopy = numOutSamples - iCount ;
                        if (iToCopy > 80)
                            iToCopy = 80 ;

                          if (nSamplesPartialFrame == 0)
                          {
                             if (iToCopy >= 80)
                             {
                                queueFrame((const unsigned short *)OutBuffer+iCount);
                             }
                             else
                             {
                                nSamplesPartialFrame = iToCopy ;
                                memcpy(partialFrame, (const unsigned short *)OutBuffer+iCount,iToCopy*sizeof(MpAudioSample)) ;
                             }
                          }
                          else
                          {
                             if (iToCopy > (80-nSamplesPartialFrame))
                                iToCopy = 80-nSamplesPartialFrame ;

                             memcpy(&partialFrame[nSamplesPartialFrame],(const unsigned short *)OutBuffer+iCount, iToCopy*sizeof(MpAudioSample)) ;
                             nSamplesPartialFrame += iToCopy ;

                             if (nSamplesPartialFrame == 80)
                             {
                                queueFrame((const unsigned short *) partialFrame);
                                nSamplesPartialFrame = 0 ;
                             }
                          }
                          iCount += iToCopy ;
                     }
                }
            }
            else
            {
               // Truncated data source?
               fireEvent(DecodingErrorEvent) ;
               break ;
            }
         }               
      }
      pSrc->close() ;
   }

   queueEndOfFrames() ;      
   fireEvent(DecodingCompletedEvent) ;

   mSemExited.release() ;

   return 0 ;
}


// Advances the mCurrentChunk to the next data chunk within the stream
UtlBoolean StreamWAVFormatDecoder::nextDataChunk(int& iLength)
{
   UtlBoolean bSuccess = FALSE ;
   int iRead ;
   char Header[128]; 
   unsigned long blockSize=0;
   int iCurrentPosition ;
   iLength = 0 ;
   
   StreamDataSource* pDataSource = getDataSource() ;
   if (pDataSource != NULL)
   {      
      while (!mbEnd && pDataSource->read((char*) Header, 4, iRead) == OS_SUCCESS)
      {      
          pDataSource->getPosition(iCurrentPosition);
         if (iCurrentPosition == 4 && memcmp(Header, "RIFF", 4) != 0)
         {
             //if this is true, then this file is not a wav, since
             //all wave files start with RIFF.
             mbEnd = TRUE;
             iLength = 0;

             // Search to see if this is a 404 error.
             pDataSource->read((char*) Header + 4, 123, iRead);
             Header[127] = '\0';
             if (strstr(Header, "404 Not Found") != NULL)
                syslog(FAC_STREAMING, PRI_ERR, "StreamWAVFormatDecoder::nextDataChunk (404 Not Found).)");
             else
                syslog(FAC_STREAMING, PRI_ERR, "StreamWAVFormatDecoder::nextDataChunk (RIFF not detected.)");

             // Just return successful with zero data read.
             return TRUE;
         }
         else
         if (memcmp(Header, "RIFF", 4) == 0)
         {
             //just read the block size
            if (pDataSource->read((char*) &blockSize, sizeof(blockSize), iRead) != OS_SUCCESS)
            {
                syslog(FAC_STREAMING, PRI_ERR, "StreamWAVFormatDecoder::nextDataChunk (Error reading block size!)");
                break;
            }

         }
         else
         if (memcmp(Header, "WAVE", 4) == 0)
             ; //do nothing, doesn't even have block size
         else
         if (memcmp(Header, "fmt ", 4) == 0)
         {
            
            FORMATChunkInfo formatChunkInfo;
            if (pDataSource->read((char*) &blockSize, sizeof(blockSize), iRead) == OS_SUCCESS)
            {
                //save the current position, well need it for the jump
                if (pDataSource->getPosition(iCurrentPosition) != OS_SUCCESS)
                {
                   break;
                }

                if (pDataSource->read((char*) &formatChunkInfo, sizeof(formatChunkInfo), iRead) == OS_SUCCESS)
                {
                    // for streaming, we currently only support one format:
                    // 16 bit, 8khz, signed.
                    // !SLG! Enable 8 bit playback - seems to work fine - run method converts 8 bit to 16 bit                   
                    if (formatChunkInfo.nSamplesPerSec != 8000 || 
                        (formatChunkInfo.nBitsPerSample != 16 && formatChunkInfo.nBitsPerSample != 8) ||
                        formatChunkInfo.nChannels != 1)
                    {
                         syslog(FAC_STREAMING, PRI_ERR, "StreamWAVFormatDecoder::nextDataChunk (File is not 8 or 16 bit, 8khz, mono, signed format!)");
                         mbEnd = TRUE;
                         break;
                    }

                    memcpy(&mFormatChunk, &formatChunkInfo, sizeof(formatChunkInfo)) ;
                    iLength = blockSize ;
                    
                    //now move to next block
                    if (pDataSource->seek(iCurrentPosition + blockSize) != OS_SUCCESS)
                    {
                       // Kick out if we cannot seek
                       break ;
                    }
                }
                else
                {
                    syslog(FAC_STREAMING, PRI_ERR, "StreamWAVFormatDecoder::nextDataChunk (Error reading block \"fmt\"!)");
                    break;
                }
            }
            else
            {
                syslog(FAC_STREAMING, PRI_ERR, "StreamWAVFormatDecoder::nextDataChunk (Error reading block size of fmt chunk)");
                break;
            }
             //we won't set true yet, because we'd like to get one block of data first
         }
         else
         if (memcmp(Header, "data", 4) == 0)
         {
            if (pDataSource->read((char*) &blockSize, sizeof(blockSize), iRead) == OS_SUCCESS)
            {
                 iLength = blockSize ;
                 bSuccess = TRUE ;
                 break ;
            }
         }            
         else
         {
            // Unsupported chunk, advance to the next one...
             //just read the block size & block
            if (pDataSource->read((char*) &blockSize, sizeof(blockSize), iRead) == OS_SUCCESS)
            {
                char tmpbuf[16000];
                int totalLeftToRead = blockSize;
                OsStatus retval = OS_FAILED;
                do
                {
                    retval = pDataSource->read(tmpbuf, 16000, iRead);
                    totalLeftToRead -= iRead;
                } while (totalLeftToRead > 0 && retval == OS_SUCCESS);
            }
            else
            {
                syslog(FAC_STREAMING, PRI_ERR, "StreamWAVFormatDecoder::nextDataChunk (Error reading block size of block)");
                break;
            }
         }
      }

   
       //if we haven't reached the end of the stream and we are still not success
       //then fire the decoding error
       int currentPosition;
       int streamLength;

       pDataSource->getLength(streamLength);
       pDataSource->getPosition(currentPosition);

       if (!bSuccess && (currentPosition < streamLength || streamLength == 0))
          fireEvent(DecodingErrorEvent) ;
   }
   

   return bSuccess ;
}


/* //////////////////////////// PRIVATE /////////////////////////////////// */

/* ============================ TESTING =================================== */

/* ============================ FUNCTIONS ================================= */

#endif // DISABLE_STREAM_PLAYER ]
