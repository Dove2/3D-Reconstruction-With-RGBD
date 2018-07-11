﻿//------------------------------------------------------------------------------
// <copyright file="DepthBasics.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include "stdafx.h"
#include <strsafe.h>
#include "KinectSensor.h"
#include "resource.h"

#include <iostream>
#include <windows.h>
/// <summary>
/// Constructor
/// </summary>
KinectSensor::KinectSensor() :
    m_hNextDepthFrameEvent(INVALID_HANDLE_VALUE),
	m_hNextColorFrameEvent(INVALID_HANDLE_VALUE),
    m_pDepthStreamHandle(INVALID_HANDLE_VALUE),
	m_pColorStreamHandle(INVALID_HANDLE_VALUE),
    m_bNearMode(false),
    m_pNuiSensor(NULL)
{
    // create heap storage for depth pixel data in RGBX format
    m_depthRGBX = new BYTE[cDepthWidth*cDepthHeight*cBytesPerPixel];
	depthValues = new USHORT[cDepthWidth*cDepthHeight];
	colorsRGBValues = new USHORT[cDepthWidth * cDepthHeight * 3];
}

/// <summary>
/// Destructor
/// </summary>
KinectSensor::~KinectSensor()
{
    if (m_pNuiSensor)
    {
        m_pNuiSensor->NuiShutdown();
    }

    if (m_hNextDepthFrameEvent != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hNextDepthFrameEvent);
    }

    // done with depth pixel data
    delete[] m_depthRGBX;
	delete[] colorsRGBValues;
	delete[] depthValues;

    SafeRelease(m_pNuiSensor);
	//CloseHandle(m_hNextDepthFrameEvent);
	//CloseHandle(m_hNextColorFrameEvent);
}

/// <summary>
/// Main processing function
/// </summary>
void KinectSensor::Update()
{
    if (NULL == m_pNuiSensor)
    {
        return;
    }

    if ( WAIT_OBJECT_0 == WaitForSingleObject(m_hNextDepthFrameEvent, 0) )
    {
        ProcessDepth();
		ProcessColor();
    }
}

/// <summary>
/// Create the first connected Kinect found 
/// </summary>
/// <returns>indicates success or failure</returns>
HRESULT KinectSensor::CreateFirstConnected()
{
    INuiSensor * pNuiSensor;
    HRESULT hr;

    int iSensorCount = 0;
    hr = NuiGetSensorCount(&iSensorCount);
    if (FAILED(hr))
    {
        return hr;
    }

    // Look at each Kinect sensor
    for (int i = 0; i < iSensorCount; ++i)
    {
        // Create the sensor so we can check status, if we can't create it, move on to the next
        hr = NuiCreateSensorByIndex(i, &pNuiSensor);
        if (FAILED(hr))
        {
            continue;
        }

        // Get the status of the sensor, and if connected, then we can initialize it
        hr = pNuiSensor->NuiStatus();
        if (S_OK == hr)
        {
            m_pNuiSensor = pNuiSensor;
            break;
        }

        // This sensor wasn't OK, so release it since we're not using it
        pNuiSensor->Release();
    }

    if (NULL != m_pNuiSensor)
    {
        // Initialize the Kinect and specify that we'll be using depth
        hr = m_pNuiSensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_COLOR | NUI_INITIALIZE_FLAG_USES_DEPTH);
        if (SUCCEEDED(hr))
        {
            // Create an event that will be signaled when depth data is available
            m_hNextDepthFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

            // Open a depth image stream to receive depth frames
            hr = m_pNuiSensor->NuiImageStreamOpen(
                NUI_IMAGE_TYPE_DEPTH,
                NUI_IMAGE_RESOLUTION_640x480,
                0,
                2,
                m_hNextDepthFrameEvent,
                &m_pDepthStreamHandle);

			// Create an event that will be signaled when color data is available
			m_hNextColorFrameEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

			// Open a color image stream to receive color frames
			hr = m_pNuiSensor->NuiImageStreamOpen(
				NUI_IMAGE_TYPE_COLOR,
				NUI_IMAGE_RESOLUTION_640x480,
				0,
				2,
				m_hNextColorFrameEvent,
				&m_pColorStreamHandle);
        }
    }

    if (NULL == m_pNuiSensor || FAILED(hr))
    {
        SetStatusMessage(L"No ready Kinect found!");
        return E_FAIL;
    }

    return hr;
}

void KinectSensor::DisConnected() {
	if (m_pNuiSensor)
		m_pNuiSensor->NuiShutdown();
}

/// <summary>
/// Handle new depth data
/// </summary>
void KinectSensor::ProcessDepth()
{
    HRESULT hr;
    NUI_IMAGE_FRAME imageFrame;
    // Attempt to get the depth frame
    hr = m_pNuiSensor->NuiImageStreamGetNextFrame(m_pDepthStreamHandle, 0, &imageFrame);
    if (FAILED(hr))	
    {
        return;
    }

    BOOL nearMode;
    INuiFrameTexture* pTexture;

    // Get the depth image pixel texture
    hr = m_pNuiSensor->NuiImageFrameGetDepthImagePixelFrameTexture(
        m_pDepthStreamHandle, &imageFrame, &nearMode, &pTexture);
    if (FAILED(hr))
    {
        goto ReleaseFrame;
    }

    NUI_LOCKED_RECT LockedRect;

    // Lock the frame data so the Kinect knows not to modify it while we're reading it
    pTexture->LockRect(0, &LockedRect, NULL, 0);

    // Make sure we've received valid data
    if (LockedRect.Pitch != 0)
    {
        // Get the min and max reliable depth for the current frame
        int minDepth = (nearMode ? NUI_IMAGE_DEPTH_MINIMUM_NEAR_MODE : NUI_IMAGE_DEPTH_MINIMUM) >> NUI_IMAGE_PLAYER_INDEX_SHIFT;
        int maxDepth = (nearMode ? NUI_IMAGE_DEPTH_MAXIMUM_NEAR_MODE : NUI_IMAGE_DEPTH_MAXIMUM) >> NUI_IMAGE_PLAYER_INDEX_SHIFT;

        BYTE * rgbrun = m_depthRGBX;
		USHORT * depthValue = depthValues;
        const NUI_DEPTH_IMAGE_PIXEL * pBufferRun = reinterpret_cast<const NUI_DEPTH_IMAGE_PIXEL *>(LockedRect.pBits);

        // end pixel is start + width*height - 1
        const NUI_DEPTH_IMAGE_PIXEL * pBufferEnd = pBufferRun + (cDepthWidth * cDepthHeight);
		
		//-----------------test---------------//
		//long count = 0;

		//----------test end -------------------//
        while ( pBufferRun < pBufferEnd )
        {
            // discard the portion of the depth that contains only the player index
            USHORT depth = pBufferRun->depth;

			*depthValue = (depth >= minDepth && depth <= maxDepth ? depth - minDepth : 0);
			depthValue++;

			//-----------------test---------------//
			//count++;

			//----------test end -------------------//

            // To convert to a byte, we're discarding the most-significant
            // rather than least-significant bits.
            // We're preserving detail, although the intensity will "wrap."
            // Values outside the reliable depth range are mapped to 0 (black).

            // Note: Using conditionals in this loop could degrade performance.
            // Consider using a lookup table instead when writing production code.
            BYTE intensity = static_cast<BYTE>(depth >= minDepth && depth <= maxDepth ? depth % 256 : 0);

            // Write out blue byte
            *(rgbrun++) = intensity;

            // Write out green byte
            *(rgbrun++) = intensity;

            // Write out red byte
            *(rgbrun++) = intensity;

            // We're outputting BGR, the last byte in the 32 bits is unused so skip it
            // If we were outputting BGRA, we would write alpha here.
            ++rgbrun;

            // Increment our index into the Kinect's depth buffer
            ++pBufferRun;
        }

		//-----------------test---------------//
		//std::cout << count << std::endl;

		//----------test end -------------------//
    }

    // We're done with the texture so unlock it
    pTexture->UnlockRect(0);

    pTexture->Release();

ReleaseFrame:
    // Release the frame
    m_pNuiSensor->NuiImageStreamReleaseFrame(m_pDepthStreamHandle, &imageFrame);


}


void KinectSensor::ProcessColor() {

	HRESULT hr;
	NUI_IMAGE_FRAME imageFrame;

	// Attempt to get the color frame
	hr = m_pNuiSensor->NuiImageStreamGetNextFrame(m_pColorStreamHandle, 0, &imageFrame);
	if (FAILED(hr))
	{
		return;
	}

	INuiFrameTexture * pTexture = imageFrame.pFrameTexture;
	NUI_LOCKED_RECT LockedRect;

	// Lock the frame data so the Kinect knows not to modify it while we're reading it
	pTexture->LockRect(0, &LockedRect, NULL, 0);

	// Make sure we've received valid data
	if (LockedRect.Pitch != 0)
	{
		for (int j = 0; j < cDepthHeight; j++)
		{
			for (int i = 0; i < cDepthWidth; i++)
			{
				//内部数据是4个字节，0-1-2是BGR，第4个现在未使用
				//std::cout << (int)LockedRect.pBits[4 * j] << " " << (int)LockedRect.pBits[4 * j + 1] << " " << (int)LockedRect.pBits[4 * j + 2] << std::endl;
				colorsRGBValues[3 * cDepthWidth * j + i + 0] = (int)LockedRect.pBits[4 * cDepthWidth * j + i]; // R
				colorsRGBValues[3 * cDepthWidth * j + i + 1] = (int)LockedRect.pBits[4 * cDepthWidth * j + i + 1]; // G
				colorsRGBValues[3 * cDepthWidth * j + i + 2] = (int)LockedRect.pBits[4 * cDepthWidth * j + i + 2]; // B
			}
		}
	}

	// We're done with the texture so unlock it
	pTexture->UnlockRect(0);

	// Release the frame
	m_pNuiSensor->NuiImageStreamReleaseFrame(m_pColorStreamHandle, &imageFrame);
}


/// <summary>
/// Set the status bar message
/// </summary>
/// <param name="szMessage">message to display</param>
void KinectSensor::SetStatusMessage(WCHAR * szMessage)
{
    SendDlgItemMessageW(m_hWnd, IDC_STATUS, WM_SETTEXT, 0, (LPARAM)szMessage);
}