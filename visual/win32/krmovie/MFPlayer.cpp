
#include <windows.h>
#include <assert.h>
#include <math.h>
#include <cmath>
#include <propvarutil.h>
#include <string>

// for MFPCreateMediaPlayer
#include <Mfplay.h>
// Mfplay.lib
// Mfplay.dll

// for MFCreateMFByteStreamOnStream, MFCreateVideoRendererActivate
#include <Mfidl.h>
// Mfplat.lib
// Mfplat.dll

#include <streams.h>
#include <atlbase.h>
#include <atlcom.h>

#include "../krmovie.h"
#include "MFPlayer.h"

#if 0
#include "TVPEVR.h"
#else
#include <intsafe.h>
#include <mfapi.h>
#include <mferror.h>
//#include <dxva2api.h>
//#include <evr9.h>
//#include <evcode.h> // EVR event codes (IMediaEventSink)
#endif

#include "DShowException.h"
#include "tp_stub.h"

#pragma comment( lib, "propsys.lib" )
#pragma comment( lib, "Mfplay.lib" )
#pragma comment( lib, "Mfplat.lib" )
#pragma comment( lib, "Mf.lib" )
#pragma comment( lib, "Mfuuid.lib" )


#pragma comment( lib, "d3d9.lib" )
#pragma comment( lib, "dxva2.lib" )
#pragma comment( lib, "evr.lib" )

// DEFINE_CLASSFACTORY_SERVER_LOCK

//----------------------------------------------------------------------------
//! @brief	  	VideoOverlay MediaFoundation���擾����
//! @param		callbackwin : 
//! @param		stream : 
//! @param		streamname : 
//! @param		type : 
//! @param		size : 
//! @param		out : VideoOverlay Object
//! @return		�G���[������
//----------------------------------------------------------------------------
void __stdcall GetMFVideoOverlayObject(
	HWND callbackwin, IStream *stream, const wchar_t * streamname,
	const wchar_t *type, unsigned __int64 size, iTVPVideoOverlay **out)
{
	*out = new tTVPMFPlayer;

	if( *out )
		static_cast<tTVPMFPlayer*>(*out)->BuildGraph( callbackwin, stream, streamname, type, size );
}
//----------------------------------------------------------------------------
STDMETHODIMP tTVPPlayerCallback::NonDelegatingQueryInterface(REFIID riid,void **ppv) {
	if(IsEqualIID(riid,IID_IMFPMediaPlayerCallback)) return GetInterface(static_cast<IMFPMediaPlayerCallback *>(this),ppv);
	if(IsEqualIID(riid,IID_IMFAsyncCallback)) return GetInterface(static_cast<IMFAsyncCallback *>(this),ppv);
	return CUnknown::NonDelegatingQueryInterface(riid,ppv);
}
void STDMETHODCALLTYPE tTVPPlayerCallback::OnMediaPlayerEvent( MFP_EVENT_HEADER *pEventHeader ) {
	if( FAILED(pEventHeader->hrEvent) ) {
		owner_->NotifyError( pEventHeader->hrEvent );
		return;
	}
	switch( pEventHeader->eEventType ) {
	case MFP_EVENT_TYPE_MEDIAITEM_CREATED:
		owner_->OnMediaItemCreated( MFP_GET_MEDIAITEM_CREATED_EVENT(pEventHeader) );
		break;

	case MFP_EVENT_TYPE_MEDIAITEM_SET:
		owner_->OnMediaItemSet( MFP_GET_MEDIAITEM_SET_EVENT(pEventHeader) );
		break;

	case MFP_EVENT_TYPE_RATE_SET:
		owner_->OnRateSet( MFP_GET_RATE_SET_EVENT(pEventHeader) );
		break;

	// -----
	case MFP_EVENT_TYPE_PLAYBACK_ENDED:
		owner_->OnPlayBackEnded( MFP_GET_PLAYBACK_ENDED_EVENT(pEventHeader) );
		break;
	case MFP_EVENT_TYPE_STOP:
		owner_->OnStop( MFP_GET_STOP_EVENT(pEventHeader) );
		break;
	case MFP_EVENT_TYPE_PLAY:
		owner_->OnPlay( MFP_GET_PLAY_EVENT(pEventHeader) );
		break;
	case MFP_EVENT_TYPE_PAUSE:
		owner_->OnPause( MFP_GET_PAUSE_EVENT(pEventHeader) );
		break;
	case MFP_EVENT_TYPE_POSITION_SET:
		owner_->OnPositionSet( MFP_GET_POSITION_SET_EVENT(pEventHeader) );
		break;
	case MFP_EVENT_TYPE_FRAME_STEP:
		owner_->OnFremeStep( MFP_GET_FRAME_STEP_EVENT(pEventHeader) );
		break;
	case MFP_EVENT_TYPE_MEDIAITEM_CLEARED:
		owner_->OnMediaItemCleared( MFP_GET_MEDIAITEM_CLEARED_EVENT(pEventHeader) );
		break;
	case MFP_EVENT_TYPE_MF:
		owner_->OnMF( MFP_GET_MF_EVENT(pEventHeader) );
		break;
	case MFP_EVENT_TYPE_ERROR:
		owner_->OnError( MFP_GET_ERROR_EVENT(pEventHeader) );
		break;
	case MFP_EVENT_TYPE_ACQUIRE_USER_CREDENTIAL:
		owner_->OnAcquireUserCredential( MFP_GET_ACQUIRE_USER_CREDENTIAL_EVENT(pEventHeader) );
		break;
	}
	owner_->NotifyState( pEventHeader->eState );
}

STDMETHODIMP tTVPPlayerCallback::GetParameters( DWORD *pdwFlags, DWORD *pdwQueue ) {
	return E_NOTIMPL;
}
#if 0
class SessionHandler {
public:
	void OnClosed();
	void OnPaused();
	void OnEnded();
	void OnNotifyPresentationTime();
	void OnRateChaned();
	void OnScrubSampleComplete();
	void OnStarted();
	void OnStopped();
	void OnStreamSinkFormatChanged();
	void OnTopologiesCleared();
	void OnTopologySet();
	void OnTopologyStatus();

};
#endif
STDMETHODIMP tTVPPlayerCallback::Invoke( IMFAsyncResult *pAsyncResult ) {
	HRESULT hr;
	MediaEventType met = MESessionClosed;
	CComPtr<IMFMediaEvent> pMediaEvent;
	if( SUCCEEDED(hr = owner_->GetMediaSession()->EndGetEvent( pAsyncResult, &pMediaEvent )) ) {
		if( SUCCEEDED(hr = pMediaEvent->GetType(&met)) ) {
			// OutputDebugString( std::to_wstring(met).c_str() ); OutputDebugString( L"\n" );
			PROPVARIANT pvValue;
			PropVariantInit(&pvValue);
			switch( met ) {
			case MESessionClosed:
				owner_->OnMediaItemCleared(NULL);
				break;
			case MESessionPaused:
				owner_->OnPause(NULL);
				break;
			case MESessionEnded:
				owner_->OnPlayBackEnded(NULL);
				break;
			case MESessionNotifyPresentationTime:
				break;
			case MESessionRateChanged:
				if( SUCCEEDED(pMediaEvent->GetValue( &pvValue )) ) {
					double value;
					if( FAILED(PropVariantToDouble(pvValue,&value)) ) {
						value = 1.0;
					}
					owner_->OnRateSet(NULL);// owner_->OnRateSet(value);
				} else {
					owner_->OnRateSet(NULL);// owner_->OnRateSet(1.0);
				}
				break;
			case MESessionScrubSampleComplete:
				break;
			case MESessionStarted:
				owner_->OnPlay(NULL);
				break;
			case MESessionStopped:
				owner_->OnStop(NULL);
				break;
			case MESessionStreamSinkFormatChanged:
				break;
			case MESessionTopologiesCleared:
				break;
			case MESessionTopologySet:
				break;
			case MESessionTopologyStatus: {
				UINT32 status = MF_TOPOSTATUS_INVALID;
				pMediaEvent->GetUINT32( MF_EVENT_TOPOLOGY_STATUS, &status );
				owner_->OnTopologyStatus(status);
				break;
				}
			}
			PropVariantClear(&pvValue);
		}
		owner_->GetMediaSession()->BeginGetEvent( this, NULL );
	}
	return S_OK;
}
//----------------------------------------------------------------------------
#if 0
class EVRActiveObject : public IMFActivate, public CUnknown {
	CComPtr<tTVPEVRCustomPresenter> CustomPres;
public:
	EVRActiveObject() : CUnknown(L"EVRActiveObject",NULL) {} 

	// IUnknown
	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv) {
		if(IsEqualIID(riid,IID_IMFActivate)) return GetInterface(static_cast<IMFActivate *>(this),ppv);
		return CUnknown::NonDelegatingQueryInterface(riid,ppv);
	}

	STDMETHODIMP ActivateObject( REFIID riid, void **ppv ) {
		if( riid != __uuidof(IMFVideoPresenter) ) {
			return MF_E_CANNOT_CREATE_SINK;
		}
		if( ppv == NULL ) {
			return E_POINTER;
		}
		if( CustomPres != NULL ) {
			CComQIPtr<IMFVideoPresenter> mfvp(CustomPres);
			*ppv = mfvp;
			mfvp.p->AddRef();
			return S_OK;
		}
		//CComObject<tTVPEVRCustomPresenter> * object;
		//CComObject<tTVPEVRCustomPresenter>::CreateInstance(&object);
		HRESULT hr;
		CComPtr<tTVPEVRCustomPresenter>	evrPresent;
		hr = tTVPEVRCustomPresenter::CreateInstance(NULL,IID_IMFVideoPresenter,(void**)&evrPresent);
		CustomPres.Attach(evrPresent);
		CustomPres.p->AddRef();
		CComQIPtr<IMFVideoPresenter> mfvp(evrPresent);
		if( mfvp == NULL ) {
			return MF_E_CANNOT_CREATE_SINK;
		}
		*ppv = mfvp;
		mfvp.p->AddRef();
		return S_OK;
	}
	STDMETHODIMP DetachObject() {
		CustomPres.Release();
		return S_OK;
	}
	STDMETHODIMP ShutdownObject() {
		CustomPres.Release();
		return S_OK;
	}
};
#endif
//----------------------------------------------------------------------------
tTVPMFPlayer::tTVPMFPlayer() {
	CoInitialize(NULL);
	MFStartup( MF_VERSION );

	OwnerWindow = NULL;
	CallbackWindow = NULL;
	Visible = false;
	Rect.left = 0; Rect.top = 0; Rect.right = 320; Rect.bottom = 240;
	RefCount = 1;
	Shutdown = false;
	PlayerCallback = new tTVPPlayerCallback(this);
	MediaPlayer = NULL;
	ByteStream = NULL;
	MediaItem = NULL;
	// AudioVolume = NULL;
	VideoDisplayControl = NULL;
	VideoStatue = vsStopped;
	FPSNumerator = 1;
	FPSDenominator = 1;
	
	MediaSession = NULL;
	Topology = NULL;
	Stream = NULL;
}
tTVPMFPlayer::~tTVPMFPlayer() {
	Shutdown = true;
	ReleaseAll();
	MFShutdown();
	CoUninitialize();
}
void __stdcall tTVPMFPlayer::AddRef(){
	RefCount++;
}
void __stdcall tTVPMFPlayer::Release(){
	if(RefCount == 1)
		delete this;
	else
		RefCount--;
}
//----------------------------------------------------------------------------
void __stdcall tTVPMFPlayer::BuildGraph( HWND callbackwin, IStream *stream,
	const wchar_t * streamname, const wchar_t *type, unsigned __int64 size )
{
	OwnerWindow = callbackwin;
	CallbackWindow = callbackwin;
	StreamName = std::wstring(streamname);
	Stream = stream;
	Stream->AddRef();
	
	HRESULT hr = S_OK;
	if( FAILED(hr = MFCreateMFByteStreamOnStream( stream, &ByteStream )) ) {
		TVPThrowExceptionMessage(L"Faild to create stream.");
	}
}

void tTVPMFPlayer::OnTopologyStatus(UINT32 status) {
	HRESULT hr;
	switch( status ) {
	case MF_TOPOSTATUS_INVALID:
		break;
	case MF_TOPOSTATUS_READY: {
		CComPtr<IMFGetService> pGetService;
		if( SUCCEEDED(hr = MediaSession->QueryInterface( &pGetService )) ) {
			hr = pGetService->GetService( MR_VIDEO_RENDER_SERVICE, IID_IMFVideoDisplayControl, (void**)&VideoDisplayControl );
		}
		break;
		}
	case MF_TOPOSTATUS_STARTED_SOURCE:
		break;
	case MF_TOPOSTATUS_DYNAMIC_CHANGED:
		break;
	case MF_TOPOSTATUS_SINK_SWITCHED:
		break;
	case MF_TOPOSTATUS_ENDED:
		break;
	}
}
HRESULT tTVPMFPlayer::CreateVideoPlayer( HWND hWnd ) {
	if( hWnd == NULL || hWnd == INVALID_HANDLE_VALUE )
		return E_FAIL;

	if( MediaSession ) {
		return S_OK;	// ���ɍ쐬�ς�
	}

	HRESULT hr;
	if( FAILED(hr = MFCreateMediaSession( NULL, &MediaSession )) ) {
		TVPThrowExceptionMessage(L"Faild to create Media session.");
	}
	if( FAILED(hr = MediaSession->BeginGetEvent( PlayerCallback, NULL )) ) {
		TVPThrowExceptionMessage(L"Faild to begin get event.");
	}
	CComPtr<IMFSourceResolver> pSourceResolver;
	if( FAILED(hr = MFCreateSourceResolver(&pSourceResolver)) ) {
		TVPThrowExceptionMessage(L"Faild to create source resolver.");
	}
	MF_OBJECT_TYPE ObjectType = MF_OBJECT_INVALID;
	CComPtr<IUnknown> pSource;
	if( FAILED(hr = pSourceResolver->CreateObjectFromByteStream( ByteStream, StreamName.c_str(), MF_RESOLUTION_MEDIASOURCE, NULL, &ObjectType, (IUnknown**)&pSource )) ) {
	//if( FAILED(hr = pSourceResolver->CreateObjectFromURL( L"C:\\ToolDev\\kirikiri_vc\\master\\krkrz\\bin\\win32\\data\\test.mp4",
	//	MF_RESOLUTION_MEDIASOURCE, NULL, &ObjectType, (IUnknown**)&pSource)) ) {
		TVPThrowExceptionMessage(L"Faild to open stream.");
	}
	if( ObjectType != MF_OBJECT_MEDIASOURCE ) {
		TVPThrowExceptionMessage(L"Invalid media source.");
	}
	CComPtr<IMFMediaSource> pMediaSource;
	if( FAILED(hr = pSource.QueryInterface(&pMediaSource)) ) {
		TVPThrowExceptionMessage(L"Faild to query Media source.");
	}
	if( FAILED(hr = MFCreateTopology(&Topology)) ) {
		TVPThrowExceptionMessage(L"Faild to create Topology.");
	}
	CComPtr<IMFPresentationDescriptor> pPresentationDescriptor;
	if( FAILED(hr = pMediaSource->CreatePresentationDescriptor(&pPresentationDescriptor)) ) {
		TVPThrowExceptionMessage(L"Faild to create Presentation Descriptor.");
	}
	DWORD streamCount;
	if( FAILED(hr = pPresentationDescriptor->GetStreamDescriptorCount(&streamCount)) ) {
		TVPThrowExceptionMessage(L"Faild to get stream count.");
	}
	if( streamCount < 1 ) {
		TVPThrowExceptionMessage(L"Not found media stream.");
	}
	for( DWORD i = 0; i < streamCount; i++ ) {
		if( FAILED(hr = AddBranchToPartialTopology(Topology, pMediaSource, pPresentationDescriptor, i, hWnd)) ) {
			TVPThrowExceptionMessage(L"Faild to add nodes.");
		}
	}
	
	if( FAILED(hr = MediaSession->SetTopology( 0, Topology )) ) {
		TVPThrowExceptionMessage(L"Faild to set topology.");
	}
#if 0
	hr = pSource.QueryInterface(&MediaItem);
	if( SUCCEEDED(hr) ) {
		BOOL hasVideo, selected;
		hr = MediaItem->HasVideo( &hasVideo, &selected );
		if( FAILED(hr) ) {
			TVPThrowExceptionMessage(L"Faild to call HasVide.");
		}
		if( hasVideo && selected ) {
			DWORD streamCount = 0;
			hr = MediaItem->GetNumberOfStreams( &streamCount );
			if( FAILED(hr) ) {
				TVPThrowExceptionMessage(L"Faild to call GetNumberOfStreams.");
			}
			DWORD videoStreamIndex = 0;
			PROPVARIANT var;
			PropVariantInit(&var);
			for( DWORD sidx = 0; sidx < streamCount; sidx++ ) {
				PropVariantInit(&var);
				hr = MediaItem->GetStreamAttribute( sidx, MF_MT_MAJOR_TYPE, &var);
				if( SUCCEEDED(hr) && var.vt == VT_CLSID ) {
					if( IsEqualGUID( *var.puuid, MFMediaType_Video ) ) {
						// OutputDebugString( L"Video" );
						PropVariantClear(&var);
						videoStreamIndex = sidx;
						PropVariantInit(&var);

						// Get FPS
						if( SUCCEEDED(hr = MediaItem->GetStreamAttribute( sidx, MF_MT_FRAME_RATE, &var)) ) {
							ULONGLONG val;
							hr = PropVariantToUInt64( var, &val );
							if( SUCCEEDED(hr) ) {
								FPSNumerator = (unsigned long)((val>>32ULL)&0xffffffff);
								FPSDenominator = (unsigned long)(val&0xffffffff);
							}
							PropVariantClear(&var);
						}
						break;
					} else if( IsEqualGUID( *var.puuid, MFMediaType_Audio ) ) {
						// OutputDebugString( L"Audio" );
					}
				}
				PropVariantClear(&var);
			}
		}
	}
#endif

#if 0
	//hr = MFPCreateMediaPlayer( NULL, FALSE, 0, PlayerCallback, NULL, &MediaPlayer );
	hr = MFPCreateMediaPlayer( NULL, FALSE, 0, PlayerCallback, NULL, &MediaPlayer );
	if( FAILED(hr) ) {
		TVPThrowExceptionMessage(L"Faild to create Media Player.");
	}


	//hr = MediaPlayer->CreateMediaItemFromObject( ByteStream, FALSE, this, &MediaItem );	// �񓯊�����
	hr = MediaPlayer->CreateMediaItemFromObject( ByteStream, TRUE, (DWORD_PTR)this, &MediaItem );	// ��������
	if( FAILED(hr) ) {
		TVPThrowExceptionMessage(L"Faild to create media item.");
	}

	// �摜�X�g���[���Ɏ��O�̕`��pSink��ݒ肷��
	// �񓯊��Ń��f�B�A���J���ꍇ�A���̏����́AMFP_EVENT_TYPE_MEDIAITEM_SET �C�x���g���ɍs���K�v������
	// �擾�� IMFPMediaPlayer::GetMediaItem �ōs��
	BOOL hasVideo, selected;
	hr = MediaItem->HasVideo( &hasVideo, &selected );
	if( FAILED(hr) ) {
		TVPThrowExceptionMessage(L"Faild to call HasVide.");
	}
	if( hasVideo && selected ) {
		CComPtr<IMFActivate> pActivate;
		if( FAILED(hr = MFCreateVideoRendererActivate(OwnerWindow, &pActivate)) ) {
			TVPThrowExceptionMessage(L"Faild to call MFCreateVideoRendererActivate.");
		}
		//CComObject<EVRActiveObject>* pEVRActiveObj;
		//CComObject<EVRActiveObject>::CreateInstance(&pEVRActiveObj);
		//CComPtr<IUnknown> pUnknown(pEVRActiveObj);
		CComPtr<IUnknown> pUnknown;
		if( FAILED(hr = tTVPEVRCustomPresenter::CreateInstance( NULL, IID_IUnknown, (void**)&pUnknown ) ) ) {
			TVPThrowExceptionMessage(L"Faild to create Activate.");
		}
		if( FAILED(hr = pActivate->SetUnknown(MF_ACTIVATE_CUSTOM_VIDEO_PRESENTER_ACTIVATE, pUnknown) ) ) {
			TVPThrowExceptionMessage(L"Faild to call IMFActivate::SetUnknown.");
		}
		CComPtr<IMFVideoPresenter> pVideoPresenter;
		//if( FAILED(hr = pUnknown.ActivateObject( IID_IMFVideoPresenter, (void**)&pVideoPresenter ) ) ) {
		if( FAILED(hr = pUnknown.QueryInterface( &pVideoPresenter ) ) ) {
			TVPThrowExceptionMessage(L"Faild to get VideoPresenter.");
		}
		//if( FAILED( hr = pVideoPresenter.p->QueryInterface( IID_IMFVideoDisplayControl, (void**)&VideoDisplayControl ) ) ) {
		if( FAILED( hr = pVideoPresenter.QueryInterface( &VideoDisplayControl ) ) ) {
			TVPThrowExceptionMessage(L"Faild to get VideoDisplayControl.");
		}

		DWORD streamCount = 0;
		hr = MediaItem->GetNumberOfStreams( &streamCount );
		if( FAILED(hr) ) {
			TVPThrowExceptionMessage(L"Faild to call GetNumberOfStreams.");
		}
		DWORD videoStreamIndex = 0;
		PROPVARIANT var;
		PropVariantInit(&var);
		for( DWORD sidx = 0; sidx < streamCount; sidx++ ) {
			PropVariantInit(&var);
			hr = MediaItem->GetStreamAttribute( sidx, MF_MT_MAJOR_TYPE, &var);
			if( SUCCEEDED(hr) && var.vt == VT_CLSID ) {
				if( IsEqualGUID( *var.puuid, MFMediaType_Video ) ) {
					// OutputDebugString( L"Video" );
					PropVariantClear(&var);
					videoStreamIndex = sidx;
					PropVariantInit(&var);

					// Get FPS
					if( SUCCEEDED(hr = MediaItem->GetStreamAttribute( sidx, MF_MT_FRAME_RATE, &var)) ) {
						ULONGLONG val;
						hr = PropVariantToUInt64( var, &val );
						if( SUCCEEDED(hr) ) {
							FPSNumerator = (unsigned long)((val>>32ULL)&0xffffffff);
							FPSDenominator = (unsigned long)(val&0xffffffff);
						}
						PropVariantClear(&var);
					}
					break;
				} else if( IsEqualGUID( *var.puuid, MFMediaType_Audio ) ) {
					// OutputDebugString( L"Audio" );
				}
			}
			PropVariantClear(&var);
		}
		hr = MediaItem->SetStreamSink( videoStreamIndex, pActivate );
		if( FAILED(hr) ) {
			TVPThrowExceptionMessage(L"Faild to set Video Sink.");
		}
	}
#endif
	/*
	HRESULT hr = tTVPAudioSessionVolume::CreateInstance( WM_AUDIO_EVENT, callbackwin, &AudioVolume );
	if( SUCCEEDED(hr) ) {
		// Ask for audio session events.
		hr = AudioVolume->EnableNotifications(TRUE);
	}

	if( FAILED(hr) ) {
		if( AudioVolume ) {
			AudioVolume->Release();
			AudioVolume = NULL;
		}
	}
	*/
	return hr;
}

HRESULT tTVPMFPlayer::AddBranchToPartialTopology( IMFTopology *pTopology, IMFMediaSource *pSource, IMFPresentationDescriptor *pPD, DWORD iStream, HWND hVideoWnd ) {
	CComPtr<IMFStreamDescriptor>	pSD;
	HRESULT hr;
	BOOL selected = FALSE;
    if( FAILED(hr = pPD->GetStreamDescriptorByIndex(iStream, &selected, &pSD)) ) {
		TVPThrowExceptionMessage(L"Faild to get stream desc.");
	}
	if( selected ) {
		// Create the media sink activation object.
		CComPtr<IMFActivate> pSinkActivate;
		if( FAILED(hr = CreateMediaSinkActivate(pSD, hVideoWnd, &pSinkActivate)) ) {
			return S_OK;	// video �� audio �ȊO�͖���
		}
		// Add a source node for this stream.
		CComPtr<IMFTopologyNode> pSourceNode;
        if( FAILED(hr = AddSourceNode(pTopology, pSource, pPD, pSD, &pSourceNode) ) ) {
			TVPThrowExceptionMessage(L"Faild to add source node.");
		}
		// Create the output node for the renderer.
		CComPtr<IMFTopologyNode> pOutputNode;
        if( FAILED(hr = AddOutputNode(pTopology, pSinkActivate, 0, &pOutputNode)) ) {
			TVPThrowExceptionMessage(L"Faild to add output node.");
		}
		// Connect the source node to the output node.
        if( FAILED(hr = pSourceNode->ConnectOutput(0, pOutputNode, 0)) ) {
			TVPThrowExceptionMessage(L"Faild to connect node.");
		}
	}
	return hr;
}
HRESULT tTVPMFPlayer::CreateMediaSinkActivate( IMFStreamDescriptor *pSourceSD, HWND hVideoWindow, IMFActivate **ppActivate ) {
	HRESULT hr;
	CComPtr<IMFMediaTypeHandler> pHandler;
	// Get the media type handler for the stream.
    if( FAILED(hr = pSourceSD->GetMediaTypeHandler(&pHandler)) ) {
		TVPThrowExceptionMessage(L"Faild to get media type handler.");
	}
	// Get the major media type.
    GUID guidMajorType;
    if( FAILED(hr = pHandler->GetMajorType(&guidMajorType)) ) {
		TVPThrowExceptionMessage(L"Faild to get major type.");
	}
    CComPtr<IMFActivate>		pActivate;
	if( MFMediaType_Audio == guidMajorType ) {
		// Create the audio renderer.
        if( FAILED(hr = MFCreateAudioRendererActivate(&pActivate) )) {
			TVPThrowExceptionMessage(L"Faild to create audio render.");
		}
	} else if( MFMediaType_Video == guidMajorType ) {
		// Get FPS
		CComPtr<IMFMediaType> pMediaType;
		if( SUCCEEDED(hr = pHandler->GetCurrentMediaType(&pMediaType)) ) {
			hr = MFGetAttributeRatio( pMediaType, MF_MT_FRAME_RATE, &FPSNumerator, &FPSDenominator );
		}

        // Create the video renderer.
        if( FAILED(hr = MFCreateVideoRendererActivate(hVideoWindow, &pActivate) ) ) {
			TVPThrowExceptionMessage(L"Faild to create video render.");
		}
		// TODO �����ŃJ�X�^��EVR���Ȃ��悤�ɂ���Ǝ��O�ŐF�X�`��ł���悤�ɂȂ�
		// ����͕W���̂��̂��g���Ă���
#if 0
		tTVPEVRCustomPresenter* my_activate_obj = new tTVPEVRCustomPresenter(hr);
		my_activate_obj->AddRef();
		CComPtr<IUnknown> unk;
		my_activate_obj->QueryInterface( IID_IUnknown, (void**)&unk );
		if( FAILED(hr = pActivate->SetUnknown(MF_ACTIVATE_CUSTOM_VIDEO_PRESENTER_ACTIVATE, unk)) ) {
			my_activate_obj->Release();
			TVPThrowExceptionMessage(L"Faild to add custom EVR presenter video render.");
		}
		my_activate_obj->Release();
#endif
	} else {
		hr = E_FAIL;
	}
	if( SUCCEEDED(hr) ) {
		// Return IMFActivate pointer to caller.
		*ppActivate = pActivate;
		(*ppActivate)->AddRef();
	}
	return hr;
}
HRESULT tTVPMFPlayer::AddSourceNode( IMFTopology *pTopology, IMFMediaSource *pSource, IMFPresentationDescriptor *pPD, IMFStreamDescriptor *pSD, IMFTopologyNode **ppNode ) {
	HRESULT hr;
	// Create the node.
	CComPtr<IMFTopologyNode> pNode;
    if( FAILED(hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &pNode)) ) {
		TVPThrowExceptionMessage(L"Faild to create source node.");
	}
	// Set the attributes.
    if( FAILED(hr = pNode->SetUnknown(MF_TOPONODE_SOURCE, pSource) ) ) {
		TVPThrowExceptionMessage(L"Faild to set source node.");
	}
	if( FAILED(hr = pNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, pPD) ) ) {
		TVPThrowExceptionMessage(L"Faild to set presentation desc.");
	}
	if( FAILED(hr = pNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, pSD)) ) {
		TVPThrowExceptionMessage(L"Faild to set stream desc.");
	}
	// Add the node to the topology.
    if( FAILED(hr = pTopology->AddNode(pNode)) ) {
		TVPThrowExceptionMessage(L"Faild to add source node to topology.");
	}
	// Return the pointer to the caller.
    *ppNode = pNode;
    (*ppNode)->AddRef();

	return hr;
}
HRESULT tTVPMFPlayer::AddOutputNode( IMFTopology *pTopology, IMFActivate *pActivate, DWORD dwId, IMFTopologyNode **ppNode ) {
	HRESULT hr;
    // Create the node.
    CComPtr<IMFTopologyNode> pNode;
    if( FAILED(hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pNode)) ){
		TVPThrowExceptionMessage(L"Faild to create output node.");
	}
    // Set the object pointer.
    if( FAILED(hr = pNode->SetObject(pActivate)) ) {
		TVPThrowExceptionMessage(L"Faild to set activate.");
	}
    // Set the stream sink ID attribute.
    if( FAILED(hr = pNode->SetUINT32(MF_TOPONODE_STREAMID, dwId)) ) {
		TVPThrowExceptionMessage(L"Faild to set stream id.");
	}
	if( FAILED(hr = pNode->SetUINT32(MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, FALSE)) ) {
		TVPThrowExceptionMessage(L"Faild to set no shutdown on remove.");
	}
    // Add the node to the topology.
    if( FAILED(hr = pTopology->AddNode(pNode)) ) {
		TVPThrowExceptionMessage(L"Faild to add ouput node to topology.");
	}
    // Return the pointer to the caller.
    *ppNode = pNode;
    (*ppNode)->AddRef();

	return hr;
}
/*
HRESULT tTVPMFPlayer::AddOutputNode( IMFTopology *pTopology, IMFStreamSink *pStreamSink, IMFTopologyNode **ppNode ) {
	HRESULT hr;
	// Create the node.
	CComPtr<IMFTopologyNode> pNode;
    if( FAILED(hr = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pNode)) ) {
		TVPThrowExceptionMessage(L"Faild to create output node.");
	}
	// Set the object pointer.
	if( FAILED(hr = pNode->SetObject(pStreamSink)) ) {
		TVPThrowExceptionMessage(L"Faild to set stream sink.");
	}
	// Add the node to the topology.
	if( FAILED(hr = pTopology->AddNode(pNode))) {
		TVPThrowExceptionMessage(L"Faild to add output node.");
	}
	if( FAILED(hr = pNode->SetUINT32(MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, TRUE) ) ) {
		TVPThrowExceptionMessage(L"Faild to set no shutdown on remove.");
	}
	*ppNode = pNode;
    (*ppNode)->AddRef();

	return hr;
}
*/
//----------------------------------------------------------------------------
//! @brief	  	�C���^�[�t�F�C�X���������
//----------------------------------------------------------------------------
void __stdcall tTVPMFPlayer::ReleaseAll()
{
	if( PlayerCallback ) {
		delete PlayerCallback;
		PlayerCallback = NULL;
	}
	if( ByteStream ) {
		ByteStream->Release();
		ByteStream = NULL;
	}
	if( MediaItem ) {
		MediaItem->Release();
		MediaItem = NULL;
	}
	/*
	if( AudioVolume ) {
		AudioVolume->EnableNotifications(FALSE);
		AudioVolume->Release();
		AudioVolume = NULL;
	}
	*/
	if( VideoDisplayControl ) {
		VideoDisplayControl->Release();
		VideoDisplayControl = NULL;
	}
	if( MediaPlayer ) {
		MediaPlayer->Release();
		MediaPlayer = NULL;
	}
	if( Topology ) {
		Topology->Release();
		Topology = NULL;
	}
	if( MediaSession ) {
		MediaSession->Release();
		MediaSession = NULL;
	}
	if( Stream ) {
		Stream->Release();
		Stream = NULL;
	}
}
//----------------------------------------------------------------------------
void tTVPMFPlayer::NotifyError( HRESULT hr ) {
	TVPThrowExceptionMessage(L"MF Operation Error.",hr);
}
void tTVPMFPlayer::NotifyState( MFP_MEDIAPLAYER_STATE state ) {
	switch( state ) {
	case MFP_MEDIAPLAYER_STATE_STOPPED:
		VideoStatue = vsStopped;
		break;
	case MFP_MEDIAPLAYER_STATE_PLAYING:
		VideoStatue = vsPlaying;
		break;
	case MFP_MEDIAPLAYER_STATE_PAUSED:
		VideoStatue = vsPaused;
		break;
	case MFP_MEDIAPLAYER_STATE_SHUTDOWN:
		break;
	}
}
void tTVPMFPlayer::OnMediaItemCreated( MFP_MEDIAITEM_CREATED_EVENT* event ) {
	HRESULT hr = S_OK; 
	IUnknown *pMFT = NULL;
	HasVideo = true;
	if( (MediaPlayer != NULL) && (event->pMediaItem != NULL) ) {
        BOOL bHasVideo = FALSE, bIsSelected = FALSE;
		hr = event->pMediaItem->HasVideo(&bHasVideo, &bIsSelected);
		if( FAILED(hr) ) { goto done; }
		HasVideo = (bHasVideo!=FALSE) && (bIsSelected!=FALSE);
		hr = MediaPlayer->SetMediaItem(event->pMediaItem);
		if( FAILED(hr) ) { goto done; }
    }
done:
	if( FAILED(hr) ) {
		NotifyError(hr);
	}
	if( pMFT ) {
		pMFT->Release();
		pMFT = NULL;
	}
}
void tTVPMFPlayer::OnMediaItemSet( MFP_MEDIAITEM_SET_EVENT* event ) {
	HRESULT hr = S_OK;
	hr = event->header.hrEvent;
	if( FAILED(hr) ) { goto done; }
	if( event->pMediaItem ) {
		hr = event->pMediaItem->GetCharacteristics( &MediaIteamCap );
		if (FAILED(hr)) { goto done; }
	}
	// hr = MediaPlayer->Play();
done:
	if( FAILED(hr) ) {
		NotifyError(hr);
	}
}
void tTVPMFPlayer::OnRateSet( MFP_RATE_SET_EVENT* event ) {
	PlayRate = event->flRate;
}
void tTVPMFPlayer::OnPlayBackEnded( MFP_PLAYBACK_ENDED_EVENT* event ) {
}
void tTVPMFPlayer::OnStop( MFP_STOP_EVENT* event ) {
}
void tTVPMFPlayer::OnPlay( MFP_PLAY_EVENT* event ) {
}
void tTVPMFPlayer::OnPause( MFP_PAUSE_EVENT* event ) {
}
void tTVPMFPlayer::OnPositionSet( MFP_POSITION_SET_EVENT* event ) {
}
void tTVPMFPlayer::OnFremeStep( MFP_FRAME_STEP_EVENT* event ) {
}
void tTVPMFPlayer::OnMediaItemCleared( MFP_MEDIAITEM_CLEARED_EVENT* event ) {
}
void tTVPMFPlayer::OnMF( MFP_MF_EVENT* event ) {
}
void tTVPMFPlayer::OnError( MFP_ERROR_EVENT* event ) {
}
void tTVPMFPlayer::OnAcquireUserCredential( MFP_ACQUIRE_USER_CREDENTIAL_EVENT* event ) {
}
//----------------------------------------------------------------------------
void __stdcall tTVPMFPlayer::SetWindow(HWND window) {
	HRESULT hr = E_FAIL;
	OwnerWindow = window;
	if( VideoDisplayControl ) {
		hr = VideoDisplayControl->SetVideoWindow( window );
		if( FAILED(hr) ) {
			TVPThrowExceptionMessage(L"Faild to call SetVideoWindow.");
		}
	}
	CreateVideoPlayer( window );
}
void __stdcall tTVPMFPlayer::SetMessageDrainWindow(HWND window) {
	CallbackWindow = window;
}
void __stdcall tTVPMFPlayer::SetRect(RECT *rect) {
	if( VideoDisplayControl ) {
		// MF �ł́A�\�[�X��`���w��\�ɂȂ��Ă���
		HRESULT hr = VideoDisplayControl->SetVideoPosition( NULL, rect );
		if( FAILED(hr) ) {
			TVPThrowExceptionMessage(L"Faild to set rect.");
		}
	}
}
void __stdcall tTVPMFPlayer::SetVisible(bool b) {
}
void __stdcall tTVPMFPlayer::Play() {
	HRESULT hr = E_FAIL;
	if( MediaSession ) {
		PROPVARIANT varStart;
		PropVariantInit(&varStart);
		hr = MediaSession->Start( &GUID_NULL, &varStart );
		PropVariantClear(&varStart);
	}
	if( MediaPlayer ) {
		hr = MediaPlayer->Play();
	}
	if( FAILED(hr) ) {
		TVPThrowExceptionMessage(L"Faild to play.");
	}
}
void __stdcall tTVPMFPlayer::Stop() {
	HRESULT hr = E_FAIL;
	if( MediaSession ) {
		MediaSession->Stop();
	}
	if( MediaPlayer ) {
		hr = MediaPlayer->Stop();
	}
	if( FAILED(hr) ) {
		TVPThrowExceptionMessage(L"Faild to stop.");
	}
}
void __stdcall tTVPMFPlayer::Pause() {
	HRESULT hr = E_FAIL;
	if( MediaPlayer ) {
		hr = MediaPlayer->Pause();
	}
	if( FAILED(hr) ) {
		TVPThrowExceptionMessage(L"Faild to stop.");
	}
}
void __stdcall tTVPMFPlayer::SetPosition(unsigned __int64 tick) {
	HRESULT hr = E_FAIL;
	PROPVARIANT var;
	PropVariantInit(&var);
	if( MediaPlayer ) {
		var.vt = VT_I8;
		var.hVal.QuadPart = tick;
		HRESULT hr = MediaPlayer->SetPosition( MFP_POSITIONTYPE_100NS, &var );
	}
	PropVariantClear(&var);
	if( FAILED(hr) ) {
		TVPThrowExceptionMessage(L"Faild to set position.");
	}
}
void __stdcall tTVPMFPlayer::GetPosition(unsigned __int64 *tick) {
	HRESULT hr = E_FAIL;
	PROPVARIANT var;
	PropVariantInit(&var);
	if( MediaPlayer ) {
		hr = MediaPlayer->GetPosition( MFP_POSITIONTYPE_100NS, &var );
		if( SUCCEEDED(hr) ) {
			*tick = var.hVal.QuadPart;
		}
	}
	PropVariantClear(&var);
	if( FAILED(hr) ) {
		TVPThrowExceptionMessage(L"Faild to get position.");
	}
}
void __stdcall tTVPMFPlayer::GetStatus(tTVPVideoStatus *status) {
	if( status ) {
		*status = VideoStatue;
	}
}
void __stdcall tTVPMFPlayer::GetEvent(long *evcode, long *param1, long *param2, bool *got) {
	*got = false;
}
void __stdcall tTVPMFPlayer::FreeEventParams(long evcode, long param1, long param2) {
}
void __stdcall tTVPMFPlayer::Rewind() {
	SetPosition( 0 );
}
void __stdcall tTVPMFPlayer::SetFrame( int f ) {
	UINT64 avgTime;
	HRESULT hr = MFFrameRateToAverageTimePerFrame( FPSNumerator, FPSDenominator, &avgTime );
	if( SUCCEEDED(hr) ) {
		LONGLONG requestTime = avgTime * (LONGLONG)f;
		SetPosition( requestTime );
	}
}
void __stdcall tTVPMFPlayer::GetFrame( int *f ) {
	UINT64 avgTime;
	HRESULT hr = MFFrameRateToAverageTimePerFrame( FPSNumerator, FPSDenominator, &avgTime );
	if( SUCCEEDED(hr) ) {
		unsigned __int64 tick;
		GetPosition( &tick );
		*f = (int)( tick / avgTime );
	}
}
void __stdcall tTVPMFPlayer::GetFPS( double *f ) {
	*f = (double)FPSNumerator / (double)FPSDenominator;
}
void __stdcall tTVPMFPlayer::GetNumberOfFrame( int *f ) {
	UINT64 avgTime;
	HRESULT hr = MFFrameRateToAverageTimePerFrame( FPSNumerator, FPSDenominator, &avgTime );
	if( SUCCEEDED(hr) ) {
		long long t;
		GetTotalTime( &t );
		*f = (int)( t / avgTime );
	}
}
void __stdcall tTVPMFPlayer::GetTotalTime( __int64 *t ) {
	HRESULT hr = E_FAIL;
	PROPVARIANT var;
	PropVariantInit(&var);
	if( MediaPlayer ) {
		hr = MediaPlayer->GetDuration( MFP_POSITIONTYPE_100NS, &var );
		if( SUCCEEDED(hr) ) {
			*t = var.uhVal.QuadPart;
		}
	}
	PropVariantClear(&var);
	if( FAILED(hr) ) {
		TVPThrowExceptionMessage(L"Faild to get total time.");
	}
}

void __stdcall tTVPMFPlayer::GetVideoSize( long *width, long *height ){
	HRESULT hr = E_FAIL;
	if( VideoDisplayControl ) {
		SIZE vsize;
		hr = VideoDisplayControl->GetNativeVideoSize( &vsize, NULL );
		if( FAILED(hr) ) {
			TVPThrowExceptionMessage(L"Faild to get video size.");
		}
		*width = vsize.cx;
		*height = vsize.cy;
	}
}
void __stdcall tTVPMFPlayer::GetFrontBuffer( BYTE **buff ){
}
void __stdcall tTVPMFPlayer::SetVideoBuffer( BYTE *buff1, BYTE *buff2, long size ){
}

void __stdcall tTVPMFPlayer::SetStopFrame( int frame ) {
}
void __stdcall tTVPMFPlayer::GetStopFrame( int *frame ) {
}
void __stdcall tTVPMFPlayer::SetDefaultStopFrame() {
}

void __stdcall tTVPMFPlayer::SetPlayRate( double rate ) {
	HRESULT hr = E_FAIL;
	if( MediaPlayer ) {
		hr = MediaPlayer->SetRate( (float)rate );
	}
	if( FAILED(hr) ) {
		TVPThrowExceptionMessage(L"Faild to set play rate.");
	}
}
void __stdcall tTVPMFPlayer::GetPlayRate( double *rate ) {
	HRESULT hr = E_FAIL;
	if( MediaPlayer ) {
		float playrate;
		hr = MediaPlayer->GetRate( &playrate );
		if( SUCCEEDED(hr) ) {
			*rate = playrate;
		}
	}
	if( FAILED(hr) ) {
		TVPThrowExceptionMessage(L"Faild to get play rate.");
	}
}

void __stdcall tTVPMFPlayer::SetAudioBalance( long balance ) {
	HRESULT hr = E_FAIL;
	if( MediaPlayer ) {
		float b = balance / 10000.0f;
		hr = MediaPlayer->SetBalance( b );
	}
	if( FAILED(hr) ) {
		TVPThrowExceptionMessage(L"Faild to set audio balance.");
	}
}
void __stdcall tTVPMFPlayer::GetAudioBalance( long *balance ) {
	HRESULT hr = E_FAIL;
	if( MediaPlayer ) {
		float b;
		hr = MediaPlayer->GetBalance( &b );
		if( SUCCEEDED(hr) ) {
			*balance = (long)( b*10000.0f );
		}
	}
	if( FAILED(hr) ) {
		TVPThrowExceptionMessage(L"Faild to get audio balance.");
	}
}
void __stdcall tTVPMFPlayer::SetAudioVolume( long volume ) {
	HRESULT hr = E_FAIL;
	if( MediaPlayer ) {
		if( volume <= -10000  ) {
			hr = MediaPlayer->SetVolume( 0.0f );
		} else {
			hr = MediaPlayer->SetVolume( (10000 + volume)/10000.0f );
		}
	}
	if( FAILED(hr) ) {
		TVPThrowExceptionMessage(L"Faild to set audio volume.");
	}
}
void __stdcall tTVPMFPlayer::GetAudioVolume( long *volume ) {
	HRESULT hr = E_FAIL;
	float vol = 0.0f;
	if( MediaPlayer ) {
		hr = MediaPlayer->GetVolume( &vol );
	}
	if( FAILED(hr) ) {
		TVPThrowExceptionMessage(L"Faild to get audio volume.");
	} else {
		*volume = 10000 - (long)(vol*10000);
	}
}

void __stdcall tTVPMFPlayer::GetNumberOfAudioStream( unsigned long *streamCount ){
}
void __stdcall tTVPMFPlayer::SelectAudioStream( unsigned long num ){
}
void __stdcall tTVPMFPlayer::GetEnableAudioStreamNum( long *num ){
}
void __stdcall tTVPMFPlayer::DisableAudioStream( void ){
}

void __stdcall tTVPMFPlayer::GetNumberOfVideoStream( unsigned long *streamCount ){
}
void __stdcall tTVPMFPlayer::SelectVideoStream( unsigned long num ){
}
void __stdcall tTVPMFPlayer::GetEnableVideoStreamNum( long *num ){
}

void __stdcall tTVPMFPlayer::SetMixingBitmap( HDC hdc, RECT *dest, float alpha ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::ResetMixingBitmap() {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::SetMixingMovieAlpha( float a ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::GetMixingMovieAlpha( float *a ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::SetMixingMovieBGColor( unsigned long col ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::GetMixingMovieBGColor( unsigned long *col ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::PresentVideoImage() {
}
void __stdcall tTVPMFPlayer::GetContrastRangeMin( float *v ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::GetContrastRangeMax( float *v ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::GetContrastDefaultValue( float *v ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::GetContrastStepSize( float *v ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::GetContrast( float *v ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::SetContrast( float v ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::GetBrightnessRangeMin( float *v ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::GetBrightnessRangeMax( float *v ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::GetBrightnessDefaultValue( float *v ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::GetBrightnessStepSize( float *v ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::GetBrightness( float *v ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::SetBrightness( float v ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::GetHueRangeMin( float *v ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::GetHueRangeMax( float *v ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::GetHueDefaultValue( float *v ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::GetHueStepSize( float *v ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::GetHue( float *v ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::SetHue( float v ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::GetSaturationRangeMin( float *v ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::GetSaturationRangeMax( float *v ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::GetSaturationDefaultValue( float *v ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::GetSaturationStepSize( float *v ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::GetSaturation( float *v ) {
	/* �������Ȃ� */
}
void __stdcall tTVPMFPlayer::SetSaturation( float v ) {
	/* �������Ȃ� */
}


