#include "../base/zm.h"
#include "zmLocalFileDump.h"

#include "../base/zmFeedFrame.h"
#include "../base/zmFeedProvider.h"

LocalFileDump::LocalFileDump( const std::string &name, const std::string &location ) :
    FeedConsumer(),
    Thread( identity() ),
    mLocation( location )
{
    setIdentity( cClass(), name );
}

int LocalFileDump::run()
{
    std::string filePath;
    FILE *fileDesc = NULL;

    if ( waitForProviders() )
    {
        while( !mStop )
        {
            mQueueMutex.lock();
            if ( !mFrameQueue.empty() )
            {
                for ( FrameQueue::iterator iter = mFrameQueue.begin(); iter != mFrameQueue.end(); iter++ )
                {
                    const FeedFrame *frame = iter->get();
                    Info( "F:%ld", frame->buffer().size() );
                    if ( filePath.empty() )
                    {
                        filePath = stringtf( "%s/%s-%s", mLocation.c_str(), mName.c_str(), frame->provider()->cidentity() );
                        Info( "Path: %s", filePath.c_str() );
                        fileDesc = fopen( filePath.c_str(), "w" );
                        if ( !fileDesc )
                            Fatal( "Failed to open dump file '%s': %s", filePath.c_str(), strerror(errno) );
                    }
                    if ( fwrite( frame->buffer().data(), frame->buffer().size(), 1, fileDesc ) <= 0 )
                        Fatal( "Failed to write to dump file '%s': %s", filePath.c_str(), strerror(errno) );
                    //delete *iter;
                }
                mFrameQueue.clear();
            }
            mQueueMutex.unlock();
            checkProviders();
            usleep( INTERFRAME_TIMEOUT );
        }
        fclose( fileDesc );
    }
    cleanup();
    return( 0 );
}

