//
//  SpotifyAuth.mm
//  CinderSpotify
//

#include "SpotifyAuth.h"

#import <AuthenticationServices/AuthenticationServices.h>
#import <CommonCrypto/CommonDigest.h>
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include <mutex>

namespace {

NSString* const kAuthorizeUrl = @"https://accounts.spotify.com/authorize";
NSString* const kTokenUrl     = @"https://accounts.spotify.com/api/token";

//! base64url, i.e. base64 with the URL-safe alphabet and no padding.
NSString* base64Url( NSData *data )
{
	NSString *s = [data base64EncodedStringWithOptions:0];
	s = [s stringByReplacingOccurrencesOfString:@"+" withString:@"-"];
	s = [s stringByReplacingOccurrencesOfString:@"/" withString:@"_"];
	s = [s stringByReplacingOccurrencesOfString:@"=" withString:@""];
	return s;
}

NSString* randomVerifier()
{
	uint8_t bytes[64];
	// Cryptographic randomness matters: a guessable verifier defeats PKCE.
	if( SecRandomCopyBytes( kSecRandomDefault, sizeof(bytes), bytes ) != errSecSuccess )
		return nil;
	return base64Url( [NSData dataWithBytes:bytes length:sizeof(bytes)] );
}

NSString* challengeFor( NSString *verifier )
{
	NSData *ascii = [verifier dataUsingEncoding:NSASCIIStringEncoding];
	uint8_t digest[CC_SHA256_DIGEST_LENGTH];
	CC_SHA256( ascii.bytes, (CC_LONG)ascii.length, digest );
	return base64Url( [NSData dataWithBytes:digest length:sizeof(digest)] );
}

// Tokens are persisted so a returning user is not asked to sign in again.
// The refresh token is the sensitive one: it is long-lived and can mint access
// tokens indefinitely. NSUserDefaults is plain-text within the app container —
// adequate on a device, where the container is sandboxed and encrypted at rest,
// but the Keychain would be the stricter home if this ever ships.
NSString* const kDefaultsAccessToken  = @"CinderSpotifyAccessToken";
NSString* const kDefaultsRefreshToken = @"CinderSpotifyRefreshToken";
NSString* const kDefaultsExpiresAt    = @"CinderSpotifyExpiresAt";

NSString* infoPlistString( NSString *key )
{
	id value = [[NSBundle mainBundle].infoDictionary objectForKey:key];
	return [value isKindOfClass:[NSString class]] && [value length] > 0 ? value : nil;
}

} // anonymous namespace

// ASWebAuthenticationSession needs somewhere to present from. Cinder owns the
// only window, so hand back whichever one is currently key.
@interface CinderSpotifyPresentationContext : NSObject <ASWebAuthenticationPresentationContextProviding>
@end

@implementation CinderSpotifyPresentationContext
- (ASPresentationAnchor)presentationAnchorForWebAuthenticationSession:(ASWebAuthenticationSession *)session
{
	for( UIScene *scene in [UIApplication sharedApplication].connectedScenes ) {
		if( [scene isKindOfClass:[UIWindowScene class]] ) {
			for( UIWindow *window in ((UIWindowScene *)scene).windows ) {
				if( window.isKeyWindow )
					return window;
			}
		}
	}
	return nil;
}
@end

namespace cinder { namespace spotify {

const char* Auth::kScopes =
	"user-library-read "          // saved albums and tracks
	"user-follow-read "           // followed artists
	"playlist-read-private "      // the user's own playlists
	"playlist-read-collaborative "
	"user-read-playback-state "   // what is playing, for the transport
	"user-modify-playback-state"; // play/pause/skip/seek

struct Auth::Impl {
	std::mutex   mutex;
	NSString    *accessToken  = nil;
	NSString    *refreshToken = nil;
	NSDate      *expiresAt    = nil;

	ASWebAuthenticationSession        *session = nil;
	CinderSpotifyPresentationContext  *context = nil;
};

Auth::Auth()
	: mImpl( new Impl )
{
	mImpl->context = [[CinderSpotifyPresentationContext alloc] init];

	// Restore a previous session. An expired access token is kept rather than
	// discarded: blockingAccessToken() will refresh it, and throwing it away
	// would force a needless sign-in.
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	mImpl->accessToken  = [defaults stringForKey:kDefaultsAccessToken];
	mImpl->refreshToken = [defaults stringForKey:kDefaultsRefreshToken];
	double expiry = [defaults doubleForKey:kDefaultsExpiresAt];
	if( expiry > 0 )
		mImpl->expiresAt = [NSDate dateWithTimeIntervalSince1970:expiry];
}

void Auth::persist()
{
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	if( mImpl->accessToken )
		[defaults setObject:mImpl->accessToken forKey:kDefaultsAccessToken];
	if( mImpl->refreshToken )
		[defaults setObject:mImpl->refreshToken forKey:kDefaultsRefreshToken];
	if( mImpl->expiresAt )
		[defaults setDouble:[mImpl->expiresAt timeIntervalSince1970] forKey:kDefaultsExpiresAt];
}

Auth& Auth::instance()
{
	static Auth sAuth;
	return sAuth;
}

bool Auth::isAuthorized() const
{
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	// A held refresh token counts: the access token can be renewed without
	// involving the user, so a restored-but-expired session is still usable.
	return mImpl->accessToken != nil || mImpl->refreshToken != nil;
}

void Auth::signOut()
{
	std::lock_guard<std::mutex> lock( mImpl->mutex );
	mImpl->accessToken  = nil;
	mImpl->refreshToken = nil;
	mImpl->expiresAt    = nil;

	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	[defaults removeObjectForKey:kDefaultsAccessToken];
	[defaults removeObjectForKey:kDefaultsRefreshToken];
	[defaults removeObjectForKey:kDefaultsExpiresAt];
}

void Auth::authorize( std::function<void(bool, const std::string&)> completion )
{
	if( isAuthorized() ) {
		// A restored session may hold only an expired access token, so prove it
		// can still be renewed before reporting success — otherwise the library
		// loaders would run against a dead token and quietly return nothing.
		dispatch_async( dispatch_get_global_queue( QOS_CLASS_UTILITY, 0 ), ^{
			bool usable = ! blockingAccessToken().empty();
			dispatch_async( dispatch_get_main_queue(), ^{
				if( usable ) {
					completion( true, "" );
				}
				else {
					signOut();          // stale; fall through to a fresh sign-in
					authorize( completion );
				}
			} );
		} );
		return;
	}

	NSString *clientId    = infoPlistString( @"SpotifyClientID" );
	NSString *redirectUri = infoPlistString( @"SpotifyRedirectURI" );
	if( ! clientId || ! redirectUri ) {
		dispatch_async( dispatch_get_main_queue(), ^{
			completion( false, "SpotifyClientID and SpotifyRedirectURI must be set in Info.plist." );
		} );
		return;
	}

	NSString *verifier = randomVerifier();
	if( ! verifier ) {
		dispatch_async( dispatch_get_main_queue(), ^{
			completion( false, "Could not generate a PKCE verifier." );
		} );
		return;
	}

	// The state parameter is checked on return, so a callback that did not
	// originate from this request is rejected.
	NSString *state = randomVerifier();

	NSURLComponents *components = [NSURLComponents componentsWithString:kAuthorizeUrl];
	components.queryItems = @[
		[NSURLQueryItem queryItemWithName:@"client_id"             value:clientId],
		[NSURLQueryItem queryItemWithName:@"response_type"         value:@"code"],
		[NSURLQueryItem queryItemWithName:@"redirect_uri"          value:redirectUri],
		[NSURLQueryItem queryItemWithName:@"scope"                 value:@(kScopes)],
		[NSURLQueryItem queryItemWithName:@"state"                 value:state],
		[NSURLQueryItem queryItemWithName:@"code_challenge_method" value:@"S256"],
		[NSURLQueryItem queryItemWithName:@"code_challenge"        value:challengeFor( verifier )],
	];

	NSString *scheme = [redirectUri componentsSeparatedByString:@":"].firstObject;
	Impl *impl = mImpl;

	// ASWebAuthenticationSession must be created and started on the main thread.
	dispatch_async( dispatch_get_main_queue(), ^{
		impl->session = [[ASWebAuthenticationSession alloc]
			initWithURL:components.URL
			callbackURLScheme:scheme
			completionHandler:^(NSURL *callbackUrl, NSError *error) {
				if( error || ! callbackUrl ) {
					BOOL cancelled = error.code == ASWebAuthenticationSessionErrorCodeCanceledLogin;
					completion( false, cancelled ? "Sign-in cancelled."
					                             : error.localizedDescription.UTF8String );
					return;
				}

				NSURLComponents *back = [NSURLComponents componentsWithURL:callbackUrl
				                                   resolvingAgainstBaseURL:NO];
				NSString *code = nil, *returnedState = nil;
				for( NSURLQueryItem *item in back.queryItems ) {
					if( [item.name isEqualToString:@"code"] )  code = item.value;
					if( [item.name isEqualToString:@"state"] ) returnedState = item.value;
				}

				if( ! [returnedState isEqualToString:state] ) {
					completion( false, "Sign-in state mismatch; the callback was not ours." );
					return;
				}
				if( ! code ) {
					completion( false, "Spotify did not return an authorization code." );
					return;
				}

				// Exchange the code for tokens.
				NSMutableURLRequest *request =
					[NSMutableURLRequest requestWithURL:[NSURL URLWithString:kTokenUrl]];
				request.HTTPMethod = @"POST";
				[request setValue:@"application/x-www-form-urlencoded"
				         forHTTPHeaderField:@"Content-Type"];

				NSURLComponents *body = [[NSURLComponents alloc] init];
				body.queryItems = @[
					[NSURLQueryItem queryItemWithName:@"client_id"     value:clientId],
					[NSURLQueryItem queryItemWithName:@"grant_type"    value:@"authorization_code"],
					[NSURLQueryItem queryItemWithName:@"code"          value:code],
					[NSURLQueryItem queryItemWithName:@"redirect_uri"  value:redirectUri],
					[NSURLQueryItem queryItemWithName:@"code_verifier" value:verifier],
				];
				request.HTTPBody = [body.query dataUsingEncoding:NSUTF8StringEncoding];

				[[[NSURLSession sharedSession] dataTaskWithRequest:request
					completionHandler:^(NSData *data, NSURLResponse *response, NSError *taskError) {
						if( taskError || ! data ) {
							dispatch_async( dispatch_get_main_queue(), ^{
								completion( false, taskError.localizedDescription.UTF8String );
							} );
							return;
						}
						NSDictionary *json = [NSJSONSerialization JSONObjectWithData:data
						                                                     options:0
						                                                       error:nil];
						NSString *token = json[@"access_token"];
						if( ! token ) {
							NSString *why = json[@"error_description"] ?: @"Token exchange failed.";
							dispatch_async( dispatch_get_main_queue(), ^{
								completion( false, why.UTF8String );
							} );
							return;
						}
						{
							std::lock_guard<std::mutex> lock( impl->mutex );
							impl->accessToken  = token;
							impl->refreshToken = json[@"refresh_token"];
							NSNumber *expiresIn = json[@"expires_in"];
							impl->expiresAt = [NSDate dateWithTimeIntervalSinceNow:expiresIn.doubleValue];
							Auth::instance().persist(); // under the lock, as required
						}
						dispatch_async( dispatch_get_main_queue(), ^{ completion( true, "" ); } );
					}] resume];
			}];

		impl->session.presentationContextProvider = impl->context;
		// Share the Safari cookie jar, so an already signed-in user is not
		// asked for a password.
		impl->session.prefersEphemeralWebBrowserSession = NO;
		[impl->session start];
	} );
}

std::string Auth::blockingAccessToken()
{
	NSString *token      = nil;
	NSString *refresh    = nil;
	BOOL      needsRefresh = NO;
	{
		std::lock_guard<std::mutex> lock( mImpl->mutex );
		token   = mImpl->accessToken;
		refresh = mImpl->refreshToken;
		// 60s of slack, so a token cannot expire between here and the request.
		needsRefresh = mImpl->expiresAt &&
		               [mImpl->expiresAt timeIntervalSinceNow] < 60.0;
	}

	if( ! token )
		return std::string();
	if( ! needsRefresh )
		return token.UTF8String;
	if( ! refresh )
		return std::string();

	NSString *clientId = infoPlistString( @"SpotifyClientID" );
	if( ! clientId )
		return std::string();

	NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:kTokenUrl]];
	request.HTTPMethod = @"POST";
	[request setValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];

	NSURLComponents *body = [[NSURLComponents alloc] init];
	body.queryItems = @[
		[NSURLQueryItem queryItemWithName:@"client_id"     value:clientId],
		[NSURLQueryItem queryItemWithName:@"grant_type"    value:@"refresh_token"],
		[NSURLQueryItem queryItemWithName:@"refresh_token" value:refresh],
	];
	request.HTTPBody = [body.query dataUsingEncoding:NSUTF8StringEncoding];

	__block NSData *result = nil;
	dispatch_semaphore_t done = dispatch_semaphore_create( 0 );
	[[[NSURLSession sharedSession] dataTaskWithRequest:request
		completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
			result = data;
			dispatch_semaphore_signal( done );
		}] resume];
	dispatch_semaphore_wait( done, dispatch_time( DISPATCH_TIME_NOW, 30ll * NSEC_PER_SEC ) );

	if( ! result )
		return std::string();

	NSDictionary *json = [NSJSONSerialization JSONObjectWithData:result options:0 error:nil];
	NSString *fresh = json[@"access_token"];
	if( ! fresh )
		return std::string();

	std::lock_guard<std::mutex> lock( mImpl->mutex );
	mImpl->accessToken = fresh;
	// Spotify usually omits a new refresh token; keep the one we have.
	if( json[@"refresh_token"] )
		mImpl->refreshToken = json[@"refresh_token"];
	NSNumber *expiresIn = json[@"expires_in"];
	mImpl->expiresAt = [NSDate dateWithTimeIntervalSinceNow:expiresIn.doubleValue];
	persist();
	return fresh.UTF8String;
}

} } // namespace cinder::spotify
