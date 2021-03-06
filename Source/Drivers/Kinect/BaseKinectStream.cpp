#include "BaseKinectStream.h"
#include "KinectStreamImpl.h"
#include <Shlobj.h>
#include "XnHash.h"
#include "XnEvent.h"
#include "XnPlatform.h"
#include "NuiApi.h"
#include "PS1080.h"
#include "XnMath.h"

using namespace oni::driver;
using namespace kinect_device;

BaseKinectStream::BaseKinectStream(KinectStreamImpl* pStreamImpl):
	m_pStreamImpl(pStreamImpl)
{
	m_running = false;
	pStreamImpl->addStream(this);
}

BaseKinectStream::~BaseKinectStream()
{
	destroy();
}

OniStatus BaseKinectStream::start()
{
	OniStatus status = m_pStreamImpl->start();
	if (status == ONI_STATUS_OK)
		m_running = TRUE;
	return status;
}

void BaseKinectStream::stop()
{
	m_running = FALSE;
	m_pStreamImpl->stop();
}

void BaseKinectStream::destroy()
{
	stop();
	m_pStreamImpl->removeStream(this);
}

OniStatus BaseKinectStream::getProperty(int propertyId, void* data, int* pDataSize)
{
	OniStatus status = ONI_STATUS_NOT_SUPPORTED;
	switch (propertyId)
	{
	case ONI_STREAM_PROPERTY_CROPPING:
		status = ONI_STATUS_NOT_IMPLEMENTED;
		break;
	case ONI_STREAM_PROPERTY_HORIZONTAL_FOV:
		{
			float* val = (float*)data;
			XnDouble tmp;
			if (m_videoMode.resolutionX == 640)
				tmp =  NUI_CAMERA_COLOR_NOMINAL_HORIZONTAL_FOV * xnl::Math::DTR;
			else
				tmp = NUI_CAMERA_DEPTH_NOMINAL_HORIZONTAL_FOV * xnl::Math::DTR;
			*val = (float)tmp;
			status = ONI_STATUS_OK;
			break;
		}		
	case ONI_STREAM_PROPERTY_VERTICAL_FOV:
		{
			float* val = (float*)data;
			XnDouble tmp;
			if (m_videoMode.resolutionY == 480)
				tmp =  NUI_CAMERA_COLOR_NOMINAL_VERTICAL_FOV * xnl::Math::DTR;
			else
				tmp = NUI_CAMERA_DEPTH_NOMINAL_VERTICAL_FOV * xnl::Math::DTR;
			*val = (float)tmp;
			status = ONI_STATUS_OK;
			break;
		}
	case ONI_STREAM_PROPERTY_VIDEO_MODE:
		{
			if (*pDataSize != sizeof(OniVideoMode))
			{
				printf("Unexpected size: %d != %d\n", *pDataSize, sizeof(OniVideoMode));
				status = ONI_STATUS_ERROR;
			}
			else
			{
				status = GetVideoMode((OniVideoMode*)data);
			}
			
			break;
		}		
	default:
		status = ONI_STATUS_NOT_SUPPORTED;
		break;
	}

	return status;
}

OniStatus BaseKinectStream::setProperty(int propertyId, const void* data, int dataSize)
{
	OniStatus status = ONI_STATUS_NOT_SUPPORTED;
	if (propertyId == ONI_STREAM_PROPERTY_VIDEO_MODE)
	{
		if (dataSize != sizeof(OniVideoMode))
		{
			printf("Unexpected size: %d != %d\n", dataSize, sizeof(OniVideoMode));
			 status = ONI_STATUS_ERROR;
		}
		status = SetVideoMode((OniVideoMode*)data);
	}
	return status;
}

OniBool BaseKinectStream::isPropertySupported(int propertyId)
{
	OniBool status = FALSE;
	switch (propertyId)
	{
	case ONI_STREAM_PROPERTY_HORIZONTAL_FOV:
	case ONI_STREAM_PROPERTY_VERTICAL_FOV:
	case ONI_STREAM_PROPERTY_VIDEO_MODE:
		status = TRUE;
	default:
		status = FALSE;
		break;
	}
	return status;
}

void BaseKinectStream::notifyAllProperties()
{
	XnFloat nDouble;
	int size = sizeof(nDouble);
	getProperty(ONI_STREAM_PROPERTY_HORIZONTAL_FOV, &nDouble, &size);
	raisePropertyChanged(ONI_STREAM_PROPERTY_HORIZONTAL_FOV, &nDouble, size);

	getProperty(ONI_STREAM_PROPERTY_VERTICAL_FOV, &nDouble, &size);
	raisePropertyChanged(ONI_STREAM_PROPERTY_VERTICAL_FOV, &nDouble, size);
	
	OniVideoMode videoMode;
	size = sizeof(videoMode);

	getProperty(ONI_STREAM_PROPERTY_VIDEO_MODE, &videoMode, &size);
	raisePropertyChanged(ONI_STREAM_PROPERTY_VIDEO_MODE, &videoMode, size);	
}

OniStatus BaseKinectStream::SetVideoMode(OniVideoMode* videoMode)
{
	if (!m_pStreamImpl->isRunning())
	{
		m_videoMode = *videoMode;
		m_pStreamImpl->setVideoMode(videoMode);
		return ONI_STATUS_OK;
	}
	
	return ONI_STATUS_NOT_SUPPORTED;
}

OniStatus BaseKinectStream::GetVideoMode(OniVideoMode* pVideoMode)
{
	*pVideoMode = m_videoMode;
	return ONI_STATUS_OK;
}

void BaseKinectStream::addRefToFrame(OniDriverFrame* pFrame)
{
	++((KinectStreamFrameCookie*)pFrame->pDriverCookie)->refCount;
}

void BaseKinectStream::releaseFrame(OniDriverFrame* pFrame)
{
	if (0 == --((KinectStreamFrameCookie*)pFrame->pDriverCookie)->refCount)
	{
		xnOSFree(pFrame->pDriverCookie);
		xnOSFreeAligned(pFrame->frame.data);
		xnOSFree(pFrame);
	}
}
