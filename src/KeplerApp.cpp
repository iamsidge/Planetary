#include <vector>
#include <cmath>

#include "cinder/app/cocoa/AppCocoaTouch.h"
#include "cinder/app/RendererGl.h"
#include "glm/gtx/rotate_vector.hpp"
#include "cinder/app/Renderer.h"
#include "cinder/Surface.h"
#include "cinder/gl/Texture.h"
#include "cinder/Camera.h"
#include "cinder/Font.h"
#include "cinder/Arcball.h"
#include "cinder/ImageIo.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "cinder/Perlin.h"
#include "cinder/Easing.h"

#include "MusicBackend.h"

#include "OrientationHelper.h"
#include "GyroHelper.h"
#include "Device.h"

#include "Globals.h"

#include "State.h"
#include "Data.h"
#include "PlaylistFilter.h"
#include "LetterFilter.h"

#include "World.h"
#include "NodeArtist.h"
#include "NodeAlbum.h"
#include "Galaxy.h"
#include "BloomSphere.h"
#include "PlanetLighting.h"

#include "BloomScene.h"
#include "OrientationNode.h"
#include "LoadingScreen.h"
#include "HelpLayer.h"
#include "NotificationOverlay.h"
#include "Vignette.h"
#include "Stats.h"
#include "UiLayer.h"
#include "PlayControls.h"
#include "SettingsPanel.h"
#include "AlphaChooser.h"

#include "PlaylistChooser.h"
#include "PinchRecognizer.h"
#include "ParticleController.h"
#include "TextureLoader.h"
#include "TaskQueue.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace bloom;

float G_ZOOM			= 0;
int G_CURRENT_LEVEL		= 0;
bool G_DEBUG			= false;
bool G_AUTO_MOVE		= false;
bool G_SHOW_SETTINGS	= false;
bool G_DRAW_RINGS		= true;
bool G_DRAW_TEXT		= true;
bool G_USE_GYRO			= false;
bool G_IS_IPAD2			= false;
int G_NUM_PARTICLES		= 25;
int G_NUM_DUSTS			= 250;

class KeplerApp : public AppCocoaTouch {
  public:
    
	virtual void	setup();
    void            prepareSettings(Settings *settings);
    void            remainingSetup();
	void			initLoadingTextures();
	void			initTextures();
    void            onTextureLoaderComplete( TextureLoader* );

	virtual void	touchesBegan( TouchEvent event );
	virtual void	touchesMoved( TouchEvent event );
	virtual void	touchesEnded( TouchEvent event );

	bool            orientationChanged( OrientationEvent event );
    void            setInterfaceOrientation( const Orientation &orientation );
	
    virtual void	update();
	void			updateArcball();
	void			updateCamera();

	virtual void	draw();
	void			drawNoArtists();
    void            drawScene();

    // analytics hooks (see KeplerApp.cpp)
    void            logEvent(const string &event);
    void            logEvent(const string &event, const map<string,string> &params);
    
	bool			onAlphaCharStateChanged( char c );
	bool			onPlaylistStateChanged( music::PlaylistRef playlist );
	bool			onAlphaCharSelected( char c );
	bool			onVignetteToggled( bool on );
    bool            onFilterModeStateChanged( State::FilterMode filterMode );
    bool            onPlaylistChooserSelected( music::PlaylistRef );
    bool            onPlaylistChooserTouched( music::PlaylistRef );
    
	bool			onSettingsPanelButtonPressed ( BloomSceneEventRef event );
	bool			onPlayControlsButtonPressed ( BloomSceneEventRef event );
    void            togglePlayPaused();
	bool			onPlayControlsPlayheadMoved ( BloomSceneEventRef event );
    void            flyToCurrentTrack();
    void            flyToCurrentAlbum();
    void            flyToCurrentArtist();
	
    bool			onSelectedNodeChanged( Node *node );

	void			checkForNodeTouch( const Ray &ray, const vec2 &pos );
	
    bool			onPlayerStateChanged( music::Player *player );
    bool			onPlayerTrackChanged( music::Player *player );
    bool			onPlayerLibraryChanged( music::Player *player );
	
// UI BITS:
    BloomSceneRef       mBloomSceneRef;
    OrientationNodeRef  mOrientationNodeRef;
    LoadingScreen       mLoadingScreen;
    BloomNodeRef        mMainBloomNodeRef;
	UiLayer             mUiLayer;
    PlayControls        mPlayControls;
    SettingsPanel       mSettingsPanel;
	HelpLayer		    mHelpLayer;
    NotificationOverlay mNotificationOverlay;

    Vignette            mVignette;
    AlphaChooser        mAlphaChooser;
    PlaylistChooser     mPlaylistChooser;

// PERLIN BITS:
	Perlin				mPerlin;
	
// 3D BITS:
	World               mWorld;
    
// DATA/TRUTH BITS:
	State               mState;
	Data                mData;

// ORIENTATION
    OrientationHelper mOrientationHelper;    
    Orientation       mInterfaceOrientation;
    mat4         mOrientationMatrix;
    mat4         mInverseOrientationMatrix;    
    
// GYRO
    GyroHelper			mGyroHelper;
	quat				mPrevGyro;
    
// AUDIO
	music::Player		mMusicPlayer;
    music::TrackRef      mPlayingTrack;
    double              mCurrentTrackLength; // cached by onPlayerTrackChanged
    double              mCurrentTrackPlayheadTime;
    double              mPlayheadUpdateSeconds;
    music::Player::State mCurrentPlayState;
		
// CAMERA PERSP
	CameraPersp		mCam;
	
	float			mFov, mFovDest;
	vec3			mEye, mCenter, mUp;
	vec3			mCenterDest, mCenterFrom;
	vec3			mCenterOffset;		// if pinch threshold is exceeded, look towards parent?
	float			mCamDist, mCamDistDest, mCamDistFrom;
	float			mPinchPerInit;
	float			mPinchPer;			// 0.0 (max pinched) to 1.0 (max spread)
	float			mPinchTotalDest;	// accumulation of pinch deltas
	float			mPinchTotal;		// eases to pinchTotalDest
	float			mPinchTotalInit;	// reset value of pinchTotalDest
	float			mPinchScaleMax, mPinchScaleMin;
	float			mPinchPerThresh;
	float			mPinchPerFrom, mPinchTotalFrom;	// used to decelerate values post new node selection
	float			mPinchHighlightRadius;
	float			mPinchRotation;
	bool			mIsPastPinchThresh;
	float			mInteractionThreshold;
	float			mPerlinForAutoMove;
	float			mAutoMoveScale;
	
	
	float			mZoomFrom, mZoomDest;
	Arcball			mArcball;
	float			mCamRingAlpha; // 1.0 = camera is viewing rings side-on
								   // 0.0 = camera is viewing rings from above or below
	float			mFadeInAlphaToArtist;
	float			mFadeInArtistToAlbum;
	float			mFadeInAlbumToTrack;
	
// FONTS
	Font			mFontMedium, mFontBig, mFontHuge, mFontSmall; // Aaux
	Font			mFontMedi, mFontMediSmall, mFontMediTiny, mFontMediBig; // Unit Medi
    Font            mFontUltraBig; // Unit Ultra
	
// MULTITOUCH
	vec2			mTouchPos;
	vec2			mTouchVel;
	bool			mIsDragging;
    bool            mIsTouching; // set true in touchesBegan if there's one touch and it's in-world
	bool			onPinchBegan( PinchEvent event );
    bool			onPinchMoved( PinchEvent event );
    bool			onPinchEnded( PinchEvent event );
    bool            keepTouchForPinching( TouchEvent::Touch touch );
    bool            positionTouchesWorld( vec2 pos );
    vector<Ray>		mPinchRays;
	vector<vec2>	mPinchPositions;
    PinchRecognizer mPinchRecognizer;
	float			mTimePinchEnded;
	float			mPinchAlphaPer;
	bool			mIsPinching;
	
	
// PARTICLES
    ParticleController mParticleController;

// STATS
//    Stats mStats;
    
// TEXTURES
    
    // texture IDs for threaded loading
    enum TextureId { 
        STAR_TEX, 
        STAR_CORE_TEX, 
        ECLIPSE_GLOW_TEX, 
        LENS_FLARE_TEX,
        ECLIPSE_SHADOW_TEX,
        SKY_DOME_TEX,
        GALAXY_DOME_TEX,
        DOTTED_TEX,
        PLAYHEAD_PROGRESS_TEX,
        RINGS_TEX,
        UI_BUTTONS_TEX,
        ATMOSPHERE_TEX,
        ATMOSPHERE_DIRECTIONAL_TEX,
        ATMOSPHERE_SUN_TEX,
        PARTICLE_TEX,
        GALAXY_TEX,
        DARK_MATTER_TEX,
        ORBIT_RING_GRADIENT_TEX,
        TRACK_ORIGIN_TEX,
		GRADIENT_OVERLAY_TEX,
        P_CLOUDS_1,
        P_CLOUDS_2,
        P_CLOUDS_3,
        P_CLOUDS_4,
        P_CLOUDS_5,
        M_CLOUDS_1,
        M_CLOUDS_2,
        M_CLOUDS_3,
        M_CLOUDS_4,
        M_CLOUDS_5,
        TOTAL_TEXTURE_COUNT // must be last :)
    };
    
    // manages threaded texture loading by ID (above)
    TextureLoader mTextures;
    
    // groups cloud textures together
    vector<gl::TextureRef> mCloudTextures;
	
    // needed for load screen:
    gl::TextureRef mPlanetaryTex, mPlanetTex, mBackgroundTex, mStarGlowTex;
    
    // loaded on demand for empty music libraries
	gl::TextureRef		mNoArtistsTex;    
    
    // FIXME: load these in a thread sometime
	Surface			mHighResSurfaces;
	Surface			mLowResSurfaces;
	Surface			mNoAlbumArtSurface;
	
// SKYDOME
    BloomSphere mSkySphere;
    
// GALAXY
    Galaxy mGalaxy;
	
	float			mSelectionTime;
    bool            mRemainingSetupCalled; // setup() is short and fast, remainingSetup() is slow
    bool            mUiComplete;
    std::string     mLastPlayerMessage;
#if defined( PLANETARY_DEBUG_AUTOSELECT )
    bool            mDebugDidAutoSelect;
    bool            mDebugDidSelectAlbum;
    bool            mDebugDidSelectTrack;
#endif
};

void KeplerApp::prepareSettings(Settings *settings)
{
#ifdef DEBUG
    // "Kepler" ID:
#else
    // "Planetary" ID:
#endif
    
    // start requesting events ASAP
    mOrientationHelper.setup();
}

void KeplerApp::setup()
{
//    float t = getElapsedSeconds();
    
    
    mRemainingSetupCalled = false;
    mUiComplete = false;
#if defined( PLANETARY_DEBUG_AUTOSELECT )
    mDebugDidAutoSelect  = false;
    mDebugDidSelectAlbum = false;
    mDebugDidSelectTrack = false;
#endif
	mState.setup();
    
    mState.setup();
    
    G_IS_IPAD2 = bloom::isIpad2();
//    console() << "G_IS_IPAD2: " << G_IS_IPAD2 << endl;

	if( G_IS_IPAD2 ){
		G_NUM_PARTICLES = 80;
		G_NUM_DUSTS = 5000;
        mGyroHelper.setup();
	}
	
    mOrientationHelper.registerOrientationChanged( this, &KeplerApp::orientationChanged );    
    setInterfaceOrientation( mOrientationHelper.getInterfaceOrientation() );
    
    // initialize controller for all 2D UI
    // this will receive touch events before anything else (so it can cancel them before they hit the world)
    mBloomSceneRef = BloomScene::create( this );
    
    mOrientationNodeRef = OrientationNode::create( &mOrientationHelper );
    mBloomSceneRef->addChild( mOrientationNodeRef );

    // load textures (synchronously) for LoadingScreen
    initLoadingTextures();    

    // !!! this has to be set up before any other UI things so it can consume touch events
    mLoadingScreen.setup( mPlanetaryTex, mPlanetTex, mBackgroundTex, mStarGlowTex );
    mOrientationNodeRef->addChild( BloomNodeRef(&mLoadingScreen) );
    
    // make a container for all the other UI, so visibility can be toggled when loading
    mMainBloomNodeRef = BloomNodeRef( new BloomNode() );
    mOrientationNodeRef->addChild( mMainBloomNodeRef );
    mMainBloomNodeRef->setVisible(false);    
    
    
//    std::cout << (getElapsedSeconds() - t) << " seconds to setup()" << std::endl;
}

void KeplerApp::remainingSetup()
{
    if (mRemainingSetupCalled) return;
    
    mRemainingSetupCalled = true;

//    float t = getElapsedSeconds();
    

	mLoadingScreen.setVisible( true );
    
    // DATA ... is asynchronous, see update() for what happens when it's done.
    // The backend may need to sign in first: querying Spotify before that
    // succeeds returns nothing, which would look like an empty music library
    // rather than a sign-in that has not happened yet.
    planetary::ensureMusicAccess( [this]( bool ok, const std::string &message ) {
        if( ok ) {
            mData.setup();
        }
        else {
            console() << "Planetary: music backend unavailable: " << message << std::endl;
            // Still start the loader, so the app reaches its normal empty
            // state instead of sitting on the loading screen forever.
            mData.setup();
        }
    } );
    
    // TEXTURES ... also mostly asynchronous
    initTextures();
    
//    std::cout << (getElapsedSeconds() - t) << " seconds to remainingSetup()" << std::endl;
}

void KeplerApp::initTextures()
{
    
    // FONTS
    //   Note to would-be optimizers: loadResource is fairly fast (~7ms for 5 fonts)
    //   ...it's Font() that's slow (~50ms per font?)
    DataSourceRef aux   = loadResource( "AauxPro-Black.ttf");
	mFontHuge			= Font( aux, 100 );
	mFontSmall			= Font( aux, 16 );
	mFontMedium			= Font( aux, 18 );
	mFontBig            = Font( aux, 24 );

    DataSourceRef medi  = loadResource( "UnitRoundedOT-Medi.otf" );
	mFontMedi           = Font( medi, 14 );
	mFontMediSmall		= Font( medi, 13 );
	mFontMediTiny		= Font( medi, 11 );
    mFontMediBig        = Font( medi, 24 );
                               
    DataSourceRef ultra = loadResource( "UnitRoundedOT-Ultra.otf" );                               
    mFontUltraBig       = Font( ultra, 24 );

    //////////////
    
    gl::Texture::Format mipFmt;
    mipFmt.enableMipmapping( true );
    mipFmt.setMinFilter( GL_LINEAR_MIPMAP_LINEAR );    
    mipFmt.setMagFilter( GL_LINEAR ); // TODO: experiment with GL_NEAREST where appropriate
    
    gl::Texture::Format repeatMipFmt;
    repeatMipFmt.enableMipmapping( true );
    repeatMipFmt.setMinFilter( GL_LINEAR_MIPMAP_LINEAR );    
    repeatMipFmt.setMagFilter( GL_LINEAR ); // TODO: experiment with GL_NEAREST where appropriate
    repeatMipFmt.setWrap( GL_REPEAT, GL_REPEAT );    

    //////////
    
    // TODO: can we load these on a thread some day?
    mHighResSurfaces   = Surface( loadImage( loadResource( "surfacesHighRes.png" ) ) );
	mLowResSurfaces	   = Surface( loadImage( loadResource( "surfacesLowRes.png" ) ) );
    mNoAlbumArtSurface = Surface( loadImage( loadResource( "noAlbumArt.png" ) ) );
    
    //////////
    
	mTextures.addRequest( P_CLOUDS_1, "planetClouds1.png" );    
	mTextures.addRequest( P_CLOUDS_2, "planetClouds2.png" );    
	mTextures.addRequest( P_CLOUDS_3, "planetClouds3.png" );    
	mTextures.addRequest( P_CLOUDS_4, "planetClouds4.png" );    
	mTextures.addRequest( P_CLOUDS_5, "planetClouds5.png" );    
	mTextures.addRequest( M_CLOUDS_1, "moonClouds1.png" );    
	mTextures.addRequest( M_CLOUDS_2, "moonClouds2.png" );    
	mTextures.addRequest( M_CLOUDS_3, "moonClouds3.png" );    
	mTextures.addRequest( M_CLOUDS_4, "moonClouds4.png" );    
	mTextures.addRequest( M_CLOUDS_5, "moonClouds5.png" );    
    mTextures.addRequest( STAR_TEX,				"star.png",          mipFmt );
    mTextures.addRequest( STAR_CORE_TEX,		"starCore.png",      mipFmt );
    mTextures.addRequest( ECLIPSE_GLOW_TEX,		"eclipseGlow.png",   mipFmt );
    mTextures.addRequest( LENS_FLARE_TEX,		"lensFlare.png",     mipFmt );
    mTextures.addRequest( ECLIPSE_SHADOW_TEX,	"eclipseShadow.png", mipFmt );
    mTextures.addRequest( SKY_DOME_TEX,			"skydomeFull.png",     mipFmt );
    mTextures.addRequest( GALAXY_DOME_TEX,		"lightMatterFull.jpg", repeatMipFmt );      
    mTextures.addRequest( DOTTED_TEX,                 "dotted.png",           repeatMipFmt );
    mTextures.addRequest( PLAYHEAD_PROGRESS_TEX,      "playheadProgress.png", repeatMipFmt );
    mTextures.addRequest( RINGS_TEX,                  "rings.png" );
    mTextures.addRequest( UI_BUTTONS_TEX,			  "uiButtons.png" );
    mTextures.addRequest( ATMOSPHERE_TEX,             "atmosphere.png",            mipFmt );
    mTextures.addRequest( ATMOSPHERE_DIRECTIONAL_TEX, "atmosphereDirectional.png", mipFmt );
    mTextures.addRequest( ATMOSPHERE_SUN_TEX,         "atmosphereSun.png",         mipFmt );
    mTextures.addRequest( PARTICLE_TEX,               "particle.png",              mipFmt );
    mTextures.addRequest( GALAXY_TEX,				"galaxyCropped.jpg", mipFmt );
    mTextures.addRequest( DARK_MATTER_TEX,			"darkMatterFull.png", repeatMipFmt );
    mTextures.addRequest( ORBIT_RING_GRADIENT_TEX,  "orbitRingGradient.png", mipFmt );
    mTextures.addRequest( TRACK_ORIGIN_TEX,         "origin.png" );
	mTextures.addRequest( GRADIENT_OVERLAY_TEX,		"gradientLarge.png" );
    
    mTextures.registerComplete( this, &KeplerApp::onTextureLoaderComplete );
    mTextures.start();    
}

void KeplerApp::onTextureLoaderComplete( TextureLoader* loader )
{
//    float t = getElapsedSeconds();
    
	
    // CLOUD TEXTURE VECTOR
	mCloudTextures.push_back( mTextures[P_CLOUDS_1] );
    mCloudTextures.push_back( mTextures[P_CLOUDS_2] );
	mCloudTextures.push_back( mTextures[P_CLOUDS_3] );
	mCloudTextures.push_back( mTextures[P_CLOUDS_4] );
	mCloudTextures.push_back( mTextures[P_CLOUDS_5] );
	mCloudTextures.push_back( mTextures[M_CLOUDS_1] );
	mCloudTextures.push_back( mTextures[M_CLOUDS_2] );
	mCloudTextures.push_back( mTextures[M_CLOUDS_3] );
	mCloudTextures.push_back( mTextures[M_CLOUDS_4] );
	mCloudTextures.push_back( mTextures[M_CLOUDS_5] );
    
	// ARCBALL
	// 0.9's Arcball projects through a camera (which only the constructor can
	// set — &mCam is a stable member) and models a *world-space* sphere. 0.8's
	// was screen-space: centre at the window centre, radius in pixels. Feeding
	// those numbers to 0.9 made a sphere the camera could not sensibly
	// project, and mouseOnSphere's miss-fallback emitted NaN, which then stuck
	// in the quat forever. The sphere is kept centred on the camera target and
	// sized against the camera distance — see updateArcball.
	mArcball = Arcball( &mCam, Sphere( vec3( 0.0f ), G_INIT_CAM_DIST * 0.5f ) );
	// Cinder 0.8's Quatf(x,y,z) took Euler angles; GLM spells that quat(vec3).
	mArcball.setQuat( quat( vec3( -0.2f, 0.0f, -0.3f ) ) );
	
	// CAMERA PERSP
	mCamDist			= G_INIT_CAM_DIST;
	mCamDistDest		= mCamDist;
	mPinchPerInit		= 0.2f;
	mPinchPer			= mPinchPerInit;
	mPinchScaleMax		= 3.0f;
	mPinchScaleMin		= 0.5f;
	mPinchTotalInit		= ( mPinchScaleMax - mPinchScaleMin ) * mPinchPerInit + mPinchScaleMin;
	mPinchTotal			= mPinchTotalInit;
	mPinchTotalDest		= mPinchTotal;
	mPinchPerThresh		= 0.65f;
	mPinchPerFrom		= mPinchPer;
	mPinchTotalFrom		= mPinchTotal;
	mIsPinching			= false;
	mPinchHighlightRadius = 50.0f;
	mPinchRotation		= 0.0f;
	mIsPastPinchThresh	= false;
	mPerlinForAutoMove	= 0.0f;

	
	mCamDistFrom		= mCamDist;
	mEye				= vec3( 0.0f, 0.0f, mCamDist );
	mCenter				= vec3(0);
	mCenterDest			= mCenter;
	mCenterFrom			= mCenter;
	mCenterOffset		= mCenter;
	mUp					= vec3(0,1,0);
	mFov				= G_DEFAULT_FOV;
	mFovDest			= G_DEFAULT_FOV;
	mCam.setPerspective( mFov, getWindowAspectRatio(), 0.0001f, 1200.0f );
	mFadeInAlphaToArtist = 0.0f;
	mFadeInArtistToAlbum = 0.0f;
	mFadeInAlbumToTrack = 0.0f;
	
// STATS
//    mStats.setup( mFontMedi, BRIGHT_BLUE, BRIGHT_BLUE );
	
// TOUCH VARS
	mTouchPos			= getWindowCenter();
	mTouchVel			= vec2(2.1f, 0.3f );
	mIsDragging			= false;
    mIsTouching         = false;
	mSelectionTime		= getElapsedSeconds();
	mTimePinchEnded		= 0.0f;
	mPinchRecognizer.init(this);
    mPinchRecognizer.registerBegan( this, &KeplerApp::onPinchBegan );
    mPinchRecognizer.registerMoved( this, &KeplerApp::onPinchMoved );
    mPinchRecognizer.registerEnded( this, &KeplerApp::onPinchEnded );
    mPinchRecognizer.setKeepTouchCallback( this, &KeplerApp::keepTouchForPinching );
	mPinchAlphaPer		= 1.0f;
    
    // PARTICLES
    mParticleController.addParticles( G_NUM_PARTICLES );
	mParticleController.addDusts( G_NUM_DUSTS );

    // VIGNETTE
    mVignette.setup( mTextures[GRADIENT_OVERLAY_TEX] ); 
	mVignette.registerToggled( this, &KeplerApp::onVignetteToggled );    
    mMainBloomNodeRef->addChild( BloomNodeRef( &mVignette ) );
    
	// PLAY CONTROLS
	mPlayControls.setup( mBloomSceneRef->getInterfaceSize(), 
                         &mMusicPlayer, 
                         mFontMediSmall, mFontMediTiny, 
                         mTextures[UI_BUTTONS_TEX] );
	mPlayControls.registerTouchEnded( this, &KeplerApp::onPlayControlsButtonPressed );
	mPlayControls.registerTouchMoved( this, &KeplerApp::onPlayControlsPlayheadMoved );

	// SETTINGS PANEL
	mSettingsPanel.setup( mBloomSceneRef->getInterfaceSize(), 
                          &mMusicPlayer, 
                          mFontMediSmall, 
                          mTextures[UI_BUTTONS_TEX] );
	mSettingsPanel.registerTouchEnded( this, &KeplerApp::onSettingsPanelButtonPressed );
    mSettingsPanel.setVisible( G_SHOW_SETTINGS );
    // add as child of UILayer so it inherits the transform
    
	// ALPHA CHOOSER
	mAlphaChooser.setup( mFontMedium, mBloomSceneRef->getInterfaceSize() );
	mAlphaChooser.registerAlphaCharSelected( this, &KeplerApp::onAlphaCharSelected );
    mAlphaChooser.setVisible(false);
	
    // PLAYLIST CHOOSER
    mPlaylistChooser.setup( mFontSmall, mBloomSceneRef->getInterfaceSize() );
    mPlaylistChooser.registerPlaylistSelected( this, &KeplerApp::onPlaylistChooserSelected );
    mPlaylistChooser.registerPlaylistTouched( this, &KeplerApp::onPlaylistChooserTouched );
    mPlaylistChooser.setVisible(false);
    
    // refs!
    PlaylistChooserRef playlistChooserRef = PlaylistChooserRef(&mPlaylistChooser);
    AlphaChooserRef alphaChooserRef = AlphaChooserRef(&mAlphaChooser);
    PlayControlsRef playControlsRef = PlayControlsRef(&mPlayControls);
    SettingsPanelRef settingsPanelRef = SettingsPanelRef(&mSettingsPanel);    
    
    // UI LAYER
	mUiLayer.setup( playlistChooserRef, 
                    alphaChooserRef, 
                    playControlsRef, 
                    settingsPanelRef,
                    mTextures[UI_BUTTONS_TEX], 
                    G_SHOW_SETTINGS, 
                    mBloomSceneRef->getInterfaceSize() );
    // add uiLayer first
    mMainBloomNodeRef->addChild( UiLayerRef(&mUiLayer) );
    
    // then children...
    mUiLayer.addChild( alphaChooserRef );
	mUiLayer.addChild( playlistChooserRef );
    mUiLayer.addChild( settingsPanelRef );    
    mUiLayer.addChild( playControlsRef );
    
	// HELP LAYER
	mHelpLayer.setup( mFontMediSmall, mFontMediBig, mFontUltraBig );
    mHelpLayer.hide( false ); // no animation
    mMainBloomNodeRef->addChild( BloomNodeRef(&mHelpLayer) );
    	
	// STATE
	mState.registerAlphaCharStateChanged( this, &KeplerApp::onAlphaCharStateChanged );
	mState.registerNodeSelected( this, &KeplerApp::onSelectedNodeChanged );
	mState.registerPlaylistStateChanged( this, &KeplerApp::onPlaylistStateChanged );
    mState.registerFilterModeStateChanged( this, &KeplerApp::onFilterModeStateChanged );
	
	// PLAYER
    mCurrentPlayState = mMusicPlayer.getPlayState();
	mMusicPlayer.registerStateChanged( this, &KeplerApp::onPlayerStateChanged );
    mMusicPlayer.registerTrackChanged( this, &KeplerApp::onPlayerTrackChanged );
    mMusicPlayer.registerLibraryChanged( this, &KeplerApp::onPlayerLibraryChanged );
	
	// PERLIN
	mPerlin = Perlin( 4 );
	
    // WORLD
    mWorld.setup();
	
    // SKY
    mSkySphere.setup(24);
    
    // GALAXY (TODO: Move to World?)
    mGalaxy.setup( G_INIT_CAM_DIST, 
                   BRIGHT_BLUE, 
                   BLUE, 
                   mTextures[GALAXY_DOME_TEX], 
                   mTextures[GALAXY_TEX], 
                   mTextures[DARK_MATTER_TEX], 
				   mStarGlowTex );

    // NOTIFICATION OVERLAY
	mNotificationOverlay.setup( mFontBig );
    mMainBloomNodeRef->addChild( BloomNodeRef(&mNotificationOverlay) );	


    //console() << "setupEnd: " << getElapsedSeconds() << std::endl;

    mUiComplete = true;
    
//    std::cout << (getElapsedSeconds() - t) << " seconds to onTextureLoaderComplete()" << std::endl;
}

void KeplerApp::initLoadingTextures()
{
    gl::Texture::Format fmt;
    fmt.enableMipmapping( true );
    fmt.setMinFilter( GL_LINEAR_MIPMAP_LINEAR );    
    fmt.setMagFilter( GL_LINEAR ); // TODO: experiment with GL_NEAREST where appropriate
    mBackgroundTex	= gl::Texture::create( loadImage( loadResource( "background.jpg" ) )/*, fmt*/ );        
    mPlanetTex		= gl::Texture::create( loadImage( loadResource( "planet.png" ) ), fmt );
	mStarGlowTex    = gl::Texture::create( loadImage( loadResource( "starGlow.png" ) ), fmt);
    mPlanetaryTex	= gl::Texture::create( loadImage( loadResource( "planetary.png" ) )/*, fmt*/ );
}

void KeplerApp::touchesBegan( TouchEvent event )
{	
    if (!mUiComplete) return;

	mIsDragging = false;
	const vector<TouchEvent::Touch> touches = getActiveTouches();

	float timeSincePinchEnded = getElapsedSeconds() - mTimePinchEnded;
	if( touches.size() == 1 && timeSincePinchEnded > 0.2f && keepTouchForPinching(*touches.begin()) ) {
        mIsTouching = true;
        mTouchPos		= touches.begin()->getPos();
        mTouchVel		= vec2(0);
		vec3 worldTouchPos;
		if( G_USE_GYRO ) worldTouchPos = vec3(mTouchPos,0);
		else			 worldTouchPos = vec3( mInverseOrientationMatrix * vec4(mTouchPos, 0.0f, 1.0f) );
        mArcball.mouseDown( vec2(worldTouchPos.x, worldTouchPos.y), getWindowSize() );
	}
    else {
        mIsTouching = false;
    }
}

void KeplerApp::touchesMoved( TouchEvent event )
{
    if (!mUiComplete) return;

    if ( mIsTouching ) {
        float timeSincePinchEnded = getElapsedSeconds() - mTimePinchEnded;	
        const vector<TouchEvent::Touch> touches = getActiveTouches();
        if( touches.size() == 1 && timeSincePinchEnded > 0.2f ){
            vec2 currentPos = touches.begin()->getPos();
            vec2 prevPos = touches.begin()->getPrevPos();        
            if (positionTouchesWorld(currentPos) && positionTouchesWorld(prevPos)) {
                mIsDragging = true;
                mTouchVel		= currentPos - prevPos;
                mTouchPos		= currentPos;
                vec3 worldTouchPos;
				if( G_USE_GYRO ) worldTouchPos = vec3(mTouchPos,0);
				else			 worldTouchPos = vec3( mInverseOrientationMatrix * vec4(mTouchPos, 0.0f, 1.0f) );
                mArcball.mouseDrag( vec2( worldTouchPos.x, worldTouchPos.y ), getWindowSize() );
            }
        }
    }
}

void KeplerApp::touchesEnded( TouchEvent event )
{
    if (!mUiComplete) return;

	float timeSincePinchEnded = getElapsedSeconds() - mTimePinchEnded;	
	const vector<TouchEvent::Touch> touches = event.getTouches();
	if( touches.size() == 1 && timeSincePinchEnded > 0.2f ){        
        vec2 currentPos = touches.begin()->getPos();
        vec2 prevPos = touches.begin()->getPrevPos();   
        // if your touch isn't hitting the uiLayer panel and the Help panel isnt showing
        if (positionTouchesWorld(currentPos) && positionTouchesWorld(prevPos)) {
            mTouchPos = currentPos;
			
            // if you havent been dragging
            if( !mIsDragging ){
                float u			= mTouchPos.x / (float) getWindowWidth();
                float v			= mTouchPos.y / (float) getWindowHeight();
                Ray touchRay	= mCam.generateRay( u, 1.0f - v, mCam.getAspectRatio() );
                checkForNodeTouch( touchRay, mTouchPos );
            }
        }
	}
	if (getActiveTouches().size() != 1) {
//        if (mIsDragging) {
//            logEvent("Camera Moved");
//        }
		mIsDragging = false;
        mIsTouching = false;
	}
}

bool KeplerApp::onPinchBegan( PinchEvent event )
{
	mIsPinching = true;
    mPinchRays = event.getTouchRays( mCam );
	mPinchPositions.clear();
	
	mTouchVel	= vec2(0);
	vector<PinchEvent::Touch> touches = event.getTouches();
	vec2 averageTouchPos = vec2(0);
	
	for( vector<PinchEvent::Touch>::iterator it = touches.begin(); it != touches.end(); ++it ){
		averageTouchPos += it->mPos;
		mPinchPositions.push_back( it->mPos );
	}
	averageTouchPos /= touches.size();
	mTouchPos = averageTouchPos;
	
// using pinch to control arcball is weird because of the pop
// from one finger to two fingers. disabled until a fix is found.
//	vec3 worldTouchPos = vec3( mInverseOrientationMatrix * vec4(mTouchPos, 0.0f, 1.0f) );
//	mArcball.mouseDrag( ivec2( worldTouchPos.x, worldTouchPos.y ) );
	
    return false;
}

bool KeplerApp::onPinchMoved( PinchEvent event )
{	
    mPinchRays = event.getTouchRays( mCam );
	mPinchPositions.clear();
	
	mPinchTotalDest *= ( 1.0f + ( 1.0f - event.getScaleDelta() ) * mPinchPerThresh * mPinchScaleMax );
	mPinchTotalDest = constrain( mPinchTotalDest, mPinchScaleMin, mPinchScaleMax );
	
	vector<PinchEvent::Touch> touches = event.getTouches();
	vec2 averageTouchPos = vec2(0);
	for( vector<PinchEvent::Touch>::iterator it = touches.begin(); it != touches.end(); ++it ){
		averageTouchPos += it->mPos;
		mPinchPositions.push_back( it->mPos );
	}
	averageTouchPos /= touches.size();
// using pinch to control arcball is weird because of the pop
// from one finger to two fingers. disabled until a fix is found.
//	vec3 worldTouchPos = vec3( mInverseOrientationMatrix * vec4(mTouchPos, 0.0f, 1.0f) );
//	mArcball.mouseDrag( ivec2( worldTouchPos.x, worldTouchPos.y ) );
	
	//mTouchThrowVel	= ( averageTouchPos - mTouchPos );
	//mTouchVel		= mTouchThrowVel;
	mTouchPos		= averageTouchPos;
	mPinchRotation	+= event.getRotationDelta();

	mTimePinchEnded = getElapsedSeconds();
	
    return false;
}

bool KeplerApp::onPinchEnded( PinchEvent event )
{
//    logEvent("Pinch Ended");

	if( mPinchPer > mPinchPerThresh ){
		Node *selected = mState.getSelectedNode();
		if( selected ){
//            console() << "backing out using pinch!" << std::endl;
			mState.setSelectedNode( selected->mParentNode );
		}
	}
	
	mTimePinchEnded = getElapsedSeconds();
	
    mPinchRays.clear();
	mIsPinching = false;
    return false;
}

bool KeplerApp::keepTouchForPinching( TouchEvent::Touch touch )
{
    return positionTouchesWorld(touch.getPos());
}

bool KeplerApp::positionTouchesWorld( vec2 screenPos )
{
    const bool inUi              = mUiLayer.hitTest(screenPos);
    const bool inAlphaChooser    = mAlphaChooser.hitTest(screenPos);
    const bool inPlaylistChooser = mPlaylistChooser.hitTest(screenPos);
//    std::cout << "inUi: " << (inUi ? "true" : "false") << std::endl;
//    std::cout << "inAlphaChooser: " << (inAlphaChooser ? "true" : "false") << std::endl;
//    std::cout << "inPlaylistChooser: " << (inPlaylistChooser ? "true" : "false") << std::endl;
    return !(inUi || inPlaylistChooser || inAlphaChooser);
}


bool KeplerApp::orientationChanged( OrientationEvent event )
{
    Orientation orientation = event.getInterfaceOrientation();
    setInterfaceOrientation(orientation);

    Orientation prevOrientation = event.getPrevInterfaceOrientation();
    
	if( ! G_USE_GYRO ){
		// Look over there!
		// heinous trickery follows...
		if (mInterfaceOrientation != prevOrientation) {
			if( glm::length(mTouchVel) > 2.0f && !mIsDragging ){        
				int steps = getRotationSteps(prevOrientation,mInterfaceOrientation);
				mTouchVel = glm::rotate( mTouchVel, (float)((float)steps * M_PI/2.0f) );
			}
		}
		// ... end heinous trickery
	}

//    if (prevOrientation != orientation) {
//        std::map<string, string> params;
//        params["Device Orientation"] = getOrientationString(event.getDeviceOrientation());
//        logEvent("Orientation Changed", params);    
//    }

    return false;
}

void KeplerApp::setInterfaceOrientation( const Orientation &orientation )
{
    mInterfaceOrientation = orientation;

    mOrientationMatrix = getOrientationMatrix44( mInterfaceOrientation, getWindowSize() );
    mInverseOrientationMatrix = glm::inverse( mOrientationMatrix );
    
//    if( ! G_USE_GYRO ) mUp = getUpVectorForOrientation( mInterfaceOrientation );
//	else			   mUp = vec3(0,1,0);
}

bool KeplerApp::onVignetteToggled( bool on )
{
	if( !on ){
		mPinchTotalDest = 1.0f;
	}
	return false;
}

bool KeplerApp::onFilterModeStateChanged( State::FilterMode filterMode )
{    
    bool filtering = mUiLayer.isShowingFilter();
    
    // apply a new filter to world...
    if (filterMode == State::FilterModeAlphaChar) {
        mWorld.setFilter( LetterFilter::create( mState.getAlphaChar() ) );
        mAlphaChooser.setAlphaChar( mState.getAlphaChar() );
        mUiLayer.setShowAlphaFilter( filtering );
        if (!filtering) {
            mUiLayer.setShowPlaylistFilter( false );
        }
    }
    else if (filterMode == State::FilterModePlaylist) {
        music::PlaylistRef playlist = mState.getPlaylist();
        mAlphaChooser.setAlphaChar( ' ' );
        if (!playlist) {
            mState.setPlaylist( mData.mPlaylists[0] ); // triggers onPlaylistStateChanged
        }
        else {
            mWorld.setFilter( PlaylistFilter::create(playlist) );
        }
        mUiLayer.setShowPlaylistFilter( filtering );
        if (!filtering) {
            mUiLayer.setShowAlphaFilter( false );
        }        
    }

    mPlayControls.setAlphaOn( mUiLayer.isShowingAlphaFilter() );
    mPlayControls.setPlaylistOn( mUiLayer.isShowingPlaylistFilter() );
    
    // now make sure that everything is cool with the current filter
    mWorld.updateAgainstCurrentFilter();
    
    return false;
}

bool KeplerApp::onPlaylistChooserTouched( music::PlaylistRef playlist )
{
    // must have already called onPlaylistChooserSelected, so it's a "simple" matter of triggering play:
    mMusicPlayer.play( playlist, 0 );
    
    // let my people know
    string playlistName = playlist->getPlaylistName();
    string br = " "; // break with spaces
    if (playlistName.size() > 20) {
        br = "\n"; // break with newline instead
        if (playlistName.size() > 40) {
            playlistName = playlistName.substr(0, 35) + "..."; // ellipsis for long long playlist names
        }
    }
    stringstream s;
    s << "STARTING PLAYLIST" << br << "“" << playlistName << "”";
    int uw = 100;
	int uh = 100;
    mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( uw*1, uh*2, uw*2, uh*3 ), s.str() );            
    
    
    // RIGHT? SIMPLE? RIGHT?!
    return false;
}

bool KeplerApp::onPlaylistChooserSelected( music::PlaylistRef playlist )
{
    // FIXME: log params?
//    logEvent("PlaylistChooser Selected");        
    mState.setFilterMode( State::FilterModePlaylist );
    mState.setPlaylist( playlist ); // triggers onPlaylistStateChanged
    mState.setSelectedNode( NULL ); // zoom to galaxy level
    return false;
}

bool KeplerApp::onAlphaCharSelected( char c )
{
    // FIXME: log params
//    logEvent("AlphaChooser Selected");
    mState.setFilterMode( State::FilterModeAlphaChar );    
    mState.setAlphaChar( c );       // triggers onAlphaCharStateChanged
    mState.setSelectedNode( NULL ); // zoom to galaxy level
	return false;
}

bool KeplerApp::onAlphaCharStateChanged( char c )
{
    // apply new filter to World:
    mWorld.setFilter( LetterFilter::create( c ) );
    
    // notify:
    stringstream s;
    s << "FILTERING ARTISTS BY '" << mState.getAlphaChar() << "'";
    mNotificationOverlay.showLetter( mState.getAlphaChar(), s.str(), mFontHuge );
    
    // log:
//    std::map<string, string> params;
//    params["Letter"] = toString( mState.getAlphaChar() );
//    params["Count"] = toString( mWorld.getNumFilteredNodes() );
//    logEvent("Letter Selected" , params);

    // sync (this is harmless and doesn't trigger any hard work)
    mAlphaChooser.setAlphaChar( c );
    
	return false;
}

bool KeplerApp::onPlaylistStateChanged( music::PlaylistRef playlist )
{
    // apply new filter to World:    
    mWorld.setFilter( PlaylistFilter::create(playlist) );

    /////// notifications...

//    string playlistName = playlist->getPlaylistName();
    	
    // log:
//    std::map<string, string> parameters;
//    parameters["Playlist"] = mState.getPlaylist()->getPlaylistName();
//    parameters["Count"] = toString( mWorld.getNumFilteredNodes() );    
//    logEvent("Playlist Selected" , parameters);	
    
	return false;
}

bool KeplerApp::onSelectedNodeChanged( Node *node )
{
	mSelectionTime	= getElapsedSeconds();
	mCenterFrom		= mCenter;
	mCamDistFrom	= mCamDist;	
	mPinchPerFrom	= mPinchPer;
	mPinchTotalFrom	= mPinchTotal;
	mZoomFrom		= G_ZOOM;
	
	
	if( node ){
		if( node->mGen == G_ALBUM_LEVEL ){
			mPinchPerInit = 0.35f;
		} else {
			mPinchPerInit = 0.2f;
		}
	}
	mPinchPer		= mPinchPerInit;
	mPinchTotalInit	= ( mPinchScaleMax - mPinchScaleMin ) * mPinchPerInit + mPinchScaleMin;
	mPinchTotalDest = mPinchTotalInit;

	if( node && node->mGen > G_ZOOM ){
		node->wasTapped();
	}

	if( node != NULL ) {
        if (node->mGen == G_TRACK_LEVEL) {
            NodeTrack* trackNode = dynamic_cast<NodeTrack*>(node);
            if (trackNode) {
                const bool isPlayingTrack = mPlayingTrack && trackNode->getId() == mPlayingTrack->getItemId();
                if ( !isPlayingTrack ){
                    const bool playlistMode = (mState.getFilterMode() == State::FilterModePlaylist);                    
                    if( playlistMode ) {
                        // find this track node in the current playlist
                        music::PlaylistRef playlist = mState.getPlaylist();
                        int index = 0;
                        for (int i = 0; i < playlist->size(); i++) {
                            if ((*playlist)[i]->getItemId() == trackNode->getId()) {
                                index = i;
                                break;
                            }
                        }
                        mMusicPlayer.play( playlist, index );                        
                    }
                    else {
                        // just play the album from the current track
                        mMusicPlayer.play( trackNode->mAlbum, trackNode->mIndex );
                    }
                }
            }
//            logEvent("Track Selected");            
        } 
//        else if (node->mGen == G_ARTIST_LEVEL) {
//            logEvent("Artist Selected");
//        } 
//        else if (node->mGen == G_ALBUM_LEVEL) {
//            logEvent("Album Selected");
//        }
	}
//    else {
//        logEvent("Selection Cleared");        
//    }

    // highlight currently playing items
    if (mPlayingTrack) {
        uint64_t trackId = mPlayingTrack->getItemId();
        uint64_t albumId = mPlayingTrack->getAlbumId();    
        uint64_t artistId = mPlayingTrack->getArtistId();
        mWorld.updateIsPlaying( artistId, albumId, trackId );
    } else {
        mWorld.updateIsPlaying( 0, 0, 0 );        
    }
    
    // now make sure that everything is cool with the current filter
    mWorld.updateAgainstCurrentFilter();
    
    if ( node == NULL && mPlayingTrack == NULL ) {
        mPlayControls.disablePlayerControls();
    }
    else {
        mPlayControls.enablePlayerControls();            
    }
    
	return false;
}

bool KeplerApp::onPlayControlsPlayheadMoved( BloomSceneEventRef event )
{
    if ( event->getNodeRef()->getId() == PlayControls::SLIDER ) {
        if ( mMusicPlayer.hasPlayingTrack() ) {
            float dragPer = mPlayControls.getPlayheadValue();
            mCurrentTrackPlayheadTime = mCurrentTrackLength * dragPer;
            mPlayheadUpdateSeconds = getElapsedSeconds();        
            mMusicPlayer.setPlayheadTime( mCurrentTrackPlayheadTime );
        }
    }
    return false;
}

bool KeplerApp::onSettingsPanelButtonPressed( BloomSceneEventRef event )
{
    SettingsPanel::ButtonId button = SettingsPanel::ButtonId( event->getNodeRef()->getId() );
    
    int uw = 100;
	int uh = 100;
	
	Area offArea = Area( uw*4, uh*2, uw*5, uh*3 );

    switch (button) {
            
		case SettingsPanel::SHUFFLE:
			if( mMusicPlayer.getShuffleMode() != music::Player::ShuffleModeOff ){
				mMusicPlayer.setShuffleMode( music::Player::ShuffleModeOff );
				mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( uw*2, uh*0, uw*3, uh*1 ), offArea, "SHUFFLE OFF" );
			} else {
				mMusicPlayer.setShuffleMode( music::Player::ShuffleModeSongs );
				mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( uw*2, uh*0, uw*3, uh*1 ), "SHUFFLE ON" );
			}
            mSettingsPanel.setShuffleOn( mMusicPlayer.getShuffleMode() != music::Player::ShuffleModeOff );
//            logEvent("Shuffle Button Selected");    
            break;
			
		case SettingsPanel::REPEAT:
            switch ( mMusicPlayer.getRepeatMode() ) {
                case music::Player::RepeatModeNone:
                    mMusicPlayer.setRepeatMode( music::Player::RepeatModeAll );
                    mSettingsPanel.setRepeatMode( music::Player::RepeatModeAll );    
                    mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( uw*3, uh*0, uw*4, uh*1 ), "REPEAT ALL" );
                    break;
                case music::Player::RepeatModeAll:
                    mMusicPlayer.setRepeatMode( music::Player::RepeatModeOne );
                    mSettingsPanel.setRepeatMode( music::Player::RepeatModeOne );    
                    mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( uw*4, uh*0, uw*5, uh*1 ), "REPEAT ONE" );
                    break;
                case music::Player::RepeatModeOne:
                case music::Player::RepeatModeDefault:
                    // repeat mode is RepeatModeDefault when we start up and until 
                    // our user chooses it, we can't know what the current state is                    
                    mMusicPlayer.setRepeatMode( music::Player::RepeatModeNone );
                    mSettingsPanel.setRepeatMode( music::Player::RepeatModeNone );    
                    mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( uw*3, uh*0, uw*4, uh*1 ), offArea, "REPEAT NONE" );
                    break;
            }
//            logEvent("Repeat Button Selected");   
            break;
            
        case SettingsPanel::HELP:
            logEvent("Help Button Selected");            
            mHelpLayer.toggle();
            mSettingsPanel.setHelpOn( mHelpLayer.isShowing() );
            break;

		
		case SettingsPanel::DRAW_TEXT:
			if( G_SHOW_SETTINGS ){
//				logEvent("Draw Text Button Selected");            
				G_DRAW_TEXT = !G_DRAW_TEXT;
				if( G_DRAW_TEXT )	mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( uw*2, uh*1, uw*3, uh*2 ), "TEXT LABELS" );
				else				mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( uw*2, uh*1, uw*3, uh*2 ), offArea, "TEXT LABELS" );
			}
            mSettingsPanel.setLabelsOn( G_DRAW_TEXT );
            break;
			
			
        case SettingsPanel::DRAW_RINGS:
			if( G_SHOW_SETTINGS ){
//				logEvent("Draw Rings Button Selected");            
				G_DRAW_RINGS = !G_DRAW_RINGS;
				if( G_DRAW_RINGS )	mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( uw*3, uh*1, uw*4, uh*2 ), "ORBIT LINES" );
				else				mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( uw*3, uh*1, uw*4, uh*2 ), offArea, "ORBIT LINES" );
			}
            mSettingsPanel.setOrbitsOn( G_DRAW_RINGS );            
            break;
            

            
		case SettingsPanel::USE_GYRO:
			if( G_SHOW_SETTINGS ){
//				logEvent("Use Gyro Button Selected");            
				G_USE_GYRO = !G_USE_GYRO;
				
				if( !G_USE_GYRO ) {
                    mUp = getUpVectorForOrientation( mInterfaceOrientation );
                } else {
                    mUp = vec3(0,1,0);
                }				
				
				if( G_USE_GYRO )	mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( uw*4, uh*1, uw*5, uh*2 ), "GYROSCOPE" );
                else				mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( uw*4, uh*1, uw*5, uh*2 ), offArea, "GYROSCOPE" );
                
			}
            mSettingsPanel.setGyroOn( G_USE_GYRO );
            break;
            
		case SettingsPanel::DEBUG_FEATURE:
			G_DEBUG = !G_DEBUG;
			if( G_DEBUG )	mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( uw*2, uh*2, uw*3, uh*3 ), "DEBUG MODE" );
			else			mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( uw*2, uh*2, uw*3, uh*3 ), offArea, "DEBUG MODE" );
            mSettingsPanel.setDebugOn( G_DEBUG );
            break;
			
		case SettingsPanel::AUTO_MOVE:
			if( G_SHOW_SETTINGS ){
				logEvent("Automove Button Selected");            
				G_AUTO_MOVE = !G_AUTO_MOVE;
				if( G_AUTO_MOVE ){
					if( isLandscapeOrientation( mInterfaceOrientation ) ){
						mTouchPos = getWindowCenter();
						mTouchVel = vec2( Rand::randFloat( -0.2f, 0.2f ), Rand::randPosNegFloat( 0.5f, 1.0f ) );
					} else {
						mTouchPos = getWindowCenter();
						mTouchVel = vec2( Rand::randPosNegFloat( 0.5f, 1.0f ), Rand::randFloat( -0.2f, 0.2f ) );
					}
					
					mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( uw*3, uh*2, uw*4, uh*3 ), "ANIMATE CAMERA" );
				} else {
					mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( uw*3, uh*2, uw*4, uh*3 ), offArea, "ANIMATE CAMERA" );
				}
			}
            mSettingsPanel.setScreensaverOn( G_AUTO_MOVE );            
            break;
			
			
		case SettingsPanel::PARAMSLIDER1:
        case SettingsPanel::PARAMSLIDER2:
            // TODO: log?
            break;
			
        case SettingsPanel::NO_BUTTON:
            //console() << "unknown button pressed!" << std::endl;
            break;
            
    } // switch
    
    return false;
}

bool KeplerApp::onPlayControlsButtonPressed( BloomSceneEventRef event )
{
    PlayControls::ButtonId button = PlayControls::ButtonId( event->getNodeRef()->getId() );
    
	int uw = 100;
	int uh = 100;
	
	Area offArea = Area( uw*4, uh*2, uw*5, uh*3 );
	
    switch( button ) {
        
        case PlayControls::PREV_TRACK:
//            logEvent("Previous Track Button Selected");            
            mMusicPlayer.skipPrev();
            break;
        
        case PlayControls::PLAY_PAUSE:
            {
//                logEvent("Play/Pause Button Selected");            
                if (mMusicPlayer.hasPlayingTrack()) {
                    togglePlayPaused();
                }
                else {
                    // in the rare case that there's nothing queued, play whatever we're looking at
                    Node* selectedNode = mState.getSelectedNode();
                    if (selectedNode != NULL) {
                        if (selectedNode->mGen == G_TRACK_LEVEL) {
                            NodeTrack *nodeTrack = static_cast<NodeTrack*>(selectedNode);
                            mMusicPlayer.play( nodeTrack->mAlbum, nodeTrack->mIndex );
                        }
                        else if (selectedNode->mGen == G_ALBUM_LEVEL) {
                            NodeAlbum *nodeAlbum = static_cast<NodeAlbum*>(selectedNode);
                            mMusicPlayer.play( nodeAlbum->getPlaylist(), 0 );
                        }
                        else if (selectedNode->mGen == G_ARTIST_LEVEL) {
                            NodeArtist *nodeArtist = static_cast<NodeArtist*>(selectedNode);
                            mMusicPlayer.play( nodeArtist->getPlaylist(), 0 );
                        }
                        mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( 0.0f, 0.0f, uw, uh ), "PLAY" );                
                    }
                }
            }
            break;
        
        case PlayControls::NEXT_TRACK:
//            logEvent("Next Track Button Selected");            
            mMusicPlayer.skipNext();	
            break;
						
		case PlayControls::GOTO_GALAXY:
            logEvent("Galaxy Button Selected");
			mState.setSelectedNode( NULL );
			mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( uw*0, uh*1, uw*1, uh*2 ), "ZOOM TO GALAXY" ); // FIXME: Better copy?
            break;
			
        case PlayControls::GOTO_CURRENT_TRACK:
            logEvent("Current Track Button Selected");            
            flyToCurrentTrack();
			mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( uw*1, uh*1, uw*2, uh*2 ), "ZOOM TO CURRENT TRACK" ); // FIXME: Better copy?
            break;
			
		case PlayControls::SETTINGS:
            logEvent("Settings Button Selected");            
            G_SHOW_SETTINGS = !G_SHOW_SETTINGS;
            mPlayControls.setShowSettingsOn( G_SHOW_SETTINGS );
            mSettingsPanel.setVisible( G_SHOW_SETTINGS ); // FIXME: animate
            mUiLayer.setShowSettings( G_SHOW_SETTINGS );
            break;

        case PlayControls::SLIDER:
            break;
        
		case PlayControls::SHOW_ALPHA_FILTER:
            {
                mUiLayer.setShowAlphaFilter( !mUiLayer.isShowingAlphaFilter() );            
                mVignette.setShowing( mUiLayer.isShowingFilter() );
                mPlayControls.setAlphaOn( mUiLayer.isShowingAlphaFilter() );
                mPlayControls.setPlaylistOn( mUiLayer.isShowingPlaylistFilter() );
				if( mUiLayer.isShowingAlphaFilter() ) {
					mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( uw*0, uh*2, uw*1, uh*3 ), "FILTER BY ALPHABET" ); // FIXME: Better copy?
                }
				else {
					mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( uw*0, uh*2, uw*1, uh*3 ), offArea, "FILTER BY ALPHABET" ); // FIXME: Better copy?
                }
            }
            break;	
            
        case PlayControls::SHOW_PLAYLIST_FILTER:
            {
                mUiLayer.setShowPlaylistFilter( !mUiLayer.isShowingPlaylistFilter() );            
                mVignette.setShowing( mUiLayer.isShowingFilter() );
                mPlayControls.setAlphaOn( mUiLayer.isShowingAlphaFilter() );
                mPlayControls.setPlaylistOn( mUiLayer.isShowingPlaylistFilter() );
				
				if( mUiLayer.isShowingPlaylistFilter() ) {
					mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( uw*1, uh*2, uw*2, uh*3 ), "FILTER BY PLAYLIST" ); // FIXME: Better copy?
                }
				else {
					mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( uw*1, uh*2, uw*2, uh*3 ), offArea, "FILTER BY PLAYLIST" ); // FIXME: Better copy?
                }
            }
            break;
			
        case PlayControls::NO_BUTTON:
            //console() << "unknown button pressed!" << std::endl;
            break;

	} // switch

	return false;
}

// heavy function, should be avoided but should do the right thing when needed
void KeplerApp::flyToCurrentTrack()
{
	if (mMusicPlayer.hasPlayingTrack()) {
        
        music::TrackRef newTrack = mMusicPlayer.getPlayingTrack();
        
        uint64_t trackId = newTrack->getItemId();
        uint64_t artistId = newTrack->getArtistId();
        uint64_t albumId = newTrack->getAlbumId();            
        
        // see if we're in the current playlist
        bool inCurrentPlaylist = false;
        if( mState.getFilterMode() == State::FilterModePlaylist ) {
            // find this track node in the current playlist
            music::PlaylistRef playlist = mState.getPlaylist();
            for (int i = 0; i < playlist->size(); i++) {
                if ((*playlist)[i]->getItemId() == trackId) {
                    inCurrentPlaylist = true;
                    break;
                }
            }
        }
        
        // if we're not, set it back to alpha mode
        if (!inCurrentPlaylist) {
            // trigger hefty stuff in onFilterModeStateChanged if needed
            if ( mState.getFilterMode() != State::FilterModeAlphaChar ) {
                mState.setFilterMode( State::FilterModeAlphaChar );            
            }
            // trigger hefty stuff in onAlphaCharStateChanged if needed
            mState.setAlphaChar( newTrack->getArtist() ); 
        }
        
        // select nodes, set mIsPlaying, return selected track node:
        // (see also: onSelectedNodeChanged, triggered by this call):
        mState.setSelectedNode( mWorld.selectPlayingHierarchy( artistId, albumId, trackId ) );
    }
}

// heavy function, should be avoided but should do the right thing when needed
void KeplerApp::flyToCurrentAlbum()
{
	if (mMusicPlayer.hasPlayingTrack()) {
        
        music::TrackRef newTrack = mMusicPlayer.getPlayingTrack();
        
        uint64_t trackId = newTrack->getItemId();
        uint64_t albumId = newTrack->getAlbumId();
        uint64_t artistId = newTrack->getArtistId();
        
        // see if we're in the current playlist
        bool inCurrentPlaylist = false;
        if( mState.getFilterMode() == State::FilterModePlaylist ) {
            // find this track node in the current playlist
            music::PlaylistRef playlist = mState.getPlaylist();
            for (int i = 0; i < playlist->size(); i++) {
                if ((*playlist)[i]->getItemId() == trackId) {
                    inCurrentPlaylist = true;
                    break;
                }
            }
        }
        
        // if we're not, set it back to alpha mode
        if (!inCurrentPlaylist) {
            // trigger hefty stuff in onFilterModeStateChanged if needed
            if ( mState.getFilterMode() != State::FilterModeAlphaChar ) {
                mState.setFilterMode( State::FilterModeAlphaChar );            
            }
            // trigger hefty stuff in onAlphaCharStateChanged if needed
            mState.setAlphaChar( newTrack->getArtist() ); 
        }
        
        // select nodes, set mIsPlaying, return selected track node:
        // (see also: onSelectedNodeChanged, triggered by this call):
        mWorld.selectHierarchy( artistId, albumId, 0 );
        
        mState.setSelectedNode( mWorld.getAlbumNodeById( artistId, albumId ) );
        
        mWorld.updateIsPlaying( artistId, albumId, trackId );
    }
}

// heavy function, should be avoided but should do the right thing when needed
void KeplerApp::flyToCurrentArtist()
{
	if (mMusicPlayer.hasPlayingTrack()) {
        
        music::TrackRef newTrack = mMusicPlayer.getPlayingTrack();
        
        uint64_t trackId = newTrack->getItemId();
        uint64_t albumId = newTrack->getAlbumId();
        uint64_t artistId = newTrack->getArtistId();
        
        // see if we're in the current playlist
        bool inCurrentPlaylist = false;
        if( mState.getFilterMode() == State::FilterModePlaylist ) {
            // find this track node in the current playlist
            music::PlaylistRef playlist = mState.getPlaylist();
            for (int i = 0; i < playlist->size(); i++) {
                if ((*playlist)[i]->getItemId() == trackId) {
                    inCurrentPlaylist = true;
                    break;
                }
            }
        }
        
        // if we're not, set it back to alpha mode
        if (!inCurrentPlaylist) {
            // trigger hefty stuff in onFilterModeStateChanged if needed
            if ( mState.getFilterMode() != State::FilterModeAlphaChar ) {
                mState.setFilterMode( State::FilterModeAlphaChar );            
            }
            // trigger hefty stuff in onAlphaCharStateChanged if needed
            mState.setAlphaChar( newTrack->getArtist() ); 
        }
        
        // select nodes, set mIsPlaying, return selected track node:
        // (see also: onSelectedNodeChanged, triggered by this call):
        mWorld.selectHierarchy( artistId, 0, 0 );
        
        mState.setSelectedNode( mWorld.getArtistNodeById( artistId ) );
        
        mWorld.updateIsPlaying( artistId, albumId, trackId );
    }
}

void KeplerApp::togglePlayPaused()
{
    const bool isPlaying = (mCurrentPlayState == music::Player::StatePlaying);
    if ( isPlaying ) {
        mMusicPlayer.pause();
        mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( 100.0f, 0.0f, 200.0f, 100.0f ), "PAUSED" );
    }
    else {        
        mMusicPlayer.play();
        mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( 0.0f, 0.0f, 100.0f, 100.0f ), "PLAY" );
    }    
}
 
void KeplerApp::checkForNodeTouch( const Ray &ray, const vec2 &pos )
{
	vector<Node*> nodes;
	mWorld.checkForNameTouch( nodes, pos );

	if ( nodes.size() > 0 ) {
        
		Node *nodeWithHighestGen = *nodes.begin();
		int highestGen = nodeWithHighestGen->mGen;
		
		vector<Node*>::iterator it;
		for( it = nodes.begin(); it != nodes.end(); ++it ){
			if( (*it)->mGen > highestGen ){
				highestGen = (*it)->mGen;
				nodeWithHighestGen = *it;
			}
		}
        
        //////// perform the selection if needed
        // (toggle play state if not)
        
        const bool notSelectedNode = (mState.getSelectedNode() != nodeWithHighestGen);        
        if ( notSelectedNode ) {
            // if the tapped node isn't the current selection, it should be:
            mState.setSelectedNode( nodeWithHighestGen );
            // (other selection logic happens in onSelectedNodeChanged)            
        }
        else {
            if ( highestGen == G_TRACK_LEVEL ) {
                const bool isPlayingNode = (mPlayingTrack && mPlayingTrack->getItemId() == nodeWithHighestGen->getId());                
                if ( isPlayingNode ) {
                    // if this is already the selected node, just toggle the play state
                    togglePlayPaused();
//                    logEvent( "Playing Track Node Touched" );
                }
            }     
            else if ( highestGen == G_ALBUM_LEVEL ) {
                NodeAlbum* nodeAlbum = dynamic_cast<NodeAlbum*>(nodeWithHighestGen);
                if (mPlayingTrack && mPlayingTrack->getAlbumId() == nodeAlbum->getId()) {
                    togglePlayPaused();
                }
                else {
                    mMusicPlayer.play( nodeAlbum->getPlaylist() );
                    // FIXME: use album name in overlay:
                    mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( 0.0f, 0.0f, 100.0f, 100.0f ), "Playing Album" );                    
                }
            }
            if ( highestGen == G_ARTIST_LEVEL ) {
                NodeArtist* nodeArtist = dynamic_cast<NodeArtist*>(nodeWithHighestGen);
                if (mPlayingTrack && mPlayingTrack->getArtistId() == nodeArtist->getId()) {
                    togglePlayPaused();
                }
                else {
//                    std::cout << "Artist Tapped - different to current track. Setting playlist..." << std::endl;
                    mMusicPlayer.play( music::getAlbumPlaylistWithArtistId(nodeArtist->getId()) );
//                    std::cout << "... done setting playlist." << std::endl;
                    // FIXME: use artist name in overlay:
                    mNotificationOverlay.show( mTextures[UI_BUTTONS_TEX], Area( 0.0f, 0.0f, 100.0f, 100.0f ), "Playing Artist" );
                }
            }            
        }
        
        ////// logging for interaction tracking...
        
//        switch ( highestGen ) {
//            case G_ARTIST_LEVEL: logEvent( "Artist Node Touched" ); break;
//            case G_ALBUM_LEVEL:  logEvent( "Album Node Touched" );  break;
//            case G_TRACK_LEVEL:  logEvent( "Track Node Touched" );  break;
//        } 
	}
}

void KeplerApp::update()
{
#if defined( PLANETARY_DEBUG_AUTOSELECT )
	// Debug: select the first artist once the world is populated, flying the
	// camera in so the lit album and track spheres are on screen.
	//
	// This fires on a timer rather than on a key or a tap. Cinder only delivers
	// keyDown on iOS while a UIKeyInput responder is first responder, which this
	// app never makes one, so a keypress reaches nothing; and every screen edge
	// where a debug tap zone would sit is already claimed by the top bar or the
	// alpha wheel, which consume touches before touchesBegan runs.
	//
	// The artist's own initial is applied as the filter first, since the default
	// filter usually matches nothing and a node outside the active filter cannot
	// be selected. Compiled out unless -DPLANETARY_DEBUG_AUTOSELECT=ON.
	if( ! mDebugDidAutoSelect && mUiComplete && mData.getState() == Data::LoadStateComplete ) {
		if( NodeArtist *first = mWorld.getFirstNode() ) {
			mDebugDidAutoSelect = true;
			console() << "Planetary debug: auto-selecting first artist '" << first->getName() << "'" << endl;
			mState.setAlphaChar( first->getName() );
			mState.setSelectedNode( mWorld.getFirstFilteredNode() );
		}
	}
	// Second stage: drop into the first album once it exists, so a lit sphere
	// actually fills the frame -- at artist distance the albums are only a few
	// pixels across. Deliberately a later frame than the artist selection:
	// NodeArtist::select is what creates the album children, so they cannot be
	// read in the same frame that triggers it.
	else if( mDebugDidAutoSelect && ! mDebugDidSelectAlbum ) {
		Node *artist = mState.getSelectedArtistNode();
		if( artist && ! artist->mChildNodes.empty() ) {
			mDebugDidSelectAlbum = true;
			Node *album = artist->mChildNodes.front();
			console() << "Planetary debug: auto-selecting first album '" << album->getName()
			          << "' (" << artist->mChildNodes.size() << " albums)" << endl;
			mState.setSelectedNode( album );
		}
	}
	// Third stage: select the first track. Selecting a track node is what
	// starts playback (onSelectedNodeChanged), so this exercises the transport
	// without needing to hit a moon's touch target.
	else if( mDebugDidSelectAlbum && ! mDebugDidSelectTrack ) {
		Node *album = mState.getSelectedAlbumNode();
		if( album && ! album->mChildNodes.empty() ) {
			mDebugDidSelectTrack = true;
			Node *track = album->mChildNodes.front();
			console() << "Planetary debug: auto-selecting first track '" << track->getName()
			          << "' (" << album->mChildNodes.size() << " tracks)" << endl;
			mState.setSelectedNode( track );
		}
	}
#endif

    // Playback runs on a background queue, so a failed play() cannot report back
    // through its return value. Poll for anything the player wants to tell the
    // user -- most often that there is no active Spotify device to play on.
    {
        std::string playerMessage;
        // Deduplicated: the 1Hz state poll re-sets the same message every
        // second while the condition holds, which would strobe the overlay.
        // Only a change is worth showing.
        if( mMusicPlayer.takeStatusMessage( playerMessage ) && playerMessage != mLastPlayerMessage ) {
            mLastPlayerMessage = playerMessage;
            if( ! playerMessage.empty() ) {
                console() << "Planetary: " << playerMessage << endl;
                mNotificationOverlay.showLetter( ' ', playerMessage, mFontHuge );
            }
        }
    }

    if ( mUiComplete && (mData.getState() == Data::LoadStatePending) && mLoadingScreen.isComplete() ) {
        mData.update();

        // processes pending nodes
		mWorld.initNodes( mData.mArtists, mFontMedi, mFontMediTiny, mHighResSurfaces, mLowResSurfaces, mNoAlbumArtSurface );
        
        mAlphaChooser.setNumberAlphaPerChar( mData.mNormalizedArtistsPerChar );
		mLoadingScreen.setVisible( false ); // TODO: remove from scene graph, clean up textures
        mMainBloomNodeRef->setVisible( true );
		mUiLayer.setIsPanelOpen( true );

        // and then make sure we know about the current track if there is one...
        if ( mMusicPlayer.hasPlayingTrack() ) {
            logEvent("Startup with Track Playing");
            // update player info and then fly to current track
            onPlayerTrackChanged( &mMusicPlayer );
            flyToCurrentTrack();                
        } else {
            logEvent("Startup without Track Playing");
            // show vignette (to invite filtering) and apply first filter
            mUiLayer.setShowAlphaFilter( true );
            mPlayControls.setAlphaOn( mUiLayer.isShowingAlphaFilter() );            
            mVignette.setShowing( true );
            mWorld.setFilter( LetterFilter::create( mState.getAlphaChar() ) );
            mState.setSelectedNode( NULL ); // trigger zoom to galaxy            
		}
        
        if (mData.mPlaylists.size() > 0) {
            mPlayControls.setPlaylistButtonVisible( true );
            mPlaylistChooser.setDataWorldCam( &mData, &mWorld, &mCam );
        }
        else {
            mPlayControls.setPlaylistButtonVisible( false );            
        }
	}
    
    if ( mLoadingScreen.isVisible() ) {
        mLoadingScreen.setTextureProgress( mTextures.getProgress() );
        mLoadingScreen.setArtistProgress( mData.getArtistProgress() );
        mLoadingScreen.setPlaylistProgress( mData.getPlaylistProgress() );
    }
    
    // update UiLayer, PlayControls etc.
    mBloomSceneRef->deepUpdate();    
    
    if ( mUiComplete && (mData.getState() == Data::LoadStateComplete) )
	{
		if( G_IS_IPAD2 && G_USE_GYRO ) {
			mGyroHelper.update();
        }

//        const int elapsedFrames = getElapsedFrames();
        const float elapsedSeconds = getElapsedSeconds();

		updateArcball();
		
        // fake playhead time if we're dragging (so it looks really snappy)
        if (mPlayControls.isPlayheadDragging()) {
            mCurrentTrackPlayheadTime = mCurrentTrackLength * mPlayControls.getPlayheadValue();
            mPlayheadUpdateSeconds = elapsedSeconds;
        }
        else if (elapsedSeconds - mPlayheadUpdateSeconds > 1) {
            // mCurrentTrackPlayheadTime is set to 0 if the track changes
            mCurrentTrackPlayheadTime = mMusicPlayer.getPlayheadTime();
            mPlayheadUpdateSeconds = elapsedSeconds;
        }

		if( mPlayingTrack && mWorld.mPlayingTrackNode && G_ZOOM > G_ARTIST_LEVEL ){
            const bool isPaused = (mCurrentPlayState == music::Player::StatePaused);
            const bool isStopped = (mCurrentPlayState == music::Player::StateStopped);
            const bool isDragging = mPlayControls.isPlayheadDragging();
            const bool skipCorrection = (isPaused || isStopped || isDragging);
            float correction = skipCorrection ? 0.0f : (elapsedSeconds - mPlayheadUpdateSeconds);
			mWorld.mPlayingTrackNode->updateAudioData( mCurrentTrackPlayheadTime + correction );
		}
		
		const float scaleSlider = mSettingsPanel.getParamSlider1Value();
		const float speedSlider = mSettingsPanel.getParamSlider2Value();
        mWorld.update( 0.25f + scaleSlider * 2.0f, pow( speedSlider, 3.0f ) * 0.1f );
		
        updateCamera();
        
        vec3 bbRight, bbUp;
        mCam.getBillboardVectors( &bbRight, &bbUp );        
        
		// use raw getWindowSize (without swizzle) because 3D stuff is always drawn portrait
        // the labels and interactions are rotated for landscape left/right and upside-down modes
		vec2 interfaceSize = getWindowSize();
        mWorld.updateGraphics( mCam, interfaceSize * 0.5f, bbRight, bbUp, mFadeInAlphaToArtist );
		
		Node *selectedArtistNode = mState.getSelectedArtistNode();
		if( selectedArtistNode ){
			mParticleController.update( mCenter, selectedArtistNode->mRadius * 0.15f, bbRight, bbUp );
			float per = selectedArtistNode->mEclipseStrength * 0.5f + 0.25f;
			mParticleController.buildParticleVertexArray( 5.0f, selectedArtistNode->mColor, ( sin( per * M_PI ) * sin( per * 0.25f ) * 0.75f ) + 0.25f );
			mParticleController.buildDustVertexArray( scaleSlider, selectedArtistNode, mPinchAlphaPer, 0.1f );//( 1.0f - mCamRingAlpha ) * 0.15f * mFadeInArtistToAlbum );
		}
		
		float eclipseAmt = 0.0f;
		if( selectedArtistNode )
			eclipseAmt = selectedArtistNode->mEclipseStrength * 0.2f;
		mGalaxy.update( mEye - mCenterOffset, mFadeInAlphaToArtist, ( 1.0f - mFadeInAlphaToArtist ) * 0.1f, eclipseAmt, bbRight, bbUp );
		   
		        
        if (mPlayheadUpdateSeconds == elapsedSeconds) {
            mPlayControls.setElapsedSeconds( (int)mCurrentTrackPlayheadTime );
            mPlayControls.setRemainingSeconds( -(int)(mCurrentTrackLength - mCurrentTrackPlayheadTime) );
        }
        
        if (!mPlayControls.isPlayheadDragging()) {
            mPlayControls.setPlayheadValue( constrain( mCurrentTrackPlayheadTime / mCurrentTrackLength, 0.0, 1.0 ) );
        }
                
//        if( /*G_DEBUG &&*/ elapsedFrames % 30 == 0 ){
//            mStats.update(	getAverageFps(), 
//							mCurrentTrackPlayheadTime, 
//							mFov, 
//							mCamDist, 
//							mPinchTotalDest, 
//							G_CURRENT_LEVEL, 
//							G_ZOOM);
//        }
        
    }
    
    if (!mRemainingSetupCalled) {
        // make sure we've drawn the loading screen and then call this
        if (getElapsedFrames() > 1) {
            remainingSetup();
        }        
    }    
    
    // do UI thread things...
    UiTaskQueue::update();
}

void KeplerApp::updateArcball()
{	
	// Keep the arcball's world sphere where the camera is actually looking,
	// sized to fill most of the view — which is what 0.8's 500px screen ball
	// meant. Without this the per-frame synthetic drag below runs against a
	// stale sphere whenever the camera moves.
	float arcballRadius = std::max( mCamDist, 1.0f ) * 0.5f;
	mArcball.setSphere( Sphere( mCenter, arcballRadius ) );

	if( !G_AUTO_MOVE ){
		if( glm::length(mTouchVel) > 2.0f && !mIsDragging ){
			vec3 downPos;
			if( G_USE_GYRO )	downPos = ( vec3(mTouchPos,0) );
			else				downPos = vec3( mInverseOrientationMatrix * vec4(mTouchPos, 0.0f, 1.0f) );
			mArcball.mouseDown( vec2(downPos.x, downPos.y), getWindowSize() );
			
			vec3 dragPos;
			if( G_USE_GYRO )	dragPos = ( vec3(mTouchPos + mTouchVel,0) );
			else				dragPos = vec3( mInverseOrientationMatrix * vec4(mTouchPos + mTouchVel, 0.0f, 1.0f) );
			mArcball.mouseDrag( vec2(dragPos.x, dragPos.y), getWindowSize() );        
		}
	} else {
		if( !mIsDragging ){
			vec3 downPos;
			if( G_USE_GYRO )	downPos = ( vec3(mTouchPos,0) );
			else				downPos = vec3( mInverseOrientationMatrix * vec4(mTouchPos, 0.0f, 1.0f) );
			mArcball.mouseDown( vec2(downPos.x, downPos.y), getWindowSize() );
			
			vec3 dragPos;
			if( G_USE_GYRO )	dragPos = ( vec3(mTouchPos + mTouchVel,0) );
			else				dragPos = vec3( mInverseOrientationMatrix * vec4(mTouchPos + mTouchVel, 0.0f, 1.0f) );
			mArcball.mouseDrag( vec2(dragPos.x, dragPos.y), getWindowSize() );        
		}
	}
}


void KeplerApp::updateCamera()
{	

	mPinchTotal -= ( mPinchTotal - mPinchTotalDest ) * 0.4f;
    //mPinchTotal = mPinchTotalDest; // Tom thinks things should be less swooshy
	mPinchPer = ( mPinchTotal - mPinchScaleMin )/( mPinchScaleMax - mPinchScaleMin );
	mPinchHighlightRadius -= ( mPinchHighlightRadius - 200.0f ) * 0.4f;
	

	
// IF THE PINCH IS PAST THE POP THRESHOLD...
	if( mPinchPer > mPinchPerThresh ){
        
		if( ! mIsPastPinchThresh ) {
            mPinchHighlightRadius = 650.0f;
        }
		if( G_CURRENT_LEVEL < G_ARTIST_LEVEL ) {
			mPinchAlphaPer -= ( mPinchAlphaPer - 1.0f ) * 0.1f;
        }
		else {
			mPinchAlphaPer -= ( mPinchAlphaPer ) * 0.1f;
        }
		
		mIsPastPinchThresh = true;
		
		if( G_CURRENT_LEVEL == G_TRACK_LEVEL )			mFovDest = 70.0f; // FIXME: G_TRACK_FOV?
		else if( G_CURRENT_LEVEL == G_ALBUM_LEVEL )		mFovDest = 70.0f; // FIXME: G_ALBUM_FOV?
		else if( G_CURRENT_LEVEL == G_ARTIST_LEVEL )	mFovDest = 70.0f; // FIXME: G_ARTIST_FOV?
		else											mFovDest = G_MAX_FOV;
			
// OTHERWISE...
	} else {
        
//        std::cout << "modulating camera without pinch " << mPinchAlphaPer << std::endl;        
        
		if( mIsPastPinchThresh ) mPinchHighlightRadius = 125.0f;
		mPinchAlphaPer -= ( mPinchAlphaPer - 1.0f ) * 0.1f;
		mIsPastPinchThresh = false;
		
		if( mVignette.isShowing() ){
            mFovDest = G_MAX_FOV; // special FOV just for alpha wheel
        } else {
            mFovDest = G_DEFAULT_FOV;
        }
	}

    mFov -= ( mFov - mFovDest ) * 0.2f;    
	
	// multiplier is how far back, and addition is how close in.
	float cameraDistMulti = mPinchPer * 2.0f + 0.5f;
	if( G_CURRENT_LEVEL == G_TRACK_LEVEL ){
		cameraDistMulti = mPinchPer * 2.0f + 0.5f;
		
	} else if( G_CURRENT_LEVEL == G_ALBUM_LEVEL ){
		cameraDistMulti = mPinchPer * 2.5f + 0.25f;
		
	} else if( G_CURRENT_LEVEL == G_ARTIST_LEVEL ){
		cameraDistMulti = mPinchPer * 2.0f + 0.25f;
		
	} else {
		cameraDistMulti = mPinchPer * 2.0f + 0.675f;
		
	}
	
	
	
	Node* selectedNode = mState.getSelectedNode();
	if( selectedNode ){															// IF THERE IS A CURRENTLY SELECTED NODE...
		mCamDistDest	= selectedNode->mIdealCameraDist * cameraDistMulti;
		
		if( G_AUTO_MOVE ){															// IF THE CAMERA IS SET TO MOVE AUTOMATICALLY...
			if( selectedNode->mParentNode ){											// IF I HAVE A PARENT NODE
																						// THEN LOOK BETWEEN ME AND MY PARENT.
				float timeForAnim	= getElapsedSeconds();;
				mPerlinForAutoMove	= mPerlin.fBm( timeForAnim * 0.01f );
				
				vec3 dirToParent	= selectedNode->mParentNode->mPos - selectedNode->mPos;
				float amt			= 0.3f + sin( timeForAnim * 0.1f ) * 0.45f;
				vec3 lookToPos		= dirToParent * amt;
				mCenterOffset -= ( mCenterOffset - lookToPos ) * 0.01f;
				
				mAutoMoveScale -= ( mAutoMoveScale - ( ( mPerlinForAutoMove + 1.0f ) * G_ZOOM ) ) * 0.01f;
				
			}
		} else {																	// ELSE IF THE CAMERA IS NOT MOVING AUTOMATICALLY...
			mPerlinForAutoMove -= ( mPerlinForAutoMove - 0.0f ) * 0.15f;
			mAutoMoveScale -= ( mAutoMoveScale - 1.0f ) * 0.1f;
			
			if( selectedNode->mParentNode && mPinchPer > mPinchPerThresh ){				// IF THERE IS A PARENT AND THE PINCH THRESH HAS BEEN PASSED...
																						// LOOK BACK TOWARDS THE PARENT NODE
				vec3 dirToParent = selectedNode->mParentNode->mPos - selectedNode->mPos;
				mCenterOffset -= ( mCenterOffset - ( dirToParent * ( mPinchPer - mPinchPerThresh ) * 2.5f ) ) * 0.2f;
				
			} else {																	// ELSE DONT LOOK ANYWHERE SPECIAL... 
																						// KEEP ON DOING WHAT YOU ARE DOING...
				mCenterOffset -= ( mCenterOffset - vec3(0) ) * 0.2f;
			}
		}
		
		mCamDistDest  *= mAutoMoveScale;
		mCenterDest		= selectedNode->mPos;
		mZoomDest		= selectedNode->mGen;
		mCenterFrom		+= selectedNode->mVel;
		
	} else {																	// ELSE JUST SET CAMERA VARS TO DEFAULTS AND ZEROS
		mCamDistDest	= G_INIT_CAM_DIST * cameraDistMulti;
		mCenterDest		= vec3(0);
        mZoomDest       = G_ALPHA_LEVEL;
		mCenterOffset -= ( mCenterOffset - vec3(0) ) * 0.05f;
	}
	
	G_CURRENT_LEVEL = mZoomDest;
	
	if( mIsPinching && G_CURRENT_LEVEL <= G_ALPHA_LEVEL ){
		if( mPinchPer > mPinchPerThresh ){
            if ( !mVignette.isShowing() ) {
                mVignette.setShowing( true );
            }
		} else if( mPinchPer <= mPinchPerThresh ){
            if ( mVignette.isShowing() ) {
                mVignette.setShowing( false );
            }
		}
	}
    else if ( !mUiLayer.isShowingFilter() ) {
        if ( mVignette.isShowing() ) {
            mVignette.setShowing( false );
        }
    }

	float distToTravel = mState.getDistBetweenNodes();
	double duration = 2.5f;
	if( distToTravel < 1.0f )		duration = 2.0;
	else if( distToTravel < 50.0f )	duration = 2.75f;
	else							duration = 4.0f;

	
    double t		= constrain( getElapsedSeconds()-mSelectionTime, 0.0, duration );
	double p        = easeInOutCubic( t / duration );

	mCenter			= lerp( mCenterFrom, (mCenterDest + mCenterOffset), (float)p );
	mCamDist		= lerp( mCamDistFrom, mCamDistDest, p );
	mCamDist		= min( mCamDist, G_INIT_CAM_DIST );

	G_ZOOM			= lerp( mZoomFrom, mZoomDest, p );

	
	mFadeInAlphaToArtist	= constrain( G_ZOOM - G_ALPHA_LEVEL, 0.0f, 1.0f );
	mFadeInArtistToAlbum	= constrain( G_ZOOM - G_ARTIST_LEVEL, 0.0f, 1.0f );
	mFadeInAlbumToTrack		= constrain( G_ZOOM - G_ALBUM_LEVEL, 0.0f, 1.0f );
	

    // apply the Arcball to the camera eye/up vectors
    // (instead of to the whole scene)
    quat q = mArcball.getQuat();
    // The original shipped with "FIXME: what causes camera to sometimes
    // destroy itself?" — a NaN here is sticky, because the arcball
    // renormalises its quat on every drag. Restore the last good orientation
    // rather than letting one bad frame black the scene out permanently.
    static quat sLastGoodQuat = q;
    if( isnan( q.w ) || isnan( q.x ) || isnan( q.y ) || isnan( q.z ) ) {
        mArcball.setQuat( sLastGoodQuat );
        q = sLastGoodQuat;
    } else {
        sLastGoodQuat = q;
    }
    q.w *= -1.0; // reverse the angle, keep the axis
	if( G_IS_IPAD2 && G_USE_GYRO ){
		q = mGyroHelper.getQuat();
	}
	
    // set up vector according to screen orientation
	mUp = vec3(0,1,0);
	if( !G_USE_GYRO ){
        mUp = glm::rotateZ( mUp, -1.0f * mOrientationNodeRef->getInterfaceAngle() );
    }
    
    vec3 camOffset = q * vec3( 0, 0, mCamDist);
    mEye = mCenter - camOffset;


    // FIXME: what causes camera to sometimes destroy itself?
//    if( (isnan(mEye.x) || isnan(mEye.y) || isnan(mEye.z)) ) {
//        std::cout << mEye << std::endl;
//        std::cout << camOffset << std::endl;
//        std::cout << q << std::endl;
//        std::cout << mCamDist << std::endl;
//        std::cout << mCenter << std::endl;        
//    }

	mCam.setPerspective( mFov, getWindowAspectRatio(), 0.001f, 2000.0f );
	mCam.lookAt( mEye - mCenterOffset, mCenter, q * mUp );
}

void KeplerApp::draw()
{
	gl::clear( Color( 0, 0, 0 ), true );
	if( mData.getState() != Data::LoadStateComplete ){
        // just for loading sc
		mBloomSceneRef->deepDraw();
	} else if( mData.mArtists.size() == 0 ){
		drawNoArtists();
	} else {
		drawScene();
	}
    
    // The old glDiscardFramebufferEXT was an ES1/ES2 OES extension. It was
    // only a bandwidth hint, so dropping it costs nothing but a little fill.
}

void KeplerApp::drawNoArtists()
{
    if( !mNoArtistsTex ) {
        mNoArtistsTex = gl::Texture::create( loadImage( loadResource( "noArtists.png" ) ) );
    }
    
	gl::setMatricesWindow( getWindowSize() );    
	
    gl::pushModelMatrix();
    gl::multModelMatrix(mOrientationMatrix);
	vec2 interfaceSize = getWindowSize();
	if( isLandscapeOrientation( mInterfaceOrientation ) ){
		interfaceSize = vec2( interfaceSize.y, interfaceSize.x );
	}
    vec2 center = interfaceSize * 0.5f;
    gl::color( Color::white() );
	
	mNoArtistsTex->bind();
	vec2 v1( center - vec2( mNoArtistsTex->getSize() ) * 0.5f );
	vec2 v2( v1 + vec2( mNoArtistsTex->getSize() ) );
	gl::color( ColorA( 1.0f, 1.0f, 1.0f, 1.0f ) );
	gl::drawSolidRect( Rectf( v1, v2 ) );
	mNoArtistsTex->unbind();
}



void KeplerApp::drawScene()
{	
	vector<Node*> unsortedNodes = mWorld.getUnsortedNodes( G_ALBUM_LEVEL, G_TRACK_LEVEL );
	Node *artistNode = mState.getSelectedArtistNode();
	if( artistNode ){
		unsortedNodes.push_back( artistNode );
	}
	vector<Node*> sortedNodes = mWorld.sortNodes( unsortedNodes );

    gl::enableDepthWrite();
    gl::setMatrices( mCam );
    
// SKYDOME
    Color c = Color( CM_HSV, mPinchPer * 0.2f + 0.475f, 1.0f - mPinchPer * 0.5f, 1.0f );
    if( mIsPastPinchThresh )
        c = Color( CM_HSV, mPinchPer * 0.3f + 0.7f, 1.0f, 1.0f );
    
    if( artistNode && artistNode->mDistFromCamZAxis > 0.0f ){
		float distToCenter = min( ( glm::length( getWindowCenter() - artistNode->mScreenPos )/80.0f ) + ( 1.0f - mFadeInArtistToAlbum ), 1.0f );
        gl::color( lerp( ( artistNode->mGlowColor + BRIGHT_BLUE ) * 0.15f, BRIGHT_BLUE, distToCenter ) * mFadeInAlphaToArtist );

    } else {
        gl::color( BRIGHT_BLUE * mFadeInAlphaToArtist );
    }
  //  gl::color( c * pow( 1.0f - zoomOff, 3.0f ) );
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    mTextures[SKY_DOME_TEX]->bind();
    gl::pushModelMatrix();
    gl::scale( vec3(G_SKYDOME_RADIUS,G_SKYDOME_RADIUS,G_SKYDOME_RADIUS) );
    mSkySphere.draw();
    gl::popModelMatrix();
    mTextures[SKY_DOME_TEX]->unbind();
    glDisable(GL_CULL_FACE);
    
// GALAXY
    mGalaxy.drawLightMatter( mFadeInAlphaToArtist );	
    mGalaxy.drawSpiralPlanes();    
    mGalaxy.drawCenter();
	
// STARS
	gl::enableAdditiveBlending();
	mTextures[STAR_TEX]->bind();
	mWorld.drawStarsVertexArray();
	mTextures[STAR_TEX]->unbind();
	
// STARGLOWS bloom (TOUCH HIGHLIGHTS)
	mTextures[ECLIPSE_GLOW_TEX]->bind();
	mWorld.drawTouchHighlights( mFadeInArtistToAlbum );
	mTextures[ECLIPSE_GLOW_TEX]->unbind();
	
// STARGLOWS bloom
	mStarGlowTex->bind();
	mWorld.drawStarGlowsVertexArray();
	mStarGlowTex->unbind();

	if( artistNode ){ // defined at top of method
		artistNode->drawStarGlow( mEye - mCenterOffset, glm::normalize(( mEye - mCenter )), mStarGlowTex );
    }
		
    vec2 interfaceSize = getWindowSize();
    
    //float zoomOffset = constrain( 1.0f - ( G_ALBUM_LEVEL - G_ZOOM ), 0.0f, 1.0f );
    if (artistNode) {
        mCamRingAlpha = constrain( abs( mEye.y - artistNode->mPos.y ), 0.0f, 1.0f ); // WAS 0.6f
    }

    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );

    // Planet lighting. The original used two GL_LIGHTs at the artist node —
    // its own colour plus a blue fill — which ES3 has no equivalent for; see
    // PlanetLighting for the shader that replaces them. glLightfv transformed
    // its position by the modelview in force at the time, i.e. view space, so
    // the position is transformed here to match.
    if( artistNode ) {
        vec3 lightPosView = vec3( mCam.getViewMatrix() * vec4( artistNode->mPos, 1.0f ) );
        bloom::setPlanetLight( lightPosView, artistNode->mColor, BRIGHT_BLUE );
    }
    else {
        // Neither light was enabled without a selected artist, leaving only
        // the light model's ambient.
        bloom::setPlanetLightOff();
    }
    
    for( int i = 0; i < sortedNodes.size(); i++ ){
        
        if( (G_IS_IPAD2 || G_DEBUG) && sortedNodes[i]->mGen == G_ALBUM_LEVEL ){ // JUST ALBUM LEVEL CAUSE ALBUM TELLS CHILDREN TO ALSO FIND SHADOWS
            gl::enableAlphaBlending();
            glDisable( GL_CULL_FACE );
            mTextures[ECLIPSE_SHADOW_TEX]->bind();
            sortedNodes[i]->findShadows( pow( mCamRingAlpha, 1.2f ) );
            glEnable( GL_CULL_FACE );
            mTextures[ECLIPSE_SHADOW_TEX]->unbind();
            //gl::enableDepthWrite();
        }
        
        gl::enableDepthRead();

        gl::disableAlphaBlending(); // dings additive blending            
        gl::enableAlphaBlending();  // restores alpha blending

        sortedNodes[i]->drawPlanet( mTextures[STAR_CORE_TEX] ); // star core tex for artistars, planets do their own thing
        sortedNodes[i]->drawClouds( mCloudTextures );
        
        gl::disableDepthRead();
        
        gl::enableAdditiveBlending();
        if( sortedNodes[i]->mGen == G_ARTIST_LEVEL ) {
            sortedNodes[i]->drawAtmosphere( mEye - mCenterOffset, interfaceSize * 0.5f, mTextures[ATMOSPHERE_SUN_TEX], mTextures[ATMOSPHERE_DIRECTIONAL_TEX], mPinchAlphaPer, 0.0f );
        } else {
            float scaleSliderOffset = mSettingsPanel.getParamSlider1Value() * 0.01f;
            sortedNodes[i]->drawAtmosphere( mEye - mCenterOffset, interfaceSize * 0.5f, mTextures[ATMOSPHERE_TEX], mTextures[ATMOSPHERE_DIRECTIONAL_TEX], mPinchAlphaPer, scaleSliderOffset );
        }
    }
        
    glDisable( GL_CULL_FACE );

    if (artistNode) {
		gl::enableAdditiveBlending();
		artistNode->drawExtraGlow( mEye - mCenterOffset, mStarGlowTex, mTextures[STAR_TEX] );
	}
    
	gl::enableDepthRead();	
	gl::disableDepthWrite();
	
	
// ORBITS
	if( G_DRAW_RINGS ){
        mTextures[ORBIT_RING_GRADIENT_TEX]->bind();
        mWorld.drawOrbitRings( mPinchAlphaPer, sqrt( mCamRingAlpha ), mFadeInAlphaToArtist, mFadeInArtistToAlbum );
        mTextures[ORBIT_RING_GRADIENT_TEX]->unbind();
	}
	
// PARTICLES
	if( artistNode ){
        mTextures[PARTICLE_TEX]->bind();
		mParticleController.drawParticleVertexArray( artistNode, 0.175f );
        mTextures[PARTICLE_TEX]->unbind();
	}
//	Node *albumNode = mState.getSelectedAlbumNode();
//	if( albumNode ){
//		mTextures[PARTICLE_TEX]->bind();
//		mParticleController.drawParticleVertexArray( albumNode, 50.0f );
//        mTextures[PARTICLE_TEX]->unbind();
//	}
	
	
// RINGS
	if( artistNode ){
		//alpha = pow( mCamRingAlpha, 2.0f );
        // TODO: consider only doing this on planets in our sorted loop, above
		mWorld.drawRings( mTextures[RINGS_TEX], mCamRingAlpha * 0.5f );
	}
	
// FOR dust, playhead progress, constellations, etc.
    gl::enableAlphaBlending();    
    gl::enableAdditiveBlending();    
	
// DUSTS
	if( artistNode && (G_IS_IPAD2 || G_DEBUG) ){
		mParticleController.drawDustVertexArray( artistNode, 0.175f );
	}
	
// PLAYHEAD PROGRESS
	if( G_DRAW_RINGS && mWorld.mPlayingTrackNode && G_ZOOM > G_ARTIST_LEVEL ){
        const bool isPaused = (mCurrentPlayState == music::Player::StatePaused);
        const bool isStopped = (mCurrentPlayState == music::Player::StateStopped);
        const float pauseAlpha = (isPaused || isStopped) ? sin(getElapsedSeconds() * TWO_PI ) * 0.25f + 0.75f : 1.0f;
        mWorld.mPlayingTrackNode->drawPlayheadProgress( mPinchAlphaPer, mCamRingAlpha, pauseAlpha, mTextures[PLAYHEAD_PROGRESS_TEX], mTextures[TRACK_ORIGIN_TEX] );
	}
	
// CONSTELLATION
	if( mWorld.getNumFilteredNodes() > 1 && G_DRAW_RINGS ){
		mTextures[DOTTED_TEX]->bind();
		mWorld.drawConstellation();
		mTextures[DOTTED_TEX]->unbind();
	}
	
	
// SPLINE
//	const int numSegments = 200;
//	gl::color( ColorA( 0.8f, 0.2f, 0.8f, 0.5f ) );
//	vec3 pos	  = mEye;
//	vec3 prevPos = pos;
//	for( int s = 0; s <= numSegments; ++s ) {
//		float t = s / (float)numSegments;
//		prevPos = pos;
//		pos		= mSpline.getPosition( t );
//		gl::drawLine( pos + vec3( 0.1f, 0.1f, 0.1f ), prevPos + vec3( 0.1f, 0.1f, 0.1f ) );
//	}
	
	
// GALAXY DARK MATTER:
//    if (G_IS_IPAD2 || G_DEBUG) {
//		gl::enableAlphaBlending();    
//        mGalaxy.drawDarkMatter( galaxyRotationMulti );
//    }
	
// NAMES
	gl::disableDepthRead();
	gl::disableDepthWrite();
	gl::setMatricesWindow( getWindowSize() ); 
    gl::enableAdditiveBlending();
	
    if( G_DRAW_TEXT ){
//        std::cout << "drawing names with alpha " << mPinchAlphaPer << std::endl;
		mWorld.drawNames( mCam, mPinchAlphaPer, getAngleForOrientation(mInterfaceOrientation) );
	}
		
// LENSFLARE?!?!  SURELY YOU MUST BE MAD.
	if( (G_IS_IPAD2 || G_DEBUG) && artistNode && artistNode->mDistFromCamZAxis > 0.0f && artistNode->mEclipseStrength < 0.75f ){
		int numFlares  = 4;
		float radii[5] = { 4.5f, 12.0f, 18.0f, 13.0f };
		float dists[5] = { 1.25f, 2.50f, 6.0f, 8.5f };
		
		float distPer = constrain( 0.5f - artistNode->mScreenDistToCenterPer, 0.0f, 1.0f );
		float alpha = distPer * 0.2f * mFadeInArtistToAlbum * sin( distPer * M_PI );

        gl::enableAlphaBlending();    
        gl::enableAdditiveBlending();		
        mTextures[LENS_FLARE_TEX]->bind();
		
		vec2 flarePos = getWindowCenter() - artistNode->mScreenPos;
		float flareDist = glm::length(flarePos);
		vec2 flarePosNorm = glm::normalize(flarePos);

		gl::color( ColorA( BRIGHT_BLUE, alpha ) );
		for( int i=0; i<numFlares; i++ ){
			flarePos = getWindowCenter() + flarePosNorm * dists[i] * flareDist;
			float flareRadius = artistNode->mSphereScreenRadius * radii[i] * distPer;
			gl::drawSolidRect( Rectf( flarePos.x - flareRadius, flarePos.y - flareRadius, flarePos.x + flareRadius, flarePos.y + flareRadius ) );
		}

        mTextures[LENS_FLARE_TEX]->unbind();
	}

	
// PINCH FINGER POSITIONS
	if( mIsPinching ) {
		float radius = mPinchHighlightRadius;
		float alpha = mPinchPer > mPinchPerThresh ? 0.2f : 1.0f;
        mStarGlowTex->bind();					  
		gl::color( ColorA( c, max( mPinchPer - mPinchPerInit, 0.0f ) * alpha ) );
		for( vector<vec2>::iterator it = mPinchPositions.begin(); it != mPinchPositions.end(); ++it ){
			gl::drawSolidRect( Rectf( it->x - radius, it->y - radius, it->x + radius, it->y + radius ) );
		}
		mStarGlowTex->unbind();                
	} 
    else if( mIsTouching ){
		float radius = 100.0f;
		float alpha = 0.5f;
		mStarGlowTex->bind();
		gl::color( ColorA( BLUE, alpha ) );		
		gl::drawSolidRect( Rectf( mTouchPos.x - radius, mTouchPos.y - radius, mTouchPos.x + radius, mTouchPos.y + radius ) );
		mStarGlowTex->unbind();        
	}

//    if (G_DEBUG) {
//        mWorld.drawHitAreas();
//    }

	gl::disableAlphaBlending(); // stops additive blending
    gl::enableAlphaBlending();  // reinstates normal alpha blending
    
	// GALAXY DARK MATTER:
    if (G_IS_IPAD2 || G_DEBUG) {
		gl::setMatrices( mCam );
		gl::enableDepthRead();
		gl::enableDepthWrite();
        mGalaxy.drawDarkMatter();
		gl::disableDepthRead();
		gl::disableDepthWrite();
    }
    else {
		gl::disableDepthRead();
		gl::disableDepthWrite();        
    }
	
    // UILayer and PlayControls draw here:
    mBloomSceneRef->deepDraw();
}

bool KeplerApp::onPlayerLibraryChanged( music::Player *player )
{	
    // RESET:
	mLoadingScreen.setVisible( true ); // TODO: reload textures, add back to mBloomSceneRef
    mMainBloomNodeRef->setVisible( false );    
    mPlaylistChooser.clearTextures();
    mState.setup();
    mLoadingScreen.setArtistProgress( -1 );
    mLoadingScreen.setPlaylistProgress( -1 );    
    mData.setup();
	mWorld.setup();
    logEvent("Player Library Changed");
    return false;
}

bool KeplerApp::onPlayerTrackChanged( music::Player *player )
{	   
//    logEvent("Player Track Changed");

    if (mPlayControls.isPlayheadDragging()) {
        mPlayControls.cancelPlayheadDrag();
        mPlayControls.setPlayheadValue(0.0f);
        mMusicPlayer.setPlayheadTime( 0.0f );        
    }
    
	if (mMusicPlayer.hasPlayingTrack()) {

        // to be sure...
        mPlayControls.enablePlayerControls();                    
        
        // temporarily remember the previous track info
        music::TrackRef previousTrack = mPlayingTrack;
        
        // cache the new track
        mPlayingTrack = mMusicPlayer.getPlayingTrack();

        // only ask for id once
        uint64_t trackId = mPlayingTrack->getItemId();
        
        if (previousTrack && previousTrack->getItemId() == trackId) {
            // skip spurious change event
            return false;
        }

        // remember the previous node
        Node* prevSelectedNode = mState.getSelectedNode();
        
        // update track stats
        mCurrentTrackLength = mPlayingTrack->getLength();
        
        // reset the mPlayingTrackNode orbit and playhead displays (see update())
        mPlayheadUpdateSeconds = -1;
        
        string artistName = mPlayingTrack->getArtist();
        
        mPlayControls.setCurrentTrack( " " + artistName
                                      + " • " + mPlayingTrack->getAlbumTitle() 
                                      + " • " + mPlayingTrack->getTitle() + " " );

        uint64_t artistId = mPlayingTrack->getArtistId();
        uint64_t albumId = mPlayingTrack->getAlbumId();            
        
        // we're only going to fly to the track if we were already looking at the previous track
        // or fly to the album if we were looking at the previous album
        // or fly to the artist if we were looking at the previous artist
        bool flyingAround = false;
        if (previousTrack && prevSelectedNode != NULL) {            
            if (prevSelectedNode->mGen == G_TRACK_LEVEL) {
                if (previousTrack->getItemId() == prevSelectedNode->getId()) {
                    if (prevSelectedNode->getId() != trackId) {
                        flyToCurrentTrack();
                        flyingAround = true;
                    }
                }
            } 
            else if (prevSelectedNode->mGen == G_ALBUM_LEVEL) {
                if (previousTrack->getAlbumId() == prevSelectedNode->getId()) {
                    if (prevSelectedNode->getId() != albumId) {
                        flyToCurrentAlbum();
                        flyingAround = true;
                    }
                }
            }
            else if (prevSelectedNode->mGen == G_ARTIST_LEVEL) {
                if (previousTrack->getArtistId() == prevSelectedNode->getId()) {
                    if (prevSelectedNode->getId() != artistId) {
                        flyToCurrentArtist();
                        flyingAround = true;
                    }
                }
            } 
        }
        
        if (!flyingAround) {
            
            if( mState.getFilterMode() == State::FilterModePlaylist ) {

                // let's see if we need to switch to alpha mode...
                music::PlaylistRef playlist = mState.getPlaylist();                
                bool playingTrackIsInPlaylist = false;

                // make sure the ipod playlist is the one we have in State...
                if (playlist == mMusicPlayer.getCurrentPlaylist()) {
                    // so then maybe this track is in the playlist
                    for (int i = 0; i < playlist->size(); i++) {
                        if ((*playlist)[i]->getItemId() == trackId) {
                            playingTrackIsInPlaylist = true;
                            break;
                        }
                    }
                }
                
                // if it's not we need to switch to alphabet mode...
                if (!playingTrackIsInPlaylist) {
                    // trigger hefty stuff in onFilterModeStateChanged:
                    mState.setFilterMode( State::FilterModeAlphaChar );            
                    // trigger hefty stuff in onAlphaCharStateChanged:
                    mState.setAlphaChar( artistName ); 
                }
            }
            
            // just sync the mIsPlaying state for all nodes and update mWorld.mPlayingTrackNode...
            mWorld.updateIsPlaying( artistId, albumId, trackId );
        }        
	}
	else {
        
        // can't assign shared pointers to NULL, so this is it:
        mPlayingTrack.reset();
        mCurrentTrackLength = 0; // reset cached track stuff

        // calibrate time labels and orbit positions in update()
        mPlayheadUpdateSeconds = -1;
        
        // tidy up
        mPlayControls.setCurrentTrack("");
        
        // this should be OK to do since the above will happen if something is queued and paused
        mWorld.updateIsPlaying( 0, 0, 0 );        
        
        if (mState.getSelectedNode() == NULL) {
            mPlayControls.disablePlayerControls();
        }
        else {
            mPlayControls.enablePlayerControls();            
        }
	}

    return false;
}

bool KeplerApp::onPlayerStateChanged( music::Player *player )
{	
    static bool firstRun = true;
    
    music::Player::State prevPlayState = mCurrentPlayState;
    const bool wasPaused = (prevPlayState == music::Player::StatePaused);
    
    // this should be the only call to getPlayState() apart from during setup()
    // TODO: modify CinderIPod library to pass the new play state directly
    mCurrentPlayState = mMusicPlayer.getPlayState();

    // update UI:
    const bool isPlaying = (mCurrentPlayState == music::Player::StatePlaying);
    mPlayControls.setPlayingOn(isPlaying);
    
    // be sure the track moon and elapsed time things get an update:
    mPlayheadUpdateSeconds = -1;    

    // make sure mCurrentTrack and mWorld.mPlayingTrackNode are taken care of,
    // unless we're just continuing to play a track we're already aware of
    if ((!wasPaused && isPlaying) || firstRun) {
        onPlayerTrackChanged( player );
    }
    
    // do stats:
//    std::map<string, string> params;
//    params["State"] = mMusicPlayer.getPlayStateString();
//    logEvent("Player State Changed", params);
    
    firstRun = false;
    
    return false;
}

// Analytics hooks. Flurry was removed when the service shut down; these are
// kept as no-ops so the call sites still document what was worth measuring,
// and so a replacement can be dropped in at one place.
void KeplerApp::logEvent(const string &event)
{
    if (G_DEBUG) std::cout << "logging: " << event << std::endl;
}
void KeplerApp::logEvent(const string &event, const map<string,string> &params)
{
    if (G_DEBUG) std::cout << "logging: " << event << " (" << params.size() << " params)" << std::endl;
}



CINDER_APP_COCOA_TOUCH( KeplerApp, RendererGl )
