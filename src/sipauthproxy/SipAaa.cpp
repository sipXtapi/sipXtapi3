// 
// 
// Copyright (C) 2004 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
// 
// Copyright (C) 2004 Pingtel Corp.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// $$
//////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
#include <assert.h>


// APPLICATION INCLUDES
#include "os/OsFS.h"
#include "os/OsConfigDb.h"
#include "os/OsQueuedEvent.h"
#include "os/OsSysLog.h"
#include "os/OsTimer.h"
#include "os/OsEventMsg.h"
#include "net/SipUserAgent.h"
#include "net/NetMd5Codec.h"
#include "sipdb/ResultSet.h"
#include "sipdb/CredentialDB.h"
#include "sipdb/PermissionDB.h"
#include "sipdb/AuthexceptionDB.h"
#include "digitmaps/UrlMapping.h"
#include "SipAaa.h"

// DEFINES
//#define TEST_PRINT 1
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS

extern UtlString gUriKey;
extern UtlString gCallidKey;
extern UtlString gContactKey;
extern UtlString gQvalueKey;
extern UtlString gCseqKey;
extern UtlString gExpiresKey;

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
SipAaa::SipAaa(SipUserAgent& sipUserAgent,
               const char* authenticationRealm,
               UtlString& routeName) :
   OsServerTask("SipAaa-%d", NULL, 2000)
{
    mpSipUserAgent = &sipUserAgent;
    mRealm = authenticationRealm ? authenticationRealm : "";

    // Initialize the outbound authorization rules
    mpAuthorizationRules = new UrlMapping();

    // The name to appear in the route header for the authproxy
    mRouteName = routeName;

    char signBuf[100];
    //int ranNum = rand();
    //long epochTime = OsDateTime::getSecsSinceEpoch();
    UtlString viaHost;
    int viaPort;
    mpSipUserAgent->getViaInfo(OsSocket::UDP, viaHost, viaPort);
    // The signature should be the same after a restart or any calls
    // that are up will have problems with mid-dialog transactions
    //sprintf(signBuf, "%d:%ld:%d:", ranNum, epochTime, viaPort);
    sprintf(signBuf, "%d:", viaPort);
    mSignatureSecret = signBuf;
    mSignatureSecret.append(viaHost);

    /*Config files which are specific to a component
    (e.g. mappingrules.xml is to sipregistrar) Use the
    following logic:
    1) If  directory ../etc exists:
        The path to the data file is as follows
        ../etc/<data-file-name>

    2) Else the path is assumed to be:
       ./<data-file-name>
    */
    OsPath workingDirectory ;
    if ( OsFileSystem::exists( CONFIG_ETC_DIR ) )
    {
        workingDirectory = CONFIG_ETC_DIR;
        OsPath path(workingDirectory);
        path.getNativePath(workingDirectory);

    } else
    {
        OsPath path;
        OsFileSystem::getWorkingDirectory(path);
        path.getNativePath(workingDirectory);
    }

    UtlString fileName =
    workingDirectory +
    OsPathBase::separator +
    AUTH_RULES_FILENAME;

    mpAuthorizationRules->loadMappings(fileName);

    // Register to get incoming requests
    OsMsgQ* queue = getMessageQueue();

    sipUserAgent.addMessageObserver(
        *queue,
        "",      // All methods
        TRUE,    // Requests,
        FALSE,   // Responses,
        TRUE,    // Incoming,
        FALSE,   // OutGoing,
        "",      // eventName,
        NULL,    // SipSession* pSession,
        NULL);   // observerData

    // The period of time in seconds that nonces are kept
    mNonceExpiration = 60 * 5;

    // Set up a periodic timer for nonce garbage collection
    OsQueuedEvent* queuedEvent = new OsQueuedEvent(*queue, 0);
    OsTimer* timer = new OsTimer(*queuedEvent);
    // Once a minute
    OsTime lapseTime(60,0);
    timer->periodicEvery(lapseTime, lapseTime);
}

// Copy constructor
SipAaa::SipAaa(const SipAaa& rSipAaa)
{}

// Destructor
SipAaa::~SipAaa()
{}

/* ============================ MANIPULATORS ============================== */

UtlBoolean
SipAaa::handleMessage( OsMsg& eventMessage )
{
    int msgType = eventMessage.getMsgType();
    int msgSubType = eventMessage.getMsgSubType();

    // Timer event
    if(msgType == OsMsg::OS_EVENT &&
		msgSubType == OsEventMsg::NOTIFY)
    {
        // Garbage collect nonces
        //osPrintf("Garbage collecting nonces\n");

        OsTime time;
        OsDateTime::getCurTimeSinceBoot(time);
        long now = time.seconds();
        // Remove nonces more than 5 minutes old
        long oldTime = now - mNonceExpiration;
        mNonceDb.removeOldNonces(oldTime);
    }

    // SIP message event
    else if ( msgType == OsMsg::PHONE_APP )
    //&& msgSubType == CP_SIP_MESSAGE)
    {
        SipMessage* sipRequest = (SipMessage*)((SipMessageEvent&)eventMessage).getMessage();
        int messageType = ((SipMessageEvent&)eventMessage).getMessageStatus();
        UtlString callId;

        if ( messageType == SipMessageEvent::TRANSPORT_ERROR )
        {
            //osPrintf("ERROR: SipAaa::handleMessage received transport error message\n");
            syslog(FAC_SIP, PRI_ERR, "ERROR: SipAaa::handleMessage received transport error message\n") ;
        } else if ( sipRequest )
        {
            if ( sipRequest->isResponse() )
            {
                syslog(FAC_AUTH, PRI_ERR, "ERROR: SipAaa::handleMessage received response\n");
                //osPrintf("ERROR: SipAaa::handleMessage received response\n");
            } else
            {
                UtlString myRouteUri;
                UtlString targetUri;
                UtlBoolean isNextHop = FALSE;
                UtlString nextHopUri;
                UtlString firstRouteUri;
                UtlBoolean routeExists = sipRequest->getRouteUri(0, &firstRouteUri);
                // If there is a route header just send it on its way
                if ( routeExists )
                {
#ifdef TEST_PRINT
                    osPrintf("SipAaa::handleMessage found a route\n");
#endif
                    // Check the URI.  If the URI has lr the
                    // previous proxy was a strict router
                    UtlString requestUri;
                    sipRequest->getRequestUri(&requestUri);
                    Url routeUrlParser(requestUri, TRUE);
                    UtlString dummyValue;
                    UtlBoolean previousHopStrictRoutes =
                        routeUrlParser.getUrlParameter( "lr", dummyValue, 0 );
                    UtlBoolean uriIsMe = mpSipUserAgent->isMyHostAlias(routeUrlParser);
                    Url firstRouteUriUrl(firstRouteUri);
                    UtlBoolean firstRouteIsMe = mpSipUserAgent->isMyHostAlias(firstRouteUriUrl);

                    // If the URI is not this server and the
                    // URI is not marked as a loose route and
                    // the first route is not this server then
                    // this server was not in the routes and we
                    // really do not know if the route set is
                    // set up as loose or strict routing.  We
                    // have to assume that it is strict routing.
                    if(!previousHopStrictRoutes &&
                       !uriIsMe &&
                       !firstRouteIsMe)
                    {
                        previousHopStrictRoutes = TRUE;
                    }

                    // If the URI does not have the loose route
                    // tag and the URI is pointed to this server
                    // we assume the previous hop strict routed
                    else if(!previousHopStrictRoutes &&
                       uriIsMe)
                    {
                        previousHopStrictRoutes = TRUE;
                    }

                    if ( previousHopStrictRoutes )
                    {
#ifdef TEST_PRINT
                        osPrintf("SipAaa::handleMessage previous hop strict routed\n");
#endif
                       // If this is NOT my host and port in the URI
                        if(! uriIsMe)
                        {
                            OsSysLog::add(FAC_SIP, PRI_WARNING, "Strict route: %s not to this server",
                                requestUri.data());
                            // Put the route back on as this URI is
                            // not this server.
                            sipRequest->addRouteUri(requestUri);

                            myRouteUri = "";
                        }
                        else
                        {
                            myRouteUri = requestUri;
                        }

                        // We have to pop the last route and
                        // put it in the URI
                        UtlString contactUri;
                        int lastRouteIndex;
                        sipRequest->getLastRouteUri(contactUri, lastRouteIndex);
#ifdef TEST_PRINT
                        osPrintf("SipAaa::handleMessage setting new URI: %s\n",
                                 contactUri.data());
#endif

                        sipRequest->removeRouteUri(lastRouteIndex, &contactUri);
#ifdef TEST_PRINT
                        osPrintf("SipAaa::handleMessage route removed: %s\n",
                                 contactUri.data());
#endif
                        // Put the last route in a the URI
                        Url newUri(contactUri);
                        newUri.getUri(contactUri);
                        sipRequest->changeRequestUri(contactUri);
#ifdef TEST_PRINT
                        UtlString bytes;
                        int len;
                        sipRequest->getBytes(&bytes, &len);
                        osPrintf("SipAaa: \nStrict\n%s\nNow Loose\n",
                                 bytes.data());

                        // Where are we going?
                        // These are used for authorization
                        targetUri = contactUri;
                        isNextHop = sipRequest->getRouteUri(0, &nextHopUri);
                        if ( !isNextHop ) nextHopUri = "";
#endif
                    } else
                    {
#ifdef TEST_PRINT
                        osPrintf("SipAaa::handleMessage previous hop loose routed\n");
#endif
                        // If this route is to me, pop it off
                        if(firstRouteIsMe)
                        {
                            // THis is a loose router pop my route off
                            UtlString dummyUri;
                            sipRequest->removeRouteUri(0, &dummyUri);
                            myRouteUri = firstRouteUri;
                        }
                        else
                        {
                            myRouteUri = "";
                            OsSysLog::add(FAC_SIP, PRI_WARNING, "Loose route: %s not to this server",
                                firstRouteUri.data());
                        }

                        // Where are we going?
                        // These are used for authorization
                        targetUri = requestUri;
                        isNextHop = sipRequest->getRouteUri(0, &nextHopUri);
                        if ( !isNextHop )
                            nextHopUri = "";
                    }
                } else
                {
                    // There is no route check if it is mapped to something local
//#ifdef TEST_PRINT
                    syslog(FAC_SIP, PRI_WARNING, "SipAaa::handleMessage no route found\n") ;
                    //osPrintf("WARNING: SipAaa::handleMessage no route found\n");
//#endif
                    UtlString uri;
                    sipRequest->getRequestUri(&uri);
                    //Url originalUri(uri);
                    //UtlString domain;
                    //originalUri.getHostAddress(domain);
                    //int port = originalUri.getHostPort();
                    //if(port <= 0) port = SIP_PORT;
                    //char portBuf[64];
                    //sprintf(portBuf, "%d", port);
                    //domain.append(':');
                    //domain.append(portBuf);
//#ifdef TEST_PRINT
                    //osPrintf("SipAaa::handleMessage uri domain: %s\n",
                    //    domain.data());
//#endif
                    // Where are we going?
                    // These are used for authorization
                    myRouteUri = "";
                    targetUri = uri;
                    isNextHop = FALSE;
                    nextHopUri = "";

                    //SipMessage routeErrorResponse;
                    //routeErrorResponse.setResponseData(sipRequest,
                    //    SIP_NOT_FOUND_CODE,
                    //    "Routing error");
                    //mpSipUserAgent->send(routeErrorResponse);
                }

                // Get some info about the request
                // (method, to, from & tags)
                UtlString method;
                sipRequest->getRequestMethod(&method);
                Url fromUrl;
                Url toUrl;
                sipRequest->getFromUrl(fromUrl);
                sipRequest->getToUrl(toUrl);
                UtlString fromTag;
                UtlString toTag;
                fromUrl.getFieldParameter("tag", fromTag);
                toUrl.getFieldParameter("tag", toTag);

                // If there is a route to me and this is not
                // the inital call setup (e.g. to tag is set)
                UtlBoolean routeSignatureIsValid = FALSE;
                UtlBoolean toMatches = FALSE;
                UtlBoolean fromMatches = FALSE;
                UtlString routePermission;
                UtlString routeTag;
                UtlString routeSignature;
                if(!myRouteUri.isNull() &&
                    !toTag.isNull())
                {
                    Url myRouteUrl(myRouteUri);


                    myRouteUrl.getUrlParameter("a", routePermission);
                    myRouteUrl.getUrlParameter("t", routeTag);
                    myRouteUrl.getUrlParameter("s", routeSignature);

                    // The authentication and authorization only applies
                    // to one direction.
                    UtlString validRouteSignature;
                    if(toTag.compareTo(routeTag) == 0)
                    {
                        toMatches = TRUE;
                        calcRouteSignature(routePermission,
                                           callId,
                                           toTag,
                                           validRouteSignature);
                    }
                    else if(fromTag.compareTo(routeTag) == 0)
                    {
                        fromMatches = TRUE;
                        calcRouteSignature(routePermission,
                                           callId,
                                           fromTag,
                                           validRouteSignature);
                    }

                    // If the calculated signature and the one in the route
                    // match, the route and permission are valid
                    if(validRouteSignature.compareTo(routeSignature) == 0)
                    {
                        routeSignatureIsValid = TRUE;
                    }
                    else
                    {
                        OsSysLog::add(FAC_SIP, PRI_WARNING, "invalid route call-id: %s signature a: %s t: %s s: %s",
                            callId.data(), routePermission.data(),
                            routeTag.data(), routeSignature.data());
                    }
                }

                // Check if we need to authenticate and authorize the originator.
                ResultSet permissions;
                UtlBoolean isPstnNumber;
                // We do not authenticate ACKs
                if (mpAuthorizationRules &&
                    method.compareTo(SIP_ACK_METHOD) != 0)
                {
                    // We have a valid route with a signature
                    if(routeSignatureIsValid)
                    {
                        // We can use the permission from the
                        // route field.

                        // Request from the caller that required
                        // authorization
                        if(fromMatches && !routePermission.isNull())
                        {
                            // Need to authenticate
                            UtlHashMap record;
                            UtlString* permissionKey =
                                new UtlString("permission");
                            UtlString* permissionValue =
                                new UtlString(routePermission);

                            record.insertKeyAndValue(
                                permissionKey,permissionValue);

                            permissions.addValue(record);
                        }

                        // Request from the original callee
                        // or Request from the caller that requires
                        // no authentication
                        else // if(toMatches) or no permissions
                        {
                            // No authentication or authorization
                            // required
                            if(!fromMatches)
                            {
                                OsSysLog::add(FAC_SIP, PRI_INFO, "upstream request call-id: %s, not authenticating",
                                    callId.data());
                            }
                            else if(routePermission.isNull())
                            {
                                OsSysLog::add(FAC_SIP, PRI_INFO, "signed route call-id: %s with no authentication required",
                                    callId.data());
                            }
                            else
                            {
                                OsSysLog::add(FAC_SIP, PRI_INFO, "request call-id %s to unrestricted entity with signed route",
                                    callId.data());
                            }
                        }


                    }

                    else
                    {
                        Url targetUrl(targetUri);
                        mpAuthorizationRules->getPermissionRequired(
                            targetUrl, isPstnNumber, permissions);

                        // There was no authorization required for contact
                        // Try the next hop as well
                        if ( isNextHop && permissions.getSize()==0 )
                        {
                            Url nextHopUrl( nextHopUri );
                            mpAuthorizationRules->getPermissionRequired (
                                nextHopUrl, isPstnNumber, permissions );
                        }
                        //syslog(FAC_SIP, PRI_DEBUG,"SipAaa::handleMessage requires %s authorization\n",
                        //    authorizationType.isNull() ? "no" : authorizationType.data()) ;
                        //osPrintf("SipAaa::handleMessage requires %s authorization\n",
                        //    authorizationType.isNull() ? "no" : authorizationType.data());

                    }
                }

                UtlBoolean needsAuthentication;
                // We always have to route now so that we can sign
                // the route
                UtlBoolean needsRecordRouting = TRUE;
                if(permissions.getSize() > 0)
                {
                    needsRecordRouting = TRUE;
                    UtlString rulePermission;
                    if(permissions.getSize() == 1)
                    {
                        UtlString permissionKey("permission");
                        UtlHashMap record;
                        permissions.getIndex(0, record);
                        rulePermission = *((UtlString*)record.findValue(&permissionKey));
                    }

                    // If exactly one permission is required and it is
                    // RecordRoute, there is no need to authenticate.
                    if(rulePermission.compareTo("RecordRoute", UtlString::ignoreCase) == 0)
                    {
                        needsAuthentication = FALSE;
                    }
                    else
                    {
                        // @JC Forwarding calls to the PSTN gateway workaround
                        // if incoming calls (fromUrl) match a an entry in the
                        // AuthexceptionDB then we do not require authentication
                        UtlString requestUri;
                        sipRequest->getRequestUri( &requestUri );
                        Url requestUrl ( requestUri, TRUE );
                        UtlString userid;
                        requestUrl.getUserId(userid);
                        if ( AuthexceptionDB::getInstance()->isException( userid ) )
                        {
                            needsAuthentication = FALSE;
                        }
                        else
                        {
                            needsAuthentication = TRUE;
                        }
                    }
                }
                else
                {
                    needsAuthentication = FALSE;
                }

                // Authenticate if we need to
                SipMessage authResponse;
                UtlString authUser;
                UtlString matchedPermission;

                if (needsAuthentication &&
                     (!isAuthenticated(*sipRequest, authUser, authResponse) ||
                      !isAuthorized( *sipRequest, permissions, authUser,
                                     authResponse, matchedPermission ) ) )
                {   // Either not authenticated or not authorized
                    mpSipUserAgent->send(authResponse);
                } else
                { // Otherwise route it on
                    // Record route if authenticated
                    if ( needsRecordRouting )
                    {
                        Url routeUrl(mRouteName);
                        if(mRouteName.isNull())
                        {
                            UtlString uriString;
                            int port;
                            //sipRequest->getRequestUri(&uriString);

                            mpSipUserAgent->getViaInfo(
                                OsSocket::UDP, uriString, port );
#ifdef TEST_PRINT
                            osPrintf("SipAaa:handleMessage Record-Route address: %s port: %d\n",
                                     uriString.data(), port);
#endif

                            routeUrl.setHostAddress(uriString.data());
                            routeUrl.setHostPort(port);
                        }
#ifdef TEST_PRINT
                        else
                        {
                            osPrintf("SipAaa:handleMessage Record-Route mRouteName: %s\n",
                                mRouteName.data());
                        }
#endif

                        routeUrl.setUrlParameter("lr", "");
                        UtlString signature;

                        // Preserve the signature if it is valid
                        if(routeSignatureIsValid)
                        {
                            routeUrl.setUrlParameter("a", routePermission);
                            routeUrl.setUrlParameter("t", routeTag);
                            routeUrl.setUrlParameter("s", routeSignature);
                        }

                        // We only add the signature on the initial dialog request
                        else if(toTag.isNull())
                        {
                            calcRouteSignature(matchedPermission,
                                               callId,
                                               fromTag,
                                               signature);
                            routeUrl.setUrlParameter("a", matchedPermission);
                            routeUrl.setUrlParameter("t", fromTag);
                            routeUrl.setUrlParameter("s", signature);
                        }

                        // Check to see if we have already added a record-route
                        // This is a minor optimization.  It avoids the spiral
                        // on the mid-dialog transactions and keeps the message
                        // a little smaller to avoid UDP fragmentation which cause
                        // a problem with Cisco phones.
                        UtlString previousRRoute;
                        UtlBoolean newRRouteIsUnique = TRUE;
                        if(sipRequest->getRecordRouteUri(0, &previousRRoute))
                        {
                            // If the host, port and user ID are the same the
                            // record-route to be added and the top most record-route
                            // already in the request.
                            Url prevRRouteUrl(previousRRoute);
                            if(prevRRouteUrl.isUserHostPortEqual(routeUrl)) 
                            {
                                UtlString prevPermission;
                                UtlString prevFromTag;
                                UtlString prevSignature;
                                // If the permission, from tag and signature
                                // of the exist record-route matches the one
                                // to be added, mark it as not unique so that
                                // we do not add it again
                                if(prevRRouteUrl.getUrlParameter("a", prevPermission) &&
                                   prevPermission.compareTo(matchedPermission) == 0 &&
                                   prevRRouteUrl.getUrlParameter("t", prevFromTag) &&
                                   prevFromTag.compareTo(fromTag) == 0 &&
                                   prevRRouteUrl.getUrlParameter("s", prevSignature) &&
                                   prevSignature.compareTo(signature) == 0)
                                {
                                    newRRouteIsUnique = FALSE;
                                }
                            }
                        }

                        if(newRRouteIsUnique)
                        {
                            //routeUrl.setAngleBrackets();
                            UtlString recordRoute = routeUrl.toString();
                            sipRequest->addRecordRouteUri(recordRoute);
                        }
                    }

                    // Decrement max forwards
                    int maxForwards;
                    if ( sipRequest->getMaxForwards(maxForwards) )
                    {
                        maxForwards--;
                    } else
                    {
                        maxForwards = mpSipUserAgent->getMaxForwards();
                    }
                    sipRequest->setMaxForwards(maxForwards);

                    sipRequest->resetTransport();
                    mpSipUserAgent->send(*sipRequest);
                }
            }
        }
    }

    return(TRUE);
}

// Assignment operator
SipAaa&
SipAaa::operator=(const SipAaa& rhs)
{
    if ( this == &rhs )            // handle the assignment to self case
        return *this;

    return *this;
}

/* ============================ ACCESSORS ================================= */

/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */

UtlBoolean SipAaa::isAuthenticated(
    const SipMessage& sipRequest,
    UtlString& authUser,
    SipMessage& authResponse)
{
    UtlBoolean authenticated = FALSE;
    UtlString requestUser;
    UtlString requestRealm;
    UtlString requestNonce;
    UtlString requestUri;
    int requestAuthIndex = 0;
    UtlString callId;
    Url fromUrl;
    UtlString fromTag;
    OsTime time;
    OsDateTime::getCurTimeSinceBoot(time);
    long now = time.seconds();
    long nonceExpires = mNonceExpiration;

    sipRequest.getCallIdField(&callId);
    sipRequest.getFromUrl(fromUrl);
    fromUrl.getFieldParameter("tag", fromTag);

    while ( sipRequest.getDigestAuthorizationData(
                &requestUser,
                &requestRealm,
                &requestNonce,
                NULL,
                NULL,
                &requestUri,
                HttpMessage::PROXY,
                requestAuthIndex) )
    {
        syslog(FAC_AUTH, PRI_DEBUG, "SipRegistrar::isAuthenticated() - "
               "Message already has authorization, check if it is correct\n");
        syslog(FAC_AUTH, PRI_DEBUG, "SipRegistrar::isAuthenticated() - "
               "reqRealm-%s, reqUser-%s\n", requestRealm.data() , requestUser.data());

        if ( mRealm.compareTo(requestRealm) == 0 ) // case sensitive
        {
            syslog(FAC_AUTH, PRI_DEBUG, "SipAaa:isAuthenticated requestUser: %s requestRealm: %s\n",
                   requestUser.data(), requestRealm.data());

            // osPrintf("SipAaa:isAuthenticated requestUser: %s requestRealm: %s\n",
            //    requestUser.data(), requestRealm.data());

            UtlString userUrlString(requestUser);
            int atIndex = userUrlString.index("@");
            if ( atIndex < 0 )
            {
                userUrlString.append("@");
                userUrlString.append(mRealm);
            }
            syslog(FAC_AUTH, PRI_DEBUG, "SipAaa:isAuthenticated credentialUri: %s\n", userUrlString.data());
            //osPrintf("SipAaa:isAuthenticated credentialUri: %s\n", userUrlString.data());

            Url userUrl( userUrlString );

            // As URIs change and get mapped along the way
            // we have to trust the one passed in via the
            // auth field
            //UtlString reqUri;
            //sipRequest.getRequestUri(&reqUri);
            UtlString authTypeDB;
            UtlString passTokenDB;

            // Ignore this credential if it is not a current valid nonce
            if (mNonceDb.isNonceValid(requestNonce, callId, fromTag,
                                          requestUri, mRealm, nonceExpires))
            {
                // then get the credentials for this user and realm
                if(CredentialDB::getInstance()->getCredentialByUserid(userUrl,
                                                                      mRealm,
                                                                      requestUser,
                                                                      passTokenDB,
                                                                      authTypeDB))
                {
#ifdef TEST_PRINT
                    // THIS SHOULD NOTE BE PRINTED OUT IN PRODUCTION for
                     // security reasone we do not want to spread pass tokens
                     // all over the system
                    syslog(FAC_AUTH, PRI_DEBUG, "SipAaa::isAuthenticated found credential user: \"%s\" passToken: \"%s\"\n",
                           requestUser.data(), passTokenDB.data());
#endif
                    //osPrintf("SipAaa::isAuthenticated found credential user: \"%s\" passToken: \"%s\"\n",
                    //    requestUser.data(), passTokenDB.data());

                    authenticated =
                    sipRequest.verifyMd5Authorization( requestUser.data(),
                                                       passTokenDB.data(),
                                                       requestNonce,
                                                       requestRealm.data(),
                                                       requestUri,
                                                       HttpMessage::PROXY );

                    syslog(FAC_AUTH, PRI_DEBUG, "SipAaa::isAuthenticated %s\n",
                           authenticated ? "succeeded" : "failed");
                    //osPrintf("SipAaa::isAuthenticated %s\n",
                    //    authenticated ? "succeeded" : "failed");

                    if ( authenticated )
                    {
                        authUser = userUrlString;

                        syslog(FAC_AUTH, PRI_DEBUG, "SipAaa::isAuthenticated() request is AUTHENTICATED\n");
                    } else
                    {
                        syslog(FAC_AUTH, PRI_DEBUG, "SipAaa::isAuthenticated() request is NOT AUTHENTICATED\n");
                    }
                    break;
                }
                // Did not find credentials in DB
                else
                {
                    syslog(FAC_AUTH, PRI_INFO,
                        "No credentials found for user: \"%s\" realm: \"%s\"",
                        requestUser.data(), mRealm.data());
                }
            } // end check credentials

            // Is not a valid nonce
            else
            {
                syslog(FAC_AUTH, PRI_INFO,
                    "Invalid NONCE: %s found call-id: %s from tag: %s uri: %s realm: %s expiration: %d",
                     requestNonce.data(), callId.data(), fromTag.data(),
                     requestUri.data(), mRealm.data(), nonceExpires);
            }
        }
        requestAuthIndex++;
    }//end while

    if ( !authenticated )
    {
        authUser = "";
        UtlString newNonce;
        UtlString challangeRequestUri;
        sipRequest.getRequestUri(&challangeRequestUri);
        UtlString opaque;
        UtlString opaqueSeed;

        mNonceDb.createNewNonce(callId,
                                fromTag,
                                challangeRequestUri,
                                mRealm,
                                newNonce);

        opaqueSeed = newNonce;
        opaqueSeed.append("abcdefghij");
        NetMd5Codec::encode(opaqueSeed.data(), opaque);

        authResponse.setRequestUnauthorized(&sipRequest,
                                            HTTP_DIGEST_AUTHENTICATION,
                                            mRealm,
                                            newNonce, // nonce
                                            opaque, // opaque
                                            HttpMessage::PROXY);
    }

    syslog(FAC_AUTH, PRI_DEBUG, "SipAaa::isAuthenticated user: %s %s\n",
           authUser.isNull() ? "none" : authUser.data(),
           authenticated ? "authenticated" : "authentication failed");

    /*osPrintf("SipAaa::isAuthenticated user: %s %s\n",
        authUser.isNull() ? "none" : authUser.data(),
        authenticated ? "authenticated" : "authentication failed");*/

    return(authenticated);
}

UtlBoolean
SipAaa::isAuthorized (
    const SipMessage& sipRequest,
    const ResultSet& requiredPermissions,
    const char* authUser,
    SipMessage& authResponse,
    UtlString& matchedPermission)
{
    UtlBoolean authorized = FALSE;
    UtlString unMatchedPermissions;

    UtlString userUrlString(authUser ? authUser : "");
    if ( userUrlString.index("@") < 0 )
    {
        userUrlString.append("@");
        userUrlString.append(mRealm);
    }
    Url userUrl(userUrlString);

    // get all permissions associated with this user
    ResultSet grantedPermissions;
    PermissionDB::getInstance()->getPermissions( userUrl, grantedPermissions );

    // dpetrie:  I think that these loops are backwards.  It appears that
    // this is verifying that the user has at least one of the
    // required permissions.  I think that what is is SUPPOSED to
    // do is verify that the user has ALL of the required permissions.
    UtlString identityKey ("identity");
    UtlString permissionKey ("permission");

    int numGrantedPermissions = grantedPermissions.getSize();
    for (int i = 0; i< numGrantedPermissions; i++ )
    {

        UtlHashMap record;
        grantedPermissions.getIndex( i, record );

        UtlString rowUri        = *((UtlString*)record.findValue(&identityKey));
        UtlString grantedRowPermission = *((UtlString*)record.findValue(&permissionKey));

        syslog(FAC_AUTH, PRI_DEBUG, "SipAaa::isAuthorized found uri: %s permission: %s\n",
               rowUri.data(), grantedRowPermission.data());
        //osPrintf("SipAaa::isAuthorized found uri: %s permission: %s\n",
        //    rowUri.data(), rowPermission.data());

        // Search through the input permissions set for a match
        int numRequiredPermissions = requiredPermissions.getSize();
        if ( numRequiredPermissions > 0 )
        {
            UtlString permDB;
            for (int j=0; j<numRequiredPermissions; j++ )
            {
                // WARNING: I think that the following reuse
                // of the record object is wiping out rows in
                // the localPermisions result set for
                // numLocalPermissions > 1.  However I am
                // afraid to change it. dpetrie
                requiredPermissions.getIndex( j, record );

                // I do not know why rowUri even exists as a variable in
                // this method dpetrie
                //rowUri = *((UtlString*)record.findValue( &identityKey ));

                permDB = *((UtlString*)record.findValue( &permissionKey ));
                if ( permDB.compareTo( grantedRowPermission, UtlString::ignoreCase ) == 0 ||
                    permDB.compareTo("ValidUser", UtlString::ignoreCase ) == 0 ||
                    permDB.compareTo("RecordRoute", UtlString::ignoreCase ) == 0)
                {
                    matchedPermission.append( permDB );
                    authorized = TRUE;
                    break;
                } else
                {
                    // only the first time we need to fill in the string
                    // to get the names of permissions requeired to be send back in case of an error
                    if ( i == 0 )
                    {
                        if ( !unMatchedPermissions.isNull() )
                        {
                            unMatchedPermissions.append("+");
                        }
                        unMatchedPermissions.append(permDB);
                    }
                }
                permDB.remove(0);
            } // end 2nd for loop require permissions
        }
    } //end 1st for loop over granted permission

    // dpetrie: As the above loops are inside out and I am not about to change
    // it at this stage of the release, we have to catch the fact that
    // the user has no permissions and get the list that were
    // required
    if (!authorized)
    {
        UtlHashMap requiredPermRecord;
        int numRequiredPermissions = requiredPermissions.getSize();
        for(int i = 0; i < numRequiredPermissions; i++)
        {
            requiredPermissions.getIndex( i, requiredPermRecord );
            UtlString* requirePermPtr =
                ((UtlString*)requiredPermRecord.findValue( &permissionKey ));
            if(requirePermPtr)
            {
                if ( !unMatchedPermissions.isNull() )
                {
                    unMatchedPermissions.append("+");
                }
                unMatchedPermissions.append(*requirePermPtr);
            }
        }
    }

    syslog(FAC_AUTH, PRI_DEBUG, "SipAaa::isAuthorized user: %s %s for %s\n",
           authUser && *authUser ? authUser : "none",
           authorized ? "authorized" : "not authorized",
           matchedPermission.data());

    /*osPrintf("SipAaa::isAuthorized user: %s %s for %s\n",
        authUser && *authUser ? authUser : "none",
        authorized ? "authorized" : "not authorized",
        authorizationType);*/

    if ( !authorized )
    {
        UtlString errorText("Not authorized for ");
        errorText += unMatchedPermissions;

        authResponse.setResponseData(&sipRequest,
                                     SIP_FORBIDDEN_CODE,
                                     errorText.data());
    }
    return(authorized);
}


void SipAaa::calcRouteSignature(UtlString& matchedPermission,
                               UtlString& callId,
                               UtlString& fromTag,
                               UtlString& signature)
{
    UtlString signatureData(mSignatureSecret);
    signatureData.append(":");
    signatureData.append(matchedPermission);
    signatureData.append(":");
    signatureData.append(callId);
    signatureData.append(":");
    signatureData.append(fromTag);

    NetMd5Codec::encode(signatureData, signature);
}


/* ============================ FUNCTIONS ================================= */

